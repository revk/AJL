// ==========================================================================
//
//                       Adrian's JSON Library - ajl.h
//
// ==========================================================================

    /*
       Copyright (C) 2020-2020  RevK and Andrews & Arnold Ltd

       This program is free software: you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation, either version 3 of the License, or
       (at your option) any later version.

       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with this program.  If not, see <http://www.gnu.org/licenses/>.
     */
#pragma once
#include <stdio.h>
#include <time.h>

// This code is intended to allow loading, saving, and manipulation of JSON objects
// The key data type, j_t, is a pointer to a control structure and represents a current "point"
// with a JSON object. It references a "value". This point could be the root object, any of the
// tagged values in an object, or a value in an array.
//
// The j_find, and j_add functions take a "path" field, which can simply be the name of a tag
// in an object, or can be a path, e.g. "fred.jim". The path can reference arrays, e.g. "fred[3].jim"
// Use "[]" to reference a new entry on the end of an array. For j_add, the path causes entries
// to be created to make the path, so parent objects, path, and array elements get created along the way,
// allowing a complete tree to be created just by using j_add from root with the full path of values.
//
// Error handling. The library tries to avoid errors.
// An internal malloc fail will abort with err() unless a NULL can be returned, e.g. j_create()
// internal consistency checks use assert(), which should, of course, not happen.
// Attempting to use a NULL as a j_t is silently ignored
// Attempt to find/get data that does not exist returns NULL, and length -1, etc
// Attempting to set a value should always work, converting parent values to objects/arrays, extending arrays, etc, as needed
// Other error cases are explained in comments below
//
// Where "path" is used, it can be a dot separated string and allows [<digits>] for array index
// Where "name" is used it is the string used as the name/tag for an object, even if that contains dots, etc.

typedef struct j_s *j_t;        // Point in JSON tree, i.e. a value

j_t j_create();                 // Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
void j_delete(j_t *);           // Delete this value (remove from parent object if not root) and all sub objects, NULLs the pointer
void j_free(j_t);               // Same as j_delete but where you don't care about zapping the pointer itself

void j_log(int debug, const char *who, const char *what, j_t a, j_t b); // Generate log files
char *j_formdata(j_t);          // Generate percent encoded name=value sequence from JSON object

// Moving around the tree, these return the j_t of the new point (or NULL if does not exist)
j_t j_root(const j_t);          // Return root point
j_t j_parent(const j_t);        // Parent of this point (NULL if this point is root)
j_t j_next(const j_t);          // Next in parent object or array, NULL if at end
j_t j_prev(const j_t);          // Previous in parent object or array, NULL if at start
j_t j_first(const j_t);         // First entry in an object or array (same as j_index with 0)
j_t j_find(const j_t, const char *path);        // Find object within this object by tag/path - NULL if any in path does not exist
j_t j_path(const j_t, const char *path);        // Find, or creates (as null) a path from the object, return the final part (a null value item if newly created)
j_t j_named(const j_t, const char *name);       // Get the named entry, if exists, from a parent object
j_t j_index(const j_t, int);    // Find specific point in an array, or object - NULL if not in the array

// Information about a point
const char *j_name(const j_t);  // The name/tag of this object in parent, if it is in a parent object, else NULL
int j_pos(const j_t);           // Position in parent array or object, -1 if this is the root object so not in another array/object
const char *j_val(const j_t);   // The value of this object as a string. NULL if not found. Note that a "null" string is a valid literal value
int j_len(const j_t);           // The length of this value (characters if string or number or literal), or number of entries if object or array

const char *j_get(const j_t, const char *path); // Find and get val using path, NULL for not found. Pass PATH null to read value of j_t entry
char j_test(const j_t, const char *path, char def);     // Return 0 if false, 1 if true, else def. If path null, then checks passed node
const char *j_get_not_null(const j_t, const char *path);        // Find and get val using path, NULL for not found or null. Pass PATH null to read value of j_t entry

// Convert/check
time_t j_timez(const char *t, int z);   // convert iso time to time_t (z means assume utc is not set)
#define j_time(t) j_timez(t,0)  // Normal XML time, assumes local if no time zone
#define j_time_utc(t) j_timez(t,1)      // Expects time to be UTC even with no Z suffix
// Coding conversion
extern const char JBASE16[];
extern const char JBASE32[];
extern const char JBASE64[];
char *j_baseN(size_t, const unsigned char *, size_t, char *, const char *, unsigned int);
#define j_base64(len,buf)     j_base64N(len,buf,((len)+2)/3*4+1,alloca(((len)+2)/3*4+1))
#define j_base64N(slen,src,dlen,dst) j_baseN(slen,src,dlen,dst,JBASE64,6)
#define j_base32(len,buf)     j_base32N(len,buf,((len)+4)/5*8+1,alloca(((len)+4)/5*8+1))
#define j_base32N(slen,src,dlen,dst) j_baseN(slen,src,dlen,dst,JBASE32,5)
#define j_base16(len,buf)     j_base16N(len,buf,(len)*2+1,alloca((len)*2+1))
#define j_base16N(slen,src,dlen,dst) j_baseN(slen,src,dlen,dst,JBASE16,4)
size_t j_based(char *src, char **buf, const char *alphabet, unsigned int bits);
#define j_base64d(src,dst) j_based(src,dst,JBASE64,6)
#define j_base32d(src,dst) j_based(src,dst,JBASE32,5)
#define j_base16d(src,dst) j_based(src,dst,JBASE16,4)
const char *j_number_ok(const char *n); // Checks if a valid JSON number, returns error description if not
const char *j_datetime_ok(const char *n);       // Checks if a valid datetime, returns error description if not
const char *j_literal_ok(const char *n);        // Checks if a valid JSON literal (true/false/null/number), returns error description if not

// Information about data type of this point
int j_isarray(const j_t);       // True if is an array
int j_isobject(const j_t);      // True if is an object
int j_isnull(const j_t);        // True if is null literal
int j_isbool(const j_t);        // True if is a Boolean literal
int j_istrue(const j_t);        // True if is true literal
int j_isnumber(const j_t);      // True if is a number
int j_isstring(const j_t);      // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)

// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char *j_read(const j_t, FILE *);        // Read object from open file
char *j_read_close(const j_t root, FILE * f);   // Read object and close file
char *j_read_file(const j_t, const char *filename);     // Read object from named file
char *j_read_mem(const j_t, const char *buffer);        // Read object from string in memory (NULL terminated)

// Output an object - note this allows output of a raw value, e.g. string or number, if point specified is not an object itself
// Returns NULL if all is well, else a malloc'd error string
char *j_write(const j_t, FILE *);
char *j_write_close(const j_t, FILE *); // Also closes file
char *j_write_pretty(const j_t, FILE *);        // Write with formatting, making for debug use
char *j_write_pretty_close(const j_t, FILE *);  // Write with formatting, making for debug use, closes file
char *j_write_file(const j_t, const char *filename);
char *j_write_mem(const j_t, char **buffer, size_t *len);       // Write to specified buffer/length (len can be NULL if not needed)

// These are low level functions, and not typically used on their own, see j_store/j_append later for more useful functions
j_t j_null(const j_t);          // Null this point
j_t j_string(const j_t, const char *);  // Make this point a string, and set value
j_t j_stringn(const j_t, const char *, size_t); // Make this point a string, and set value using a lengt
j_t j_stringf(const j_t, const char *fmt, ...); // Make this point a string, and set (formatted) value
j_t j_utc(const j_t, time_t);   // Make this point a string containing an ISO8601 UTC timestamp (ending Z)
j_t j_datetime(const j_t, time_t);      // Make this point a string containing an ISO8601 local timestamp (no Z)
j_t j_literalf(const j_t, const char *fmt, ...);        // Make this point a (formatted) literal, normally used for numeric values
j_t j_literal(const j_t, const char *); // Make this point a literal specified as a string (normally for "true" and "false")
j_t j_literal_free(const j_t, char *);  // Make this point a literal specified as a string which is then freed
j_t j_object(const j_t);        // Make this point an object (empty if it was not an object before)
j_t j_array(const j_t);         // Make this point an array (empty if it was not an array before)
j_t j_make(const j_t, const char *name);        // Find the named entry in an object, or make a new named entry if not found (null value)
j_t j_append(const j_t);        // Append a new point to an array (initially a null)

// Sort
typedef int j_sort_func(const void *a, const void *b);  // Allow sorting
void j_sort(const j_t);         // Apply a recursive sort so all objects have fields in alphabetic order.
void j_sort_f(const j_t, j_sort_func, int recurse);     // Apply a sort using a function (if recursive, only sorts objects), if not, then sorts object or array at specified point

// These functions allow values to be stored in an object - each takes a point, and the name of the value in that object
// If the point passed is not an object, it is changed to one and made empty, first
// If the name passed is NULL, then the corresponding j_append function is used instead
j_t j_store_object(const j_t, const char *name);        // Add a new named object to an object
j_t j_store_array(const j_t, const char *name); // Add a new named array to an object
j_t j_store_string(const j_t, const char *name, const char *);  // Add a named string to an object
j_t j_store_stringf(const j_t, const char *name, const char *fmt, ...); // Add a named (formatted) string to an object
j_t j_store_utc(const j_t, const char *name, time_t);   // Add a named ISO8601 UTC string to an object
j_t j_store_datetime(const j_t, const char *name, time_t);      // Add a named ISO8601 local time string to an object
j_t j_store_literal(const j_t, const char *name, const char *); // Add a named literal (usually "true" or "false") to an object
j_t j_store_literalf(const j_t, const char *name, const char *fmt, ...);        // Add a named (formatted) literal *usually a number) to an object
j_t j_store_literal_free(const j_t, const char *name, char *);  // Add a named literal to an object and free the passed string
j_t j_store_json(const j_t, const char *name, j_t *);   // Add a complete JSON entry, freeing and nulling second argument, returns first
j_t j_remove(const j_t, const char *name);      // Removed the named entry from an object

// Additional functions to combine the above... Returns point for newly added value.
// These are passed an array (or make what was passed to them in to an array)
j_t j_append_object(const j_t); // Append a new (empty) object to an array
j_t j_append_array(const j_t);  // Append a new (empty) array to an array
j_t j_append_string(const j_t, const char *);   // Append a new string to an array
j_t j_append_stringf(const j_t, const char *fmt, ...);  // Append a new (formatted) string to an array
j_t j_append_utc(const j_t, time_t);    // Append a new string formatted as ISO8601 UTC datetime to an array
j_t j_append_datetime(const j_t, time_t);       // Append a new string formatted as an ISO8601 local time to an array
j_t j_append_literal(const j_t, const char *);  // Append a new literal value (normally "true" or "false") to an array
j_t j_append_literalf(const j_t, const char *fmt, ...); // Append a new (formatted) literal (usually for a number) to an array
j_t j_append_literal_free(const j_t, char *);   // Append a new literal value to an array and free the passed string
j_t j_append_json(const j_t, j_t *);    // Append a complete JSON entry, freeing and nulling second argument, returns first

// Moving parts of objects...
j_t j_replace(const j_t, j_t *);        // Overwrites j in situ, returning j (unchanged), but the replacement object is freed and NULL'd
j_t j_detach(j_t j);            // Detach from parent so a to make a top level object in itself
