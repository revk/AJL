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
#pragma once
#include <stdio.h>

// Types
typedef struct ajl_s *ajl_t;    // The JSON parse control structure

// Those functions returning const char * return NULL for OK, else return error message. Once a parse error is found it is latched

const char *ajl_close (ajl_t);  // Close control structure (closes file). For generate_mem, this sets buffer and len correctly and adds a NULL after len.
const char *ajl_ok (ajl_t);     // Return if error set in JSON object, or NULL if not error
unsigned int ajl_line(ajl_t);	// Return current line number in source

// Parse

// Allocate control structure for parsing, from file or from memory
ajl_t ajl_parse (FILE * f);
ajl_t ajl_parse_mem (unsigned char *buffer, size_t len);

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
   AJL_OBJECT,                  // Start of array
   AJL_ARRAY,                   // Start of array
} ajl_type_t;

ajl_type_t ajl_next (ajl_t, unsigned char **tag, unsigned char **value);        // Consume next syntactic unit, set tag and value to start of respective parts and return type (as above)

// Process the unsigned char* value from above in to other types
const char *ajl_number (unsigned char *, long long *);
const char *ajl_real (unsigned char *, long double *);
const char *ajl_boolean (unsigned char *, unsigned char *);
const char *ajl_string (unsigned char *j, FILE * o);
const char *ajl_string_mem (unsigned char *j, char **buffer, size_t *len);      // Allocates memory for string

int ajl_strcmp (const unsigned char *ajl, const unsigned char *string);
int ajl_strcasecmp (const unsigned char *ajl, const unsigned char *string);

// Generate

// Allocate control structure for generating, to file or to memory
ajl_t ajl_generate (FILE * f);
ajl_t ajl_generate_mem (unsigned char **buffer, size_t *len);

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
