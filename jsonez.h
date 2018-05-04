/* jsonez.h v0.21 - public domain easy json parser - github url
						  no warranty implied; use at your own risk


   Do this:
      #define JSONEZ_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define JSONEZ_IMPLEMENTATION
   #include "jsonez.h"


	Latest revision history:
		0.21 (2018-04-06) Bunches of changes
	   0.20 (2017-07-07)	Adding creation and output
		0.10 (2017-03-14)	Initial Release	

	See end of file for full revision history and license.

*/


#ifndef INCLUDE_JSONEZ_H
#define INCLUDE_JSONEZ_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
typedef _Bool bool;
#endif

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#ifndef nullptr
#define nullptr 0
#endif


#ifdef JSONEZ_STATIC
#define JSONEZDEF static
#else
#define JSONEZDEF extern
#endif


#ifndef JSON_REPORT_ERROR
#define JSON_REPORT_ERROR(msg, p) fprintf(stderr, "PARSE ERROR (%d): " msg " at %s\n", __LINE__, p)
#endif


typedef enum jsonez_type {
	JSON_UNKNOWN,
	JSON_OBJ,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOL,
} jsonez_type;


typedef struct jsonez {
	jsonez_type type;
	char *key;
	union {
		char *s; // string 
		int i; // boolean or count
		double n; // number 
	};
	struct jsonez *next;
	struct jsonez *child;
} jsonez;


typedef struct jsonez_ctx {
	bool quote_keys;
	int indent_length;
	bool use_equal_sign;
	bool add_root_object;
} jsonez_ctx;


JSONEZDEF jsonez *jsonez_parse(char *file);
JSONEZDEF void jsonez_free(jsonez *json);
JSONEZDEF jsonez *jsonez_find(jsonez *parent, const char *key);


// TODO - can create some stuff without names to put in arrays
JSONEZDEF jsonez *jsonez_create_root();
JSONEZDEF jsonez *jsonez_create_object(jsonez *parent, char *key);
JSONEZDEF jsonez *jsonez_create_array(jsonez *parent, char *key);
JSONEZDEF jsonez *jsonez_create_bool(jsonez *parent, char *key, bool value);
JSONEZDEF jsonez *jsonez_create_numd(jsonez *parent, char *key, double value);
JSONEZDEF jsonez *jsonez_create_numf(jsonez *parent, char *key, float value);
JSONEZDEF jsonez *jsonez_create_numi(jsonez *parent, char *key, int value);
JSONEZDEF jsonez *jsonez_create_string(jsonez *parent, char *key, char *value);


JSONEZDEF char *jsonez_to_string(jsonez *root, jsonez_ctx *ctx);
JSONEZDEF void jsonez_free_string(char *string);


#ifdef __cplusplus
}
#endif


#endif // INCLUDE_JSONEZ_H


////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
// IMPLEMENTATION
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#ifdef JSONEZ_IMPLEMENTATION


#define JSONEZ_BETWEEN(a,b,c) ((a) >= (b) && (a) <= (c))
#define JSONEZ_RAW_KEY(c) (JSONEZ_BETWEEN((c),'0','9')||JSONEZ_BETWEEN((c),'a','z')||JSONEZ_BETWEEN((c),'A','Z')||((c)=='_'))
#define JSONEZ_WHITESPACE(c) (JSONEZ_BETWEEN((c),0,32))
#define JSONEZ_ESCAPE(c) ((c)=='"'||(c)=='\\'||(c)=='/'||(c)=='b'||(c)=='f'||(c)=='n'||(c)=='r'||(c)=='t')
#define JSONEZ_VALID_STRING(c) (JSONEZ_BETWEEN((c),' ','~'))
#define JSONEZ_NUMBER(c) (JSONEZ_BETWEEN((c),'0','9')||(c)=='e'||(c)=='E'||(c)=='+'||(c)=='-'||(c)=='.')
#define JSONEZ_SKIP_WHITESPACE(p) while( (p) && *(p) && JSONEZ_WHITESPACE(*(p))) { (p)++; }
#define JSONEZ_IS_SINGLE_COMMENT(p) (p && (*p) && (*p=='/') && (*(p+1)) && (*(p+1)=='/'))
#define JSONEZ_IS_MULTI_COMMENT(p) (p && (*p) && (*p=='/') && (*(p+1)) && (*(p+1)=='*'))


#define JSONEZ_WRITE_STRING(stf, fmt, ...) do { \
	int written = snprintf(stf->ptr, stf->remaining, fmt, ##__VA_ARGS__); \
	stf->total += written; \
	if (stf->ptr) stf->ptr += written; \
	stf->remaining -= written; \
	if (stf->remaining < 0) stf->remaining = 0; \
} while(0)


typedef struct jsonez_output {

	char *ptr;
	int total;
	int remaining;

} jsonez_output;


static char *jsonez_parse_object(jsonez *parent, char *p);
static void jsonez_print_key_value(jsonez_output *out, int space, jsonez *obj, jsonez_ctx *ctx);
static void jsonez_print_value(jsonez_output *out, int space, jsonez *value, jsonez_ctx *ctx);


static char *jsonez_skip_whitespace(char *p) {
	JSONEZ_SKIP_WHITESPACE(p);
	while (JSONEZ_IS_SINGLE_COMMENT(p) || JSONEZ_IS_MULTI_COMMENT(p)) {
		if (JSONEZ_IS_SINGLE_COMMENT(p)) {
			while(*p && (*p != '\n')) {
				++p;
			}
			if (p == NULL || *p != '\n') {
				JSON_REPORT_ERROR("malformed single line comment", p);
			}
		} else if (JSONEZ_IS_MULTI_COMMENT(p)) {
			bool end = false;
			while (*p) {
				bool maybe = *p++ == '*';
				if (maybe && (*p++ == '/')) {
					end = true;
					break;
				}
			}
			if (!end) {
				JSON_REPORT_ERROR("malformed /* */ multiline comment", p);
			}
		}
		JSONEZ_SKIP_WHITESPACE(p);
	}
	return p;
}


static jsonez *jsonez_create(jsonez *parent, char *key) {

	jsonez *json = (jsonez *)calloc(1, sizeof(jsonez));
	json->type = JSON_UNKNOWN;
	if (key) {
		json->key = strdup(key);
	} 

	if(!parent->child) {
		parent->child = json;
	} else {
		jsonez *child = parent->child;
		while(child->next) {
			child = child->next;
		}
		child->next = json;
	}

	parent->i++;
	return json;
}


static char *jsonez_next_arr(char *p) {

	p = jsonez_skip_whitespace(p);

	if(*p==',') {
		p++;
	}

	p = jsonez_skip_whitespace(p);

	if(*p==']') return p;
	if(*p=='{'||*p=='['||*p=='"'||*p=='t'||*p=='f'||JSONEZ_NUMBER(*p)) return p;

	JSON_REPORT_ERROR("Neverending Array",p);
	return 0; // error of some king

}


static char *jsonez_next_obj(char *p) {

	p = jsonez_skip_whitespace(p);

	if(*p=='\0') return p;
	if(*p=='}'||*p==']') return p;

	if(*p==',') {
		p++;
	} else {
		JSON_REPORT_ERROR("Next item missing",p);
		return 0; //error of some king
	}

	p = jsonez_skip_whitespace(p);

	if(*p=='\0') return p;
	if(*p=='}'||*p==']') return p;
	if(*p=='"'||*p=='t'||*p=='f'||JSONEZ_RAW_KEY(*p)) return p;

	JSON_REPORT_ERROR("Unexptected end of file",p);
	return 0; // error of some king

}


static char *jsonez_skip_key_separator(char *p) {

	p = jsonez_skip_whitespace(p);

	if(*p==':' || *p=='=') {
		p++;
	} else {
		JSON_REPORT_ERROR("Missing ':' key separator",p);
		return 0; // error of some kind
	}

	p = jsonez_skip_whitespace(p);

	if(*p=='"'||*p=='t'||*p=='f'||JSONEZ_NUMBER(*p)||*p=='['||*p=='{') return p;

	JSON_REPORT_ERROR("Unknow Value type",p);
	return 0; // error of some kind

}


static char *jsonez_parse_quote_string(char **key, char *p) {

	char *s = p;
	int len = 0;
	char c = 0;

	while((c=*++p)) {
		if(c == '\\') {
			p++;
			if(*p && JSONEZ_ESCAPE(*p)) {
				len++;
			} else {
				JSON_REPORT_ERROR("Unknown escape sequence", p);
				return 0;
			}
		} else if(c == '"') {
			break;
		} else if(JSONEZ_VALID_STRING(c)) {
			len++;
		}
	}

	if(c == '"') {
		// copies over the escaped string
		// actually return good string
		// the len is not the actual length of the string
		// but the escaped length
		char *str = (char*)calloc(len+1, sizeof(char));
		char *d = str;
		for(int i = 0; i < len; ++i) {
			if(*++s == '\\') {
				switch(*++s) {
					case '"': {
						*d++ = '\"';
					} break;
					case '\\': {
						*d++ = '\\';
					} break;
					case 'b': {
						*d++ = '\b';
					} break;
					case 'f': {
						*d++ = '\f';
					} break;
					case 'n': {
						*d++ = '\n';
					} break;
					case 'r': {
						*d++ = '\r';
					} break;
					case 't': {
						*d++ = '\t';
					} break;
					default: {
						JSON_REPORT_ERROR("Unknown escape sequence", p);
						return 0; // error of some kind
					}
				}
			} else {
				*d++ = *s;
			}
		}
		str[len] = '\0';
		*key = str;
		return ++p;
	}

	JSON_REPORT_ERROR("Neverending Quoted String", p);
	return 0;
}


static char *jsonez_parse_raw_key(char **key, char *p) {

	char *s = p;
	int len = 0;

	while(*p && JSONEZ_RAW_KEY(*p)) {
		p++;
		len++;
	}

	p = jsonez_skip_whitespace(p);

	if(*p == ':' || *p == '=') {
		// got a key
		char *str = (char *)calloc(len+1, sizeof(char));		
		strncpy(str, s, len);
		str[len] = '\0';
		*key = str;
		return p;
	} 
	
	JSON_REPORT_ERROR("Missing ':' key separator", p);
	return 0;
}


static char *jsonez_parse_bool_value(jsonez *parent, char *key, char *p) {

	jsonez *json = jsonez_create(parent, key);

	char c=*p++;
	if(c=='t') {
		if((c=*p++) && c=='r') {
			if((c=*p++) && c=='u') {
				if((c=*p++) && c=='e') {
					json->type = JSON_BOOL;
					json->i = 1;
					return p;
				}
			}
		}
	} else if(c=='f') {
		if((c=*p++) && c=='a') {
			if((c=*p++) && c=='l') {
				if((c=*p++) && c=='s') {
					if((c=*p++) && c=='e') {
						json->type = JSON_BOOL;
						json->i = 0;
						return p;
					}
				}
			}
		}
	}

	JSON_REPORT_ERROR("Unknow Value", p);
	return 0;

}


static char *jsonez_parse_number_value(jsonez *parent, char *key, char *p) {

	jsonez *json = jsonez_create(parent, key);
	char* s = p;	
	char* e = 0;

	// find the first not a number
	while(JSONEZ_NUMBER(*p)) {
		e = p++;	
	}

	// try parse int
	errno = 0;
	char *ee;
	int i = strtol(s, &ee, 10);
	if( errno == 0 && ((e+1) == ee) ) {
		json->type = JSON_NUMBER;
		json->n = i;
		return p;
	}

	// try to parse double
	errno = 0;
	ee = 0;
	double n = strtod(s, &ee);
	if(errno == 0 && ((e+1) == ee) ) {
		json->type = JSON_NUMBER;
		json->n = n;
		return p;
	}

	JSON_REPORT_ERROR("Invalid Number Format", p);
	return 0;

}


static char *jsonez_parse_string_value(jsonez *parent, char *key, char *p) {

	jsonez *json = jsonez_create(parent, key);
	p = jsonez_parse_quote_string(&json->s, p);
	json->type = JSON_STRING;
	if(p) {
		return p;
	}

	return 0; // TODO - error

}


static char *jsonez_parse_array(jsonez *parent, char *p) {

	p = jsonez_skip_whitespace(p);

	while(*p) {

		if(*p=='t'||*p=='f') {
			p = jsonez_parse_bool_value(parent, parent->key, p);
		} else if(*p=='"') {
			p = jsonez_parse_string_value(parent, parent->key, p);
		} else if(JSONEZ_NUMBER(*p)) {
			p = jsonez_parse_number_value(parent, parent->key, p);
		} else if(*p=='{') {
			p++;
			jsonez* child = jsonez_create(parent, parent->key);
			p = jsonez_parse_object(child, p);
		} else if(*p=='[') {
			p++;
			jsonez* child = jsonez_create(parent, parent->key);
			p = jsonez_parse_array(child, p);
		}

		p = jsonez_next_arr(p);
		if(*p==']') {
			parent->type = JSON_ARRAY;
			return p+1;
		}

		if(!p) return 0; // error?!?
	}

	JSON_REPORT_ERROR("Syntax Error", p);
	return 0; // error of some kind

}


static char *jsonez_parse_object(jsonez *parent, char *p) {

	p = jsonez_skip_whitespace(p);

	while(*p) {

		if(*p=='}') {
			parent->type = JSON_OBJ;
			return p+1;
		}

		char *key = 0;
		if(JSONEZ_RAW_KEY(*p)) {
			p = jsonez_parse_raw_key(&key, p);
		} else if(*p=='"') {
			p = jsonez_parse_quote_string(&key, p);	
		}

		if(!p) {
			return 0; // some kind of error
		}

		p = jsonez_skip_key_separator(p);
			
		if(*p=='t'||*p=='f') {
			p = jsonez_parse_bool_value(parent, key, p);
		} else if(*p=='"') {
			p = jsonez_parse_string_value(parent, key, p);
		} else if(JSONEZ_NUMBER(*p)) {
			p = jsonez_parse_number_value(parent, key, p);
		} else if(*p=='{') {
			p++;
			jsonez* child = jsonez_create(parent, key);
			p = jsonez_parse_object(child, p);
		} else if(*p=='[') {
			p++;
			jsonez* child = jsonez_create(parent, key);
			p = jsonez_parse_array(child, p);
		}

		free(key);
		p = jsonez_next_obj(p);
		if(!p) return 0; // error?!?
	}

	JSON_REPORT_ERROR("Syntax Error", p);
	return 0; // error of some kind

}


static char *json_parse_root(jsonez *parent, char *p) {

	p = jsonez_skip_whitespace(p);

	while(*p) {

		char *key = 0;
		if(JSONEZ_RAW_KEY(*p)) {
			p = jsonez_parse_raw_key(&key, p);
		} else if(*p=='"') {
			p = jsonez_parse_quote_string(&key, p);	
		}

		if(!p) {
			return 0; // some kind of error
		}

		p = jsonez_skip_key_separator(p);
		
		if(*p=='t'||*p=='f') {
			p = jsonez_parse_bool_value(parent, key, p);
		} else if(*p=='"') {
			p = jsonez_parse_string_value(parent, key, p);
		} else if(JSONEZ_NUMBER(*p)) {
			p = jsonez_parse_number_value(parent, key, p);
		} else if(*p=='{') {
			p++;
			jsonez* child = jsonez_create(parent, key);
			p = jsonez_parse_object(child, p);
		} else if(*p=='[') {
			p++;
			jsonez* child = jsonez_create(parent, key);
			p = jsonez_parse_array(child, p);
		}

		free(key);
		p = jsonez_next_obj(p);
		if(*p=='\0') {
			parent->type = JSON_OBJ;
			return p;
		}

	}

	JSON_REPORT_ERROR("Syntax Error", p);
	//@TODO - still need to free everything on error.
	return 0; // error of some kind

}


JSONEZDEF void jsonez_free(jsonez *json) {

	if (json) {

		free(json->key);
		if (json->type == JSON_STRING) {
			free(json->s);
		}

		if (json->child) {
			jsonez_free(json->child);
		}

		if (json->next) {
			jsonez_free(json->next);
		}
		free (json);
	}

}


JSONEZDEF jsonez *jsonez_parse(char *file) {

	jsonez *json = (jsonez*)calloc(1, sizeof(jsonez));

	char *p = file;
	if (p == 0 || strlen(p) == 0) {
		json->type = JSON_OBJ;
		return json;
	}
	
	
	p = jsonez_skip_whitespace(p);


	if(*p=='{') {
		p = jsonez_parse_object(json, p+1);
	} else {
		p = json_parse_root(json, p);	
	}

	
	p = jsonez_skip_whitespace(p);

	if (!p) {
		JSON_REPORT_ERROR("Unexpected end of file", p);
	}

	json->type = JSON_OBJ;
	return json;

}


JSONEZDEF jsonez *jsonez_find(jsonez *parent, const char *key) {

	if (parent == NULL)
		return NULL;

	jsonez *next = parent->child;
	while(next) {
		if(!strcmp(key, next->key)) {
			return next;
		}
		next = next->next;
	}
	return NULL;

}


JSONEZDEF jsonez *jsonez_create_root() {

	jsonez *obj = (jsonez *)calloc(1, sizeof(jsonez));
	obj->type = JSON_OBJ;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_object(jsonez *parent, char *key) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_OBJ;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_array(jsonez *parent, char *key) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_ARRAY;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_bool(jsonez *parent, char *key, bool value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_BOOL;
	obj->i = value;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_numd(jsonez *parent, char *key, double value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_NUMBER;
	obj->n = value;
	return obj;

}

JSONEZDEF jsonez *jsonez_create_numf(jsonez *parent, char *key, float value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_NUMBER;
	obj->n = value;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_numi(jsonez *parent, char *key, int value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_NUMBER;
	obj->n = value;
	return obj;

}


JSONEZDEF jsonez *jsonez_create_string(jsonez *parent, char *key, char *value) {

	// this needs to unescape the string because
	// reading the strings in escapes them
	// why is this so hard?
	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_STRING;
	// count neede chars with escaping
	int size = 0;
	char *p = value;
	while(p && *p) {
		size++;
		switch(*p) {
			case '"':
			case '\\':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
				size++;
				break;
		}	
		p++;
	}

	obj->s = (char *)calloc(size + 1, sizeof(char));
	p = value;
	char *dest = obj->s;
	while (p && *p) {
		switch(*p) {
			case '"':
			case '\\':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
				*dest++ = '\\';
				break;
		}	
		*dest++ = *p++;
	}

	obj->s[size] = '\0';
	return obj;

}


static void jsonez_print_array_values(jsonez_output *out, int space, jsonez *obj, jsonez_ctx *ctx) {

	JSONEZ_WRITE_STRING(out, " [");
	jsonez *arr_obj = obj->child;
	if (arr_obj) {
		jsonez_print_value(out, space, arr_obj, ctx);
		while(arr_obj->next) {
			arr_obj = arr_obj->next;
			JSONEZ_WRITE_STRING(out, ", ");
			jsonez_print_value(out, space, arr_obj, ctx);
		}
	}
	JSONEZ_WRITE_STRING(out, "]");
}


static void jsonez_print_object_values(jsonez_output *out, int space, jsonez *obj, jsonez_ctx *ctx) {
	JSONEZ_WRITE_STRING(out, "{\n");
	if (obj) {
		jsonez *child = obj->child;
		if (child) {
			jsonez_print_key_value(out, space + ctx->indent_length, child, ctx);
			while(child->next) {
				JSONEZ_WRITE_STRING(out, ",\n");
				child = child->next;
				jsonez_print_key_value(out, space + ctx->indent_length, child, ctx);
			}
		}
	}
	JSONEZ_WRITE_STRING(out, "\n%*s}", space, "");
}


static bool jsonez_is_key_raw(char *key) {
	while(key && *key) {
		if (!JSONEZ_RAW_KEY(*key)) {
			return false;
		}
		key++;
	}
	return true;
}


static void jsonez_write_key_value(jsonez_output *out, int space, jsonez *obj, jsonez_ctx *ctx) {
	const char *separator = ctx->use_equal_sign ? " = " : ": ";
	if (ctx->quote_keys || !jsonez_is_key_raw(obj->key)) {
		JSONEZ_WRITE_STRING(out, "%*s\"%s\"%s", space, "", obj->key, separator);
	} else {
		JSONEZ_WRITE_STRING(out, "%*s%s%s", space, "", obj->key, separator);
	}
}



//@TODO: something better than this
static void jsonez_print_error(jsonez *obj) {
	printf("ERROR!!!\n");
}


static void jsonez_print_key_value(jsonez_output *out, int space, jsonez *obj, jsonez_ctx *ctx) {
	if(obj) {
		switch(obj->type) {
			case JSON_NUMBER: {
				jsonez_write_key_value(out, space, obj, ctx);
				JSONEZ_WRITE_STRING(out, "%f", obj->n); 
		   } break;
			case JSON_STRING:{
				jsonez_write_key_value(out, space, obj, ctx);
		   	JSONEZ_WRITE_STRING(out, "\"%s\"", obj->s);
			} break;
			case JSON_BOOL: {
				jsonez_write_key_value(out, space, obj, ctx);
				JSONEZ_WRITE_STRING(out, "%s", obj->i ? "true" : "false"); 
			} break;
			case JSON_ARRAY: {
				jsonez_write_key_value(out, space, obj, ctx);
				jsonez_print_array_values(out, space, obj, ctx);
		   } break;
			case JSON_OBJ: {
				jsonez_write_key_value(out, space, obj, ctx);
			  	jsonez_print_object_values(out, space, obj, ctx);
		   } break;
			default: jsonez_print_error(obj); return;
		}
	}
}


static void jsonez_print_value(jsonez_output *out, int space, jsonez *value, jsonez_ctx *ctx) {
	if(value) {
		switch(value->type) {
			case JSON_NUMBER: JSONEZ_WRITE_STRING(out, "%f", value->n); break;
			case JSON_STRING: JSONEZ_WRITE_STRING(out, "\"%s\"", value->s); break;
			case JSON_BOOL: JSONEZ_WRITE_STRING(out, "%s", value->i ? "true" : "false"); break;
			case JSON_ARRAY: jsonez_print_array_values(out, space, value, ctx); break;
			case JSON_OBJ: jsonez_print_object_values(out, space, value, ctx); break;
			default: jsonez_print_error(value); return;
		}
	}
}


static void jsonez_root_to_string(jsonez_output *out, jsonez *root, jsonez_ctx *ctx) {
	jsonez *start = root->child;
	if (ctx->add_root_object) JSONEZ_WRITE_STRING(out, "{\n");
	if (start) {
		int indent = ctx->add_root_object ? ctx->indent_length : 0;
		jsonez_print_key_value(out, indent, start, ctx);
		while(start->next) {
			JSONEZ_WRITE_STRING(out, ",\n");
			start = start->next;
			jsonez_print_key_value(out, indent, start, ctx);
		}
	}
	if (ctx->add_root_object) JSONEZ_WRITE_STRING(out, "\n}\n");
	else JSONEZ_WRITE_STRING(out, "\n");
}



JSONEZDEF char *jsonez_to_string(jsonez *root, jsonez_ctx *ctx) {

	jsonez_ctx default_ctx;
	if (ctx == NULL) {
		default_ctx.indent_length = 3;
		default_ctx.add_root_object = true;
		default_ctx.quote_keys = true;
		default_ctx.use_equal_sign = false;
		ctx = &default_ctx;
	}

	jsonez_output out;
	out.total = 0;
	out.ptr = NULL;
	out.remaining = 0;

	jsonez_root_to_string(&out, root, ctx);
	char *string = (char *)calloc(out.total + 1, sizeof(char));

	out.remaining = out.total;
	out.total = 0;
	out.ptr = string;
	jsonez_root_to_string(&out, root, ctx);

	string[out.total] = '\0';
	return string;

}


JSONEZDEF const char *jsonez_type_to_string(jsonez *obj) {

	if (obj == nullptr) return "NULL_OBJ";

	switch (obj->type) {
		case JSON_OBJ: return "JSON_OBJ"; break;
		case JSON_ARRAY: return "JSON_ARRAY"; break;
		case JSON_STRING: return "JSON_STRING"; break;
		case JSON_NUMBER: return "JSON_NUMBER"; break;
		case JSON_BOOL: return "JSON_BOOL"; break;
		default: return "UNKNOWN_TYPE";
	}
}


JSONEZDEF void jsonez_free_string(char *string) {
	free(string);
}


#endif // JSONEZ_IMPLEMENTATION

/*

	revision history:
		0.21 (2018-04-06) Bunches of changes
		 - An empty file returns and empty node, not null
		 - Changed float/int to just number
		 - Passing a null pointer value as a string makes an empty string
	   0.20 (2017-07-07)	Adding creation and output
		0.10 (2017-03-14)	Initial Release	

	Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
	software, either in source code form or as a compiled binary, for any purpose, 
	commercial or non-commercial, and by any means.

	In jurisdictions that recognize copyright laws, the author or authors of this 
	software dedicate any and all copyright interest in the software to the public 
	domain. We make this dedication for the benefit of the public at large and to 
	the detriment of our heirs and successors. We intend this dedication to be an 
	overt act of relinquishment in perpetuity of all present and future rights to 
	this software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */
