# Using AJL

There are actually two levels to the library, the `ajlparse`, and the `ajl` library. This manual covers the high level. It is not usually necessary to deal with the low level parsing library.

See the include file for full function prototypes, and comments.

`#include <ajl.h>`

Note this also defines an `extern char j_iso8601utc`. If this is set non zero then UTC ISO8601 format date/times are used.

## JSON type

The key data type used for all AJL operations is `j_t`. This is itself a pointer type, and memory is allocated for the data as needed. You need to ensure the usage of the type is freed.

The actual type can reference the whole JSON object, the tree, which is what you need to ensure is freed. But it can also be used to reference any *node* in the tree, and allow walking the tree and creating and changing the tree.

As such you should keep a root node variable from the `j_create()` and ensure you free (`j_delete()`) that when finished.

## Creating / deleting

You can create a new JSON object, type `j_t` using `j_create()`. This allocates an empty JSON object which can be used in various functions. In the event of an error a NULL is returned. AJL functions cope with NULL passed without crashing.

E.g. `j_t j=j_create();`

Once you have finished with a JSON object you need to free it. There are two ways to do this. The preferred way is `j_delete(j_t*)`. This frees the object and sets your variable to NULL. This helps avoid double free or use after free.

*Note: Yes, some modern gcc flags could help ensure that use after free is trapped at compile time, I know.*

The other call that can free a JSON object is `j_free(j_t)` which frees the data, but obviously does not overwrite your refrence to it. This is mainly intended for where some functin returns a `j_t` which you want to immediately free for some reason without assigning a variable.

## Navigation

You can navigate around a JSON object as needed, the `j_t` type points to a place within the object, with `j_create()` returning the root node.

The first argument (shown `j` here) is where we are now, and the return value is where we want to be. If the arg is already NULL then the return value is NULL. If the operation does not find a new point the return value is NULL.

|Function|Meaning|
|--------|-------|
|`j_root(j)`|The root of this JSON object|
|`j_parent(j)`|The parent|
|`j_next(j)`|The next sibling|
|`j_prev(j)`|The previous sibling|
|`j_first(j)`|The first child|
|`j_find(j,path)`|Find using a path|
|`j_path(j,path)`|Find, or create using a path - creeted object is a `null` value|
|`j_named(j,name)`|Get named entry, this is different to a `path` as the name could be anything including with `/` in it (bad idea)|
|`j_index(j,n)`|Get n'th child (starting 0 as first child)|

## Information

You can find information about an object `j_t`...

|Function|Meaning|
|--------|-------|
|`j_isarray(j)`|True if this an array|
|`j_isobject(j)`|True if this an object|
|`j_isnull(j)`|True if this a null|
|`j_isbool(j)`|True if this a Boolean|
|`j_istrue(j)`|True if this a Boolean value `true`|
|`j_isfalse(j)`|True if this a Boolean value `false`|
|`j_isnumber(j)`|True if this a number (int or real)|
|`j_isliteral(j)`|True if this a literal (including `null`, `true`, `false` and numbers)|
|`j_isstring(j)`|True if this a string|
|`j_name(j)`|The name of this point, NULL if not named (e.g. part of an array)|
|`j_pos(j)`|The index of this object in its parent|
|`j_val(j)`|The value of this object if a simple type, else NULL. Note that `null` is a simple type with the value `"null"` - see `j_get_not_null`|
|`j_len(j)`|If this is an object or array, how many entries. If a simple type, then how many characters in the value|
|`j_get(j,path)`|This combined `j_find` and `j_val`|
|`j_test(j,path,def)`|Does a `j_find`, and return 1 if this is `true`, 0 if `false`, and *def* if not Boolean|
|`j_get_not_null(j,path)`|Does a `j_find` and returns the value if it is not a `null`. NULL retrun for not found or `null`|
|`j_time(j)`|Returns a `time_t` if value is a suitably formatted date/time - assumes local time if no timezone suffix|
|`j_time_utc(j)`|As `j_time` but assumes UTC if no timezone suffix|

## Reading

The following functions fill in a JSON objec, use `j_create()` to make the object and pass to these functions.

|Function|Meaning|
|--------|-------|
|`j_read(j,FILE*)`|Read an open file, type `FILE*`|
|`j_read_fd(j,int)`|Read an open file by file descriptor|
|`j_read_close(j,FILE*)`|Read an open file, type `FILE*`, and close when read|
|`j_read_file(j,filename)`|Read a file from a filename|
|`j_read_mem(j,buf,len)`|Read from memory|

## Writing

The following functions output JSON. These all return NULL for OK, or a malloc'd error string.

|Function|Meaning|
|--------|-------|
|`j_write(j,FILE*)`|Write to an open file, type `FILE*`|
|`j_write_fd(j,int)`|Write to an open file by file descriptor|
|`j_write_func(j,find,arg)`|Write by calling a function for blocks of text written|
|`j_write_close(j,FILE*)`|Write to an open file, type `FILE*`, and close when written|
|`j_write_pretty(j,FILE*)`|Write to an open file, type `FILE*`, but with formatting white space for human reading|
|`j_write_file(j,filename)`|Create a file by filename and write to it|
|`j_write_mem(j,*buf,*len)`|Write to memory, malloced, address written to `buf` and size to `len`| 

## Changing/generating

Functions an be used to change a JSON object, and hence create one from scratch if you want.

### Low level functions

These low level functions are not usually needed, they typically change a value in situ.

|Function|Meaning|
|--------|-------|
|`j_null(j)`|Make this a `null`|
|`j_numberstring(j,val)`|Make this a number or string or `null` depending on val. I.e. if val looks like a valid number, make this a number.|
|`j_string(j,val)`|Make this a string|
|`j_stringn(j,val,len)`|Make this a string with specified lenght, i.e. where `val` is not null terminated|
|`j_stringf(j,fmt,...)`|Make this a string, `printf` style|
|`j_datetime(j,t)`|Make this a string with a string that is a datetime, see `j_iso8601utc`|
|`j_literal(j,val)`|Make this a literal|
|`j_literalf(j,fmt,...)`|Make this a literal, `printf` style|
|`j_literal_free(j,val)`|Make this a lietral, free the arg, useful where a function returns a malloc's string|
|`j_int(j,val)`|Make this an integer|
|`j_boolean(j,val)`|Make this a boolean (`true` if `val` is non zero)|
|`j_true(j)`|Make this `true`|
|`j_false(j)`|Make this `false`|
|`j_make(j,name)`|Make a named sub field (value is `null`)|
|`j_append(j)`|Make a new entry at end of an array (value is `null`)|
|`j_replace(j,*j)`|Replace this node with another JSON node.  - the JSON value object is unlinked from any parent and the variable holding it is NULL'd to avoid use afterwards|
|`j_detach(j)`|Detaches this node, and returns a new *root* node that is this JSON object|

### Manifulation functions

These create a node within a parent. The following are the `store` version, which create a named entry in an object parent. The same functions exists with `append` instead of `store` to append a new entry to an array, and so have no name. Using `store` with NULL is the same as `append`. The function returns the created node.

|Function|Meaning|
|--------|-------|
|`j_store_null(j,name)`|Create a named field that is `null`|
|`j_store_object(j,name)`|Create a named field that is an empty object|
|`j_store_array(j,name)`|Create a named field that is an emty array|
|`j_store_numberstring(j,name,val)`|Create a named field that is a number or string|
|`j_store_string(j,name,val)`|Create a named field that is a string|
|`j_store_stringn(j,name,val,len)`|Create a named field that is a string of specified length (i.e. when `val` is not null terminated)|
|`j_store_stringf(j,fmt,...)`|Create a named field that is a string, `printf` style|
|`j_store_datetime(j,t)`|Create a named field that is a string holding a datetime, see `j_iso8601utc`|
|`j_store_literal(j,name,val)`|Create a named field that is a literal|
|`j_store_literalf(j,fmt,...)`|Create a named field that is a literal, `printf` style|
|`j_store_literal_free(j,name,val)`|Create a named field that is a literal, and free argument|
|`j_store_int(j,name,val)`|Create a named field that is an integer|
|`j_store_bool(j,name,val)`|Create a named field that is a Boolean, `true` if `val` is not zero|
|`j_store_true(j,name)`|Create a named field that is a Boolean `true`|
|`j_store_false(j,name)`|Create a named field that is a Boolean `false`|
|`j_store_json(j,name,*j)`|Create a named field that is the specified JSON object - the JSON value object is unlinked from any parent and the variable holding it is NULL'd to avoid use afterwards|
|`j_remove(j,name)`|Remove a named field from parent|

## Tools

There are a number of useful tool functions.

|Function|Meaning|
|--------|-------|
|`j_string_ok(n,*end)`|Check if a string is a valid JSON string|
|`j_number_ok(n,*end)`|Check if a string is a valid JSON number - e.g. note that `-0` is not valid|
|`j_literal_ok(n,*end)`|Check if a string is a valid JSON literal|
|`j_datetime_ok(n,*end)`|Check if a string is a valid datetime|
|`j_base64(buf,len)`|Return alloc'd string of binary data base 64 coded|
|`j_base64a(buf,len)`|Return alloc'd string of binary data base 64 coded (same as `j_base64`)|
|`j_base64m(buf,len)`|Return malloc'd string of binary data base 64 coded|
|`j_base64N(slen,src,dlen,dst)`|Write string of binary data base 64 coded to specified location|
|`j_base64d(src,*dst)`|Convert base 64 to binary data, malloc'd to `dst`, return length|
|`j_base64d(dst,max,src)`|Convert base 64 to binary data, stored to `dst` up to `max` bytes, return length|
|32/16 versions|As above but using base 32 and base 16 instead of base 64|

## Misc functions

### Logging

Often JSON is used with external systems for APIs, and so logging is a common requirements. `j_log(int debug, const char *who, const char *what, j_t a, j_t b)`. This generates a log in `/var/log/json` with year, month, day subdirectories, for the `a` and `b` JSON objects (if not NULL). If `debug` is set it logs to `stderr` as well. The `who` and `what` are used as part of teh filename to allow logs to be found easily.

### Formdata

It is often useful to have a JSON turned in to HTTP style form data, two calls can do this, `j_formdata(j_t)` and `j_multipart(j_t,size_t*)`. These return a malloc'd string. The latter is formatted as a 

Both are mainly designed for a single top level JSON object with simple fields, and these make the form data tags. E.g. `{"a":1,"b":"test"}` would make `a=1&b=test` with proper escaping.

Where the JSON object is an array not an object, then this creates a list of values of the form `[0]=value1&[1]=value2` etc. If it is a simple type instead the ooutput is just the escaped value of the type which can be useful for forming queries in other ways.

For `j_formdata(j_t)` there is a special case where a value is infact itself an object with one field called `binary`. In this case the value is fully binary (percent) encoded. Otherwise any field that is not a simple type is expanded as the text version of the JSON.

For `j_multipart(j_t,size_t*)` there are extra processing options, the formdata post can handle a field that is an object with fields `type` (the mime type) and `filename` or `file` (the filename attribute). Where `file` is used the file is read from the local file system and its content is the field.

Other special cases for where the field content is an object may be added in future.

### Sort

`j_sort` will recursively sort all objects based on field names. `j_sort_f` allows a sort function to be passed.