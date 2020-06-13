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

typedef struct j_s *j_t;        // Point in JSON tree, i.e. a value

j_t j_create();                 // Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
j_t j_delete(j_t);              // Delete this value (remove from parent object if not root) and all sub objects, returns NULL

// Moving around the tree, these return the j_t of the new point (or NULL if does not exist)
j_t j_root(j_t);                // Return root point
j_t j_parent(j_t);              // Parent of this point (NULL if this point is root)
j_t j_next(j_t);                // Next in parent object or array, NULL if at end
j_t j_prev(j_t);                // Previous in parent object or array, NULL if at start
j_t j_first(j_t);               // First entry in an object or array (same as j_index with 0)
j_t j_find(j_t, const char *path);      // Find object within this object by tag/path - NULL if any in path does not exist
j_t j_index(j_t, int);          // Find specific point in an array, or object - NULL if not in the array

// Information about a point
const char *j_tag(j_t);         // The tag of this object in parent, if it is in a parent object, else NULL
int j_pos(j_t);                 // Position in parent array or object, -1 if this is the root object so not in another array/object
const char *j_val(j_t);         // The value of this object as a string. NULL if not found. Note that a "null" string is a valid literal value
int j_len(j_t);                 // The length of this value (characters if string or number or literal), or number of entries if object or array

const char *j_get(j_t, const char *path);       // Find and get val using path, NULL for not found
const char *j_get_not_null(j_t, const char *path);      // Find and get val using path, NULL for not found or null

// Convert
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

// Information about data type of this point
int j_isarray(j_t);             // True if is an array
int j_isobject(j_t);            // True if is an object
int j_isnull(j_t);              // True if is null literal
int j_isbool(j_t);              // True if is a Boolean literal
int j_istrue(j_t);              // True if is true literal
int j_isnumber(j_t);            // True if is a number
int j_isstring(j_t);            // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)

// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char *j_read(j_t, FILE *);      // Read object from open file
char *j_read_close(j_t root, FILE * f); // Read object and close file
char *j_read_file(j_t, const char *filename);   // Read object from named file
char *j_read_mem(j_t, const char *buffer);      // Read object from string in memory (NULL terminated)

// Output an object - note this allows output of a raw value, e.g. string or number, if point specified is not an object itself
// Returns NULL if all is well, else a malloc'd error string
char *j_write(j_t, FILE *);
char *j_write_close(j_t, FILE *);       // Also closes file
char *j_write_pretty(j_t, FILE *);      // Write with formatting, making for debug use
char *j_write_pretty_close(j_t, FILE *);        // Write with formatting, making for debug use, closes file
char *j_write_file(j_t, const char *filename);
char *j_write_mem(j_t, char **buffer, size_t *len);

// Changing an object/value
j_t j_null(j_t);                // Null this point
j_t j_string(j_t, const char *);        // Simple set this value to a string (null terminated).
j_t j_stringn(j_t, const char *, size_t);       // Simple set this value to a string with specified length (allows embedded nulls in string)
j_t j_stringf(j_t, const char *fmt, ...);       // Simple set this value to a string, using printf style format
j_t j_utc(j_t, time_t);         // Simple set (string) date/time (UTC)
j_t j_datetime(j_t, time_t);    // Simple set (string) date/time
j_t j_numberf(j_t, const char *fmt, ...);       // Simple set this value to a number, i.e. unquoted, using printf style format
j_t j_literal(j_t, const char *);       // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid.
j_t j_literal_free(j_t, char *);        // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid. (frees arg)
j_t j_object(j_t);              // Simple set this value to be an object if not already
j_t j_array(j_t);               // Simple set this value to be an array if not already
j_t j_add(j_t, const char *path);       // Create specified tag/path, and return the point that is the value for that tag.
j_t j_append(j_t);              // Create new point at end of array
void j_remove(j_t, const char *path);   // Remove an entry from its parent
typedef int j_sort_func(const void *a, const void *b);
void j_sort(j_t);               // Apply a recursive sort so all objects have fields in alphabetic order.
void j_sort_f(j_t, j_sort_func, int recurse);   // Apply a sort (if recursive, only sorts objects)

// Additional functions to combine the above... Returns point for newly added value.
// These insert a new tagged entry in to an object. The tag used is the last part of path. The parent is the j_t and and earlier parts of path
// E.g. if you j_add_string(x,"a.b.c","fred") you are creating object "a" in x, and "b" in "a", and then creating a string "c" in "b" with value "fred"
j_t j_add_array(j_t, const char *path); // Add array
j_t j_add_object(j_t, const char *path);        // Add object
j_t j_add_string(j_t, const char *path, const char *);  // Simple set this value to a string (null terminated).
j_t j_add_stringf(j_t, const char *path, const char *fmt, ...); // Simple set this value to a string, using printf style format
j_t j_add_utc(j_t, const char *path, time_t);   // Add date/time (UTC)
j_t j_add_datetime(j_t, const char *path, time_t);      // Add date/time
j_t j_add_numberf(j_t, const char *path, const char *fmt, ...); // Simple set this value to a number, i.e. unquoted, using printf style format
j_t j_add_literal(j_t, const char *path, const char *); // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid.
j_t j_add_literal_free(j_t, const char *path, char *);  // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid.

// Additional functions to combine the above... Returns point for newly added value.
// These append a new entry to an array. The array is made by j_t and a path. If j_t is the array to which you wish to append then path is NULL.
// E.g. if you j_append_string(x,"a.b.c","fred") you create object "a" in x, and "b" in "a", and then "c" in "b" which is an array, and add a string "fred" to array "c"
// Typically path will be NULL where you have the array in question already in the j_t argument
j_t j_append_object(j_t, const char *path);     // Add object
j_t j_append_array(j_t, const char *path);      // Add array
j_t j_append_string(j_t, const char *path, const char *);       // Simple set this value to a string (null terminated).
j_t j_append_stringf(j_t, const char *path, const char *fmt, ...);      // Simple set this value to a string, using printf style format
j_t j_append_utc(j_t, const char *path, time_t);        // Simple add date (UTC)
j_t j_append_datetime(j_t, const char *path, time_t);   // Simple add date
j_t j_append_numberf(j_t, const char *path, const char *fmt, ...);      // Simple set this value to a number, i.e. unquoted, using printf style format
j_t j_append_literal(j_t, const char *path, const char *);      // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid.
j_t j_append_literal_free(j_t, const char *path, char *);       // Simple set this value to a literal, e.g. "null", "true", "false", or a number known to be valid.

// Moving parts of objects...
j_t j_attach(j_t, j_t);         // Replaces j with o, unlinking o from its parent, returns o
