/* jsonez.h v0.10 - public domain easy json parser - github url
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

	Full license at bottom of file.	

	Latest revision history:
		0.10 (2017-MO-DAY)	Initial Release	

	See end of file for full revision history and license.

*/

#ifndef INCLUDE_JSONEZ_H
#define INCLUDE_JSONEZ_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef JSONEZ_STATIC
#define JSONEZDEF static
#else
#define JSONEZDEF extern
#endif

#ifndef JSON_REPORT_ERROR
#define JSON_REPORT_ERROR(msg, p) fprintf(stderr, "PARSE ERROR (%d): " msg " at %s\n", __LINE__, p)
#endif

enum jsonez_type {
	JSON_UNKNOWN,
	JSON_OBJ,
	JSON_ARRAY,
	JSON_STRING,
	JSON_INT,
	JSON_FLOAT,
	JSON_BOOL,
};


struct jsonez {
	jsonez_type type;
	char* key;
	union {
		char* s; // string 
		int i; // int, boolean, or count
		double d; // double 
	};
	jsonez* next;
	jsonez* child;
};


JSONEZDEF jsonez* jsonez_parse(char* file);
JSONEZDEF jsonez* jsonez_find(jsonez* parent, const char* key);
JSONEZDEF void jsonez_free(jsonez* json);


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


static char* jsonez_parse_object(jsonez* parent, char* p);

static jsonez* jsonez_create(jsonez*parent, char* key) {

	jsonez* json = (jsonez*)calloc(1, sizeof(jsonez));
	json->type = JSON_UNKNOWN;
	json->key = key;

	if(!parent->child) {
		parent->child = json;
	} else {
		jsonez* child = parent->child;
		while(child->next) {
			child = child->next;
		}
		child->next = json;
	}

	parent->i++;
	return json;
}

static char* jsonez_next_arr(char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p==',') {
		p++;
	}

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p==']') return p;
	if(*p=='{'||*p=='['||*p=='"'||*p=='t'||*p=='f'||JSONEZ_NUMBER(*p)) return p;

	JSON_REPORT_ERROR("Neverending Array bro!",p);
	return 0; // error of some king

}

static char* jsonez_next_obj(char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p=='\0') return p;
	if(*p=='}'||*p==']') return p;

	if(*p==',') {
		p++;
	} else {
		JSON_REPORT_ERROR("Next item missing",p);
		return 0; //error of some king
	}

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p=='\0') return p;
	if(*p=='}'||*p==']') return p;
	if(*p=='"'||*p=='t'||*p=='f'||JSONEZ_RAW_KEY(*p)) return p;

	JSON_REPORT_ERROR("Unexptected end of file",p);
	return 0; // error of some king

}

static char* jsonez_skip_key_separator(char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p==':') {
		p++;
	} else {
		JSON_REPORT_ERROR("Missing ':' key separator",p);
		return 0; // error of some kind
	}

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p=='"'||*p=='t'||*p=='f'||JSONEZ_NUMBER(*p)||*p=='['||*p=='{') return p;

	JSON_REPORT_ERROR("Unknow Value type",p);
	return 0; // error of some kind

}

static char* jsonez_parse_quote_string(char** key, char* p) {

	char* s = p;
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
		char* str = (char*)calloc(len+1, sizeof(char));
		char* d = str;
		for(int i = 0; i < len; ++i) {
			if(*++s == '\\') {
				switch(*++s) {
					case '"': {
						*d++ = '\"';
					} break;
					case '\\': {
						*d++ = '\\';
					} break;
					case '/': {
						*d++ = '/';
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

static char* jsonez_parse_raw_key(char** key, char* p) {

	char* s = p;
	int len = 0;

	while(*p && JSONEZ_RAW_KEY(*p)) {
		p++;
		len++;
	}

	JSONEZ_SKIP_WHITESPACE(p);

	if(*p == ':') {
		// got a key
		char* str = (char*)calloc(len+1, sizeof(char));		
		strncpy(str, s, len);
		str[len] = '\0';
		*key = str;
		return p;
	} 
	
	JSON_REPORT_ERROR("Missing ':' key separator", p);
	return 0;
}

static char* jsonez_parse_bool_value(jsonez* parent, char* key, char* p) {

	jsonez* json = jsonez_create(parent, key);

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

static char* jsonez_parse_number_value(jsonez* parent, char* key, char* p) {

	jsonez* json = jsonez_create(parent, key);
	char* s = p;	
	char* e = 0;

	// find the first not a number
	while(JSONEZ_NUMBER(*p)) {
		e = p++;	
	}

	// try parse int
	errno = 0;
	char* ee;
	int i = strtol(s, &ee, 10);
	if( errno == 0 && ((e+1) == ee) ) {
		json->type = JSON_INT;
		json->i = i;
		return p;
	}

	// try to parse double
	errno = 0;
	ee = 0;
	double d = strtod(s, &ee);
	if(errno == 0 && ((e+1) == ee) ) {
		json->type = JSON_FLOAT;
		json->d = d;
		return p;
	}

	JSON_REPORT_ERROR("Invalid Number Format", p);
	return 0;

}

static char* jsonez_parse_string_value(jsonez* parent, char* key, char* p) {

	jsonez* json = jsonez_create(parent, key);
	p = jsonez_parse_quote_string(&json->s, p);
	if(p) {
		json->type = JSON_STRING;
		return p;
	}

	return 0; // TODO - error

}

static char* jsonez_parse_array(jsonez* parent, char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

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

static char* jsonez_parse_object(jsonez* parent, char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

	while(*p) {

		if(*p=='}') {
			parent->type = JSON_OBJ;
			return p+1;
		}

		char* key = 0;
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

		p = jsonez_next_obj(p);
		if(!p) return 0; // error?!?
	}

	JSON_REPORT_ERROR("Syntax Error", p);
	return 0; // error of some kind

}

static char* json_parse_root(jsonez* parent, char* p) {

	JSONEZ_SKIP_WHITESPACE(p);

	while(*p) {

		char* key = 0;
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

		p = jsonez_next_obj(p);
		if(*p=='\0') {
			parent->type = JSON_OBJ;
			return p;
		}

	}

	JSON_REPORT_ERROR("Syntax Error", p);
	return 0; // error of some kind

}

JSONEZDEF void jsonez_free(jsonez* json) {
	jsonez* p = json->child;
	jsonez* p1;
	while(p) {
		p1 = p->next;
		jsonez_free(p);
		p = p1;
	}
	free(json);
}

JSONEZDEF jsonez* jsonez_parse(char* file) {

	jsonez* json = (jsonez*)calloc(1, sizeof(jsonez));
	
	char* p = file;
	
	JSONEZ_SKIP_WHITESPACE(p);

	if(*p=='{') {
		p = jsonez_parse_object(json, p+1);
	} else {
		p = json_parse_root(json, p);	
	}

	JSONEZ_SKIP_WHITESPACE(p);

	if(p && (*p == '\0')) {
		json->type = JSON_OBJ;
		return json;
	} else {
		JSON_REPORT_ERROR("Unexpected end of file", p);
		free(json);
		return 0;
	}

}

JSONEZDEF jsonez* jsonez_find(jsonez* parent, const char* key) {
	
	if( !parent ) return NULL;

	jsonez* next = parent->next;
	while(next) {
		if(!strcmp(key, next->key)) {
			return next;
		}
		next = next->next;
	}
	return NULL;

}



#endif // JSONEZ_IMPLEMENTATION

/*

	revision history:
		0.10 (2017-MO-DAY)	Initial Release	

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
