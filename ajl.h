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
// The j_find, and j_add functions take a "tags" field, which can simply be the name of a tag
// in an object, or can be a path, e.g. "fred.jim". The path can reference arrays, e.g. "fred[3].jim"
// Use "[+]" to reference a new entry on the end of an array. For j_add, the path causes entries
// to be created to make the path, so parent objects, tags, and array elements get created along the way,
// allowing a complete tree to be created just by using j_add from root with the full path of values.
//
// Error handling. The library tries to avoid errors.
// An internal malloc fail will abort with err() unless a NULL can be returned, e.g. j_create()
// internal consistency checks use assert(), which should, of course, not happen.
// Attempting to use a NULL as a j_t is silently ignored
// Attempt to find/get data that does not exist returns NULL, and length -1, etc
// Attempting to set a value should always work, converting parent values to objects/arrays, extending arrays, etc, as needed
// Other error cases are explained in comments below

typedef struct j_s *j_t;	// Point in JSON tree, i.e. a value

j_t j_create();	// Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
j_t j_delete(j_t); // Delete this value (remove from parent object if not root) and all sub objects, returns NULL

// Moving around the tree, these return the j_t of the new point (or NULL if does not exist)
j_t j_root(j_t);	// Return root point
j_t j_parent(j_t);	// Parent of this point (NULL if this point is root)
j_t j_next(j_t);	// Next in parent object or array, NULL if at end
j_t j_prev(j_t);	// Previous in parent object or array, NULL if at start
j_t j_first(j_t);	// First entry in an object or array (same as j_index with 0)
j_t j_find(j_t,const char *tags);	// Find object within this object by tag/path - NULL if any in path does not exist
j_t j_index(j_t,int);	// Find specific point in an array, or object - NULL if not in the array

// Information about a point
const char *j_tag(j_t); // The tag of this object in parent, if it is in a parent object, else NULL
int j_pos(j_t);	// Position in parent array or object, -1 if this is the root object so not in another array/object
const char *j_val(j_t);	// The value of this object as a string. NULL if not found. Note that a "null" string is a valid literal value
int j_len(j_t);	// The length of this value (characters if string or number or literal), or number of entries if object or array

const char *j_get(j_t,const char *tags);	// Find and get val using tags, NULL for not found

// Information about data type of this point
int j_isarray(j_t); // True if is an array
int j_isobject(j_t); // True if is an object
int j_isnull(j_t); // True if is null literal
int j_isbool(j_t); // True if is a Boolean literal
int j_istrue(j_t);	// True if is true literal
int j_isnumber(j_t); // True if is a number
int j_isstring(j_t); // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)

// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char * j_read(j_t,FILE *);	// Read object from open file
char * j_read_file(j_t,const char *filename); // Read object from named file
char * j_read_mem(j_t,char *buffer); // Read object from string in memory (NULL terminated)

// Output an object - note this allows output of a raw value, e.g. string or number, if point specified is not an object itself
// Returns NULL if all is well, else a malloc'd error string
char * j_write(j_t,FILE *);
char * j_write_file(j_t,char *filename);
char * j_write_mem(j_t,char **buffer,size_t *len);

// Changing an object/value
void j_null(j_t);	// Null this point
void j_string(j_t,const char *);	// Simple set this value to a string (null terminated).
void j_stringn(j_t,const char *,size_t);	// Simple set this value to a string with specified length (allows embedded nulls in string)
void j_stringf(j_t,const char *fmt,...);	// Simple set this value to a string, using printf style format
void j_numberf(j_t,const char *fmt,...);	// Simple set this value to a number, i.e. unquoted, using printf style format
void j_literal(j_t,const char *);	// Simple set this value to a literal, e.g. "null", "true", "false"
void j_object(j_t); // Simple set this value to be an object if not already
void j_array(j_t); // Simple set this value to be an array if not already
j_t j_add(j_t,const char *tags); // Create specified tag/path, and return the point that is the value for that tag.
j_t j_append(j_t);	// Create new point at end of array
void j_remove(j_t,const char *tags); // Remove an entry from its parent
void j_sort(j_t);	// Apply a recursive sort so all objects have fields in alphabetic order.

// Additional functions to combine the above... Returns point for newly added value.
j_t j_add_string(j_t,const char *tags,const char *);	// Simple set this value to a string (null terminated).
j_t j_add_stringf(j_t,const char *tags,const char *fmt,...);	// Simple set this value to a string, using printf style format
j_t j_add_numberf(j_t,const char *tags,const char *fmt,...);	// Simple set this value to a number, i.e. unquoted, using printf style format
j_t j_add_literal(j_t,const char *tags,const char *);	// Simple set this value to a literal, e.g. "null", "true", "false"

// Additional functions to combine the above... Returns point for newly added value.
j_t j_append_string(j_t,const char *tags,const char *);	// Simple set this value to a string (null terminated).
j_t j_append_stringf(j_t,const char *tags,const char *fmt,...);	// Simple set this value to a string, using printf style format
j_t j_append_numberf(j_t,const char *tags,const char *fmt,...);	// Simple set this value to a number, i.e. unquoted, using printf style format
j_t j_append_literal(j_t,const char *tags,const char *);	// Simple set this value to a literal, e.g. "null", "true", "false"

// Moving parts of objects...
j_t j_attach(j_t,j_t);	// Replaces first value with the second in a tree, removing second from its parent if it has one, returns first arg or NULL for error

