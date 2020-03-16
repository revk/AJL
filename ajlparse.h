// ==========================================================================
//
//                       Adrian's JSON Library - ajlparse.h
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

//
// THIS IS PROBABLY NOT THE FILE YOU ARE LOOKING FOR, MOVE ALONG PLEASE, NO DROIDS HERE
//
// This used for low level parsing, and not manipulating JSON objects in memory
// You probably want to look at ajl.h for that

#pragma once
#include <stdio.h>

// This library provides simple means to parse underlying JSON elements, and to output them
// Error handling is by means of setting an error state which stops further processing
// The error string is returned from most functions, and the line/character of the error can be checked

// Types
typedef struct ajl_s *ajl_t;    // The JSON parse control structure

// Those functions returning const char * return NULL for OK, else return error message. Once a parse error is found it is latched

// Common functtions
const char *ajl_close (ajl_t);  // Close control structure (closes file). For write_mem, this sets buffer and len correctly and adds a NULL after len.
ajl_t ajl_delete(ajl_t);	// Free the handle, returns NULL
const char *ajl_ok (ajl_t);     // Return if error set in JSON object, or NULL if not error

// Allocate control structure for parsing, from file or from memory
ajl_t ajl_read (FILE *);
ajl_t ajl_read_file (const char *filename);
ajl_t ajl_read_mem (unsigned char *buffer, size_t len);
int ajl_line(ajl_t);	// Return current line number in source
int ajl_char(ajl_t);	// Return current character position in source

typedef enum
{                               // Parse types
   AJL_ERROR,                   // Error is set
   AJL_EOF,                     // End of file reached (at expected point, no error)
   AJL_STRING,
   AJL_NUMBER,
   AJL_REAL,
   AJL_BOOLEAN,
   AJL_NULL,
   AJL_CLOSE,                   // End of object or array
   AJL_OBJECT,                  // Start of object
   AJL_ARRAY,                   // Start of array
} ajl_type_t;

// The basic parsing function consumes next element, and returns a type as above
// If the element is within an object, then the tag is parsed and mallocd and stored in tag
// The value of the element is parsed, and malloced and stored in value (a null is appended, not included in len)
// The length of the value is stored in len - this is mainly to allow for strings that contain a null
ajl_type_t ajl_parse (ajl_t, unsigned char **tag, unsigned char **value,size_t *len);

// Generate

// Allocate control structure for generating, to file or to memory
ajl_t ajl_write (FILE *);
ajl_t ajl_write_file (const char *filename);
ajl_t ajl_write_mem (unsigned char **buffer, size_t *len);

const char *ajl_add (ajl_t, const unsigned char *tag, const unsigned char *value);      // Add pre-formatted value (expects quotes, escapes, etc)
const char *ajl_add_string (ajl_t, const unsigned char *tag, const unsigned char *value, size_t len);   // Note len=-1 means use strlen(value)
const char *ajl_add_base64 (ajl_t, const unsigned char *tag, const unsigned char *buf, size_t len);     // Base64 (URL) coded string
const char *ajl_add_colour (ajl_t, const unsigned char *tag, unsigned int RGB); // Quoted colour # notation
const char *ajl_add_number (ajl_t, const unsigned char *tag, long long value);
const char *ajl_add_real (ajl_t, const unsigned char *tag, long double value);
const char *ajl_add_boolean (ajl_t, const unsigned char *tag, unsigned char value);
const char *ajl_add_null (ajl_t, const unsigned char *tag);
const char *ajl_add_object (ajl_t, const unsigned char *tag);   // Start an object
const char *ajl_add_array (ajl_t, const unsigned char *tag);    // Start an array
const char *ajl_add_close (ajl_t);      // close current array or object

