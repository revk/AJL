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
#include "ajlparse.h"

typedef struct j_s *j_t;	// Point in JSON tree, i.e. a value

j_t j_init();	// Allocate a new JSON object tree that is empty, ready to be added to or read in to

// Functions return const char * return error message

// Reading an object
const char* j_read(j_t,FILE *);	// Read object from open file
const char* j_read_file(j_t,const char *filename); // Read object from named file
const char* j_read_mem(j_t,char *buffer); // Read object from string in memory (NULL terminated)

// Updating an object
const char * j_add_string(j_t,const char *tag,const char *);	// Add quoted string with specified tag in current object
const char * j_add_literal(j_t,const char *tag,const char *); // Add unquoted literal (use for Boolean or null)
const char * j_rem(j_t,const char*tag); // Remove tagged entry

// TODO string with len (i.e. allow embedded nulls)
// TODO string with printf style formatting
// TODO tag as a path

// Updating an array
// TODO

// Useful type specific adds
// TODO

// Moving around an object
j_t j_root(j_t);	// Return root of point
j_t j_parent(j_t);	// Return parent of point (NULL if at root)
const char *j_tag(j_t);	// Return tag of this point, if it has one (i.e. in field in object), else NULL
const char *j_value(j_t);	// Returns text string value of this point (NULL if point is object or array)
j_t j_enter(j_t);	// Enter object or array, pointing to first item in the object or array, NULL if it has no content
j_t j_prev(j_t); // Previous point in parent object or array, NULL if at start
j_t j_next(j_t); // Next point in parent object or array, NULL if at end

// Finding tags and paths
// TODO

// Checking data types
// TODO

// Useful type specific parse
// TODO

// Writing an object
void j_write(j_t,FILE *);
void j_write_file(j_t,char *filename);
void j_write_mem(j_t,char **buffer,size_t *len);
