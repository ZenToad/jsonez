# jsonez
The easy JSON library for C/C++.

## How does it work?
This is a single header file library.  Just drop the file in, include it in your project, #define JSONEZ_IMPLEMENTATION in exactly ONE file, and you are good to go.

```
// i.e. it should look like this:
#include ...
#include ...
#define JSONEZ_IMPLEMENTATION
#include "jsonez.h"
```

## Who is this library for?
This is an "easy" library, so there are not a lot of features, there are zero classes and objects, and you probably don't want to use this in super-secret code because it favors simplicity over security.  If you need a full-featured robust, hardened business JSON parser for thousands of servers, you are in the wrong place.

However, if you have some C/C++ code, and you want to parse/save some JSON files, and you just want some code that works, you have come to the right place.

## Does this code follow the JSON language specification?
Hell no!  Why would you want to do that?  While I like the idea of JSON, the fact is that I'm too lazy to put all the keys in "quotes", I don't care about commas at the end of lists, I like my JSON to look like Javascript and/or Lua, and I really want comments in some of my config files.

Valid JSON is still valid:
```
{
  "key": "value",
  "table": {
    "array": [1,2,3]
  },
  "something else": false
}
```

But you can also leave out the surrounding curly braces if you are lazy.
```
"key": "value",
"table": {
  "array": [1,2,3]
},
"something else": false
```

It's okay to have a comma at the end of a list. (I won't tell):
```
"key": "value",
"table": {
  "array": [1,2,3,],
},
"something else": false,
```
If your keys have no spaces or special characters, you don't even need quotes.  You do always need quotes for string values however.
```
lazyAmI: "keys without quotes",
because: 42,
ofReasons: 1.234,
```
This parser lets you have single and multiple line comments, because sometimes you need them.
```
{
  // this is the root object
  root: {
    name: "bill", // name
    size: 12, // why 12?
    dead: false
  },
  /*
   * This ID really needs to be generated,
   * but we can do that later too.
   */
   id: 420,
}
```
And if you really don't like the colon separator you can use an equal sign:
```
{
  root = {
    name = "bill",
    size = 12,
    dead = false,
  },
  array = [{
    item = 1,
    id = 22,
  }, {
    item = 2,
    id = 33  
  }]
}
```
At this point, you may notice that this "JSON" parser really parses JSON, Javascript, and Lua.  (Shhhhh.  Don't tell
those fancy parsers.  They may get jealous).

## O.K. but how easy is it really?
Here is the code to parse a string once you have loaded it into your program:
```

char *json_file = load_your_file();
jsonzez *json = jsonez_parse(json_file);

// do your json stuff here

jsonez_free(json);
```
Really?  Yep.  That's it.
What if you have a JSON object and you want to write it to a file?
```
jsonez* your_object = create_your_object(); // we cover this later
char *string = jsonez_to_string(your_object);
write_to_file(string);
free(string);
jsonez_free(your_object);
```
