
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n",\
        __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr,\
        "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__,\
        clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr,\
        "[WARN] (%s:%d: errno: %s) " M "\n",\
        __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n",\
        __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) {\
    log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__);\
    errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__);\
    errno=0; goto error; }

#define mu_suite_start() const char *message = NULL

#define mu_assert(test, message) if (!(test)) { log_err(message); return message; }
#define mu_run_test(test) debug("\n-----%s", " " #test); \
    message = test(); tests_run++; if (message) return message;

#define RUN_TESTS(name) int main(int argc, char *argv[]) {\
    argc = 1; \
    debug("----- RUNNING: %s", argv[0]);\
    printf("----\nRUNNING: %s\n", argv[0]);\
    const char *result = name();\
    if (result != 0) {\
        printf("FAILED: %s\n", result);\
    }\
    else {\
        printf("ALL TESTS PASSED\n");\
    }\
    printf("Tests run: %d\n", tests_run);\
    exit(result != 0);\
}

static int tests_run;

#define JSONEZ_IMPLEMENTATION
#include "../jsonez.h"


////////////////////////////////////////////////////////////////////////////////
// Testing stuff
////////////////////////////////////////////////////////////////////////////////

// Test finding stuff
// test escaped string for keys and values

const char *test_parse_011() {

	const char* file = R"(
		/* multil
		 * 
		 * oh yeah babh //
		 *  /
		k0 = { 
		}, // comment
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json == NULL, "Should get nothing back");

	return NULL;


}
 
const char *test_parse_010() {

	const char* file = R"(
		/* multil
 * 
 * oh yeah babh //
 */
		// comment // comment
		k0 = { // comment
			k1 = { // comment
				n="v" // comment
			}, // comment
			k2= [{ // comment
				s=42 // comment
			},{ // comment
				p=false, // comment
			}], // comment
			v /*whas...*/:123, /* asdf */
		}, // comment
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "Should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be an object");

	return NULL;


}


const char* test_parse_009() {
	const char* file = R"(
		k0 = {
			k1 = {
				n="v"
			},
			k2= [{
				s=42
			},{
				p=false,
			}]
		},
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "Should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be an object");

	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(child->type == JSON_OBJ, "Should be an object");
	mu_assert(!strcmp(child->key,"k0"), " wrong key name?");
	mu_assert(child->next == 0, "should only be one");
	mu_assert(child->i == 2, "should only be two");

	child = child->child;
	mu_assert(child, "Should have a child");
	mu_assert(child->type == JSON_OBJ, "Should be an object");
	mu_assert(!strcmp(child->key,"k1"), " wrong key name?");
	mu_assert(child->next, "should be another one");
	mu_assert(child->i == 1, "should only be one");

	jsonez* tmp = child->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_STRING, "Should be an string");
	mu_assert(!strcmp(tmp->key,"n"), " wrong key name?");
	mu_assert(!strcmp(tmp->s,"v"), " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	jsonez* next = child->next;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_ARRAY, "Should be an array");
	mu_assert(!strcmp(next->key,"k2"), " wrong key name?");
	mu_assert(!next->next, "should not be another one");
	mu_assert(next->i == 2, "array has two items");

	next = next->child;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_OBJ, "Should be an array");
	mu_assert(next->next, "should be another one");
	mu_assert(next->i == 1, "array has one items");

	tmp = next->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_INT, "Should be an string");
	mu_assert(!strcmp(tmp->key,"s"), " wrong key name?");
	mu_assert(tmp->i == 42, " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	next = next->next;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_OBJ, "Should be an array");
	mu_assert(!next->next, "should be the last one");
	mu_assert(next->i == 1, "array has one items");

	tmp = next->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_BOOL, "Should be an string");
	mu_assert(!strcmp(tmp->key,"p"), " wrong key name?");
	mu_assert(tmp->i == 0, " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	return NULL;

}

const char* test_parse_008() {

	const char* file = R"(
		k0: {
			k1: {
				n:"v"
			},
			k2: [{
				s:42
			},{
				p:false,
			}]
		},
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "Should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be an object");

	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(child->type == JSON_OBJ, "Should be an object");
	mu_assert(!strcmp(child->key,"k0"), " wrong key name?");
	mu_assert(child->next == 0, "should only be one");
	mu_assert(child->i == 2, "should only be two");

	child = child->child;
	mu_assert(child, "Should have a child");
	mu_assert(child->type == JSON_OBJ, "Should be an object");
	mu_assert(!strcmp(child->key,"k1"), " wrong key name?");
	mu_assert(child->next, "should be another one");
	mu_assert(child->i == 1, "should only be one");

	jsonez* tmp = child->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_STRING, "Should be an string");
	mu_assert(!strcmp(tmp->key,"n"), " wrong key name?");
	mu_assert(!strcmp(tmp->s,"v"), " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	jsonez* next = child->next;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_ARRAY, "Should be an array");
	mu_assert(!strcmp(next->key,"k2"), " wrong key name?");
	mu_assert(!next->next, "should not be another one");
	mu_assert(next->i == 2, "array has two items");

	next = next->child;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_OBJ, "Should be an array");
	mu_assert(next->next, "should be another one");
	mu_assert(next->i == 1, "array has one items");

	tmp = next->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_INT, "Should be an string");
	mu_assert(!strcmp(tmp->key,"s"), " wrong key name?");
	mu_assert(tmp->i == 42, " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	next = next->next;
	mu_assert(next, "Should have a child");
	mu_assert(next->type == JSON_OBJ, "Should be an array");
	mu_assert(!next->next, "should be the last one");
	mu_assert(next->i == 1, "array has one items");

	tmp = next->child;
	mu_assert(tmp, "Should have a child");
	mu_assert(tmp->type == JSON_BOOL, "Should be an string");
	mu_assert(!strcmp(tmp->key,"p"), " wrong key name?");
	mu_assert(tmp->i == 0, " wrong value");
	mu_assert(!tmp->next, "should not be another one");

	return NULL;

}

const char* test_parse_007() {

	const char* file = R"(k0:[{s:42}])";

	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json != 0, "should get back something");
	mu_assert(json->type == JSON_OBJ, "?");

	return NULL;

}

const char* test_parse_006() {

	const char* file = R"(key:"value")";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json->type == JSON_OBJ, "should be an object");
	mu_assert(json->i == 1, "should have only one");
	jsonez* child = json->child;
	mu_assert(child, "should have a child");
	mu_assert(child->type == JSON_STRING, "should be a stringz");
	mu_assert(!strcmp(child->s, "value"), "should be value");

	return NULL;
}

const char* test_parse_005() {

	const char* file = R"(
		key:[]
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "WTF, over");

	return NULL;

}

const char* test_parse_004() {

	const char* file = R"(
		key0: "value",
		key1: 42,
		key2: 42.42,
		key3: true,
		key4: false,	
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be object");
	mu_assert(json->key == 0, "Key should be empty");
	mu_assert(json->i == 5, "Should have five things");
	
	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key0"), "wrong key");
	mu_assert(child->type == JSON_STRING, "Wrong type");
	mu_assert(!strcmp(child->s, "value"), "Wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key1"), "wrong key");
	mu_assert(child->type == JSON_INT, "Wrong type");
	mu_assert(child->i == 42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key2"), "wrong key");
	mu_assert(child->type == JSON_FLOAT, "Wrong type");
	mu_assert(child->d == 42.42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key3"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 1, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key4"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 0, "wrong value");

	mu_assert(child->next == 0, "Too many children");

	return NULL;

}

const char* test_parse_003() {

	const char* file = R"(
		"key0": "value",
		"key1": 42,
		"key2": 42.42,
		"key3": true,
		"key4": false,	
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be object");
	mu_assert(json->key == 0, "Key should be empty");
	mu_assert(json->i == 5, "Should have five things");
	
	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key0"), "wrong key");
	mu_assert(child->type == JSON_STRING, "Wrong type");
	mu_assert(!strcmp(child->s, "value"), "Wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key1"), "wrong key");
	mu_assert(child->type == JSON_INT, "Wrong type");
	mu_assert(child->i == 42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key2"), "wrong key");
	mu_assert(child->type == JSON_FLOAT, "Wrong type");
	mu_assert(child->d == 42.42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key3"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 1, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key4"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 0, "wrong value");

	mu_assert(child->next == 0, "Too many children");

	return NULL;

}

const char* test_parse_002() {

	const char* file = R"(
		{
			"key0": "value",
			"key1": 42,
			"key2": 42.42,
			"key3": true,
			"key4": false,	
		}		    
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be object");
	mu_assert(json->key == 0, "Key should be empty");
	mu_assert(json->i == 5, "Should have five things");
	
	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key0"), "wrong key");
	mu_assert(child->type == JSON_STRING, "Wrong type");
	mu_assert(!strcmp(child->s, "value"), "Wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key1"), "wrong key");
	mu_assert(child->type == JSON_INT, "Wrong type");
	mu_assert(child->i == 42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key2"), "wrong key");
	mu_assert(child->type == JSON_FLOAT, "Wrong type");
	mu_assert(child->d == 42.42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key3"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 1, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key4"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 0, "wrong value");

	mu_assert(child->next == 0, "Too many children");

	return NULL;

}

const char* test_parse_001() {

	const char* file = R"(
		{
			key0: "value",
			key1: 42,
			key2: 42.42,
			key3: true,
			key4: false,	
		}		    
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json, "should get something back");
	mu_assert(json->type == JSON_OBJ, "Should be object");
	mu_assert(json->key == 0, "Key should be empty");
	mu_assert(json->i == 5, "Should have five things");
	
	jsonez* child = json->child;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key0"), "wrong key");
	mu_assert(child->type == JSON_STRING, "Wrong type");
	mu_assert(!strcmp(child->s, "value"), "Wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key1"), "wrong key");
	mu_assert(child->type == JSON_INT, "Wrong type");
	mu_assert(child->i == 42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key2"), "wrong key");
	mu_assert(child->type == JSON_FLOAT, "Wrong type");
	mu_assert(child->d == 42.42, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key3"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 1, "wrong value");

	child = child->next;
	mu_assert(child, "Should have a child");
	mu_assert(!strcmp(child->key,"key4"), "wrong key");
	mu_assert(child->type == JSON_BOOL, "Wrong type");
	mu_assert(child->i == 0, "wrong value");

	mu_assert(child->next == 0, "Too many children");

	return NULL;

}

const char* test_parse_empty_string() {

	const char* file = R"(
		    
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

   jsonez* json = jsonez_parse(p);
	mu_assert(!json, "didn't work");
	//mu_assert(json->type == JSON_OBJ, "Isn't an object");
	//mu_assert(json->i == 0, "Not empty?");

	return NULL;

}

const char* test_parse_empty_obj() {

	const char* file = R"(
		{  }
	)";
	size_t len = strlen(file);
	char* buf = (char*)calloc(len+1, sizeof(char));
	strncpy(buf, file, len);
	buf[len] = '\0';
	char* p = buf;

	jsonez* json = jsonez_parse(p);
	mu_assert(json->type == JSON_OBJ, "Wrong type");
	mu_assert(json->i == 0, "should not have children");
	mu_assert(!json->key, "key should be empty");

	return NULL;

}

const char* test_single_obj() {
	const char* file = R"(
		{ "key":"value" }
	)";
	char* buf = (char*)calloc(strlen(file) + 1, sizeof(char));
	strncpy(buf, file, strlen(file));
	jsonez* json = jsonez_parse(buf);
	mu_assert(json, "Failed to parse.");
	mu_assert(json->type == JSON_OBJ, "Wrong type");
	jsonez* child = json->child;
	mu_assert(child, "Didn't have a child");
	mu_assert(child->type == JSON_STRING, "Wrong type");
	mu_assert(!strcmp(child->key, "key"), "Wrong key");
	mu_assert(!strcmp(child->s, "value"), "Wrong value");
	return NULL;
}

const char* test_create() {
	const char* file = R"(
		{ }
	)";
	char* buf = (char*)calloc(strlen(file) + 1, sizeof(char));
	strncpy(buf, file, strlen(file));
	jsonez* json = jsonez_parse(buf);
	mu_assert(json, "Failed to parse.");
	mu_assert(json->type == JSON_OBJ, "Wrong type");
	mu_assert(json->next == NULL, "shouldn't have anything else");
	return NULL;
}

// TODO
// add to object
// add to array


jsonez *jsonez_create_object(jsonez *parent, char *key) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_OBJ;
	return obj;

}


jsonez *jsonez_create_array(jsonez *parent, char *key) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_ARRAY;
	return obj;

}


jsonez *jsonez_create_bool(jsonez *parent, char *key, bool value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_BOOL;
	obj->i = value;
	return obj;

}


jsonez *jsonez_create_float(jsonez *parent, char *key, double value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_FLOAT;
	obj->d = value;
	return obj;

}


jsonez *jsonez_create_int(jsonez *parent, char *key, int value) {

	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_INT;
	obj->i = value;
	return obj;

}


jsonez *jsonez_create_string(jsonez *parent, char *key, char *value) {

	// this needs to unescape the string because
	// reading the strings in escapes them
	// why is this so hard?
	jsonez *obj = jsonez_create(parent, key);
	obj->type = JSON_STRING;
	// count neede chars with escaping
	int size = 0;
	char *p = value;
	while(*p) {
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
	while (*p) {
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


const char* testbox() {


	// got some ideas about serializing json to a string
	// a ctx object for stuff
	// ctx.quote_keys = false;
	// ctx.indent_spaces = 3;
	// ctx.color_separator = false; // true = ':', false - '='
	// ctx.root_object = true; // true = { } around json content
	
	// int32 jsonez_to_string(jsonez* json, char *buf = NULL, uint32 len = 0, jsonez_ctx *ctx = NULL);
	//
	// int32 result = jsonez_to_string(json); // just returns size of string buffer
	// int32 result = jsonez_to_string(json, buf, len); // used default context
	// int32 result = jsonez_to_string(json, buf, len, &ctx); // used custom context
	// if the len of the buf is not big enough, return size needed.
	// if (result > len) ERROR
	//
	//
	
#if 1

	// simple way
	jsonez *json = NULL; // create object

	unsigned int size = jsonez_to_string(json); // just get size needed
	char *string = (char *)malloc(size + 1);
	unsigned int wrote = jsonez_to_string(json, string, size);
	mu_assert(size == wrote, "No idea");
	string[size] = '\0';  

	// write string to file...

	free(string);

#endif

	return NULL;
}


const char* all_tests() {

	mu_suite_start();
	mu_run_test(testbox);

	//mu_run_test(test_parse_011);
	//mu_run_test(test_parse_010);
	//mu_run_test(test_parse_009);
	//mu_run_test(test_parse_008);
	//mu_run_test(test_parse_007);
	//mu_run_test(test_parse_006);
	//mu_run_test(test_parse_005);
	//mu_run_test(test_parse_004);
	//mu_run_test(test_parse_003);
	//mu_run_test(test_parse_002);
	//mu_run_test(test_parse_001);
	//mu_run_test(test_parse_empty_obj);
	//mu_run_test(test_parse_empty_string);
	//mu_run_test(test_create);
	//mu_run_test(test_single_obj);

	return NULL;
}

RUN_TESTS(all_tests);	
