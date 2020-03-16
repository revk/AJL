// ==========================================================================
//
//                       Adrian's JSON Library - ajlparse.c
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

#include "ajlparse.h"
#include <malloc.h>

struct ajl_s
{
   FILE *f;                     // File being parsed or generated
   int line;                    // Line number
   int posn;                    // Character position
   const char *error;           // Current error
   unsigned long long comma;    // bit 0 set means current level needs a comma before next value (allows 64 levels)
   unsigned long long object;   // bit 0 set means current level is an object so expects tag:value (allows 64 levels)
   unsigned char level;         // Nesting level
};

#define validate(j) if(!j)return "NULL control passed"; if(j->error)return j->error;

// Common functions
const char *
ajl_close (ajl_t j)
{                               // Close control structure (closes file). For write_mem, this sets buffer and len correctly and adds a NULL after len.
   validate (j);
   fclose (j->f);
   j->f = NULL;
   return NULL;
};

ajl_t
ajl_delete (ajl_t j)
{                               // Free the handle, returns NULL
   if (j)
   {
      if (j->f)
         fclose (j->f);
      free (j);
   }
   return NULL;
};

const char *
ajl_ok (ajl_t j)
{                               // Return if error set in JSON object, or NULL if not error
   validate (j);
   return NULL;
};


// Allocate control structure for parsing, from file or from memory
ajl_t
ajl_read (FILE * f)
{
   if (!f)
      return NULL;
   ajl_t j = calloc (1, sizeof (*j));
   if (!j)
      return j;
   j->f = f;
   j->line = 1;
   return j;
};

ajl_t
ajl_read_file (const char *filename)
{
   return ajl_read (fopen (filename, "r"));
};

ajl_t
ajl_read_mem (unsigned char *buffer, size_t len)
{
   return ajl_read (fmemopen (buffer, len, "r"));
};

int
ajl_line (ajl_t j)
{                               // Return current line number in source
   if (!j)
      return -1;
   return j->line;
};

int
ajl_char (ajl_t j)
{                               // Return current character position in source
   if (!j)
      return -1;
   return j->posn;
};

// The basic parsing function consumes next element, and returns a type as above
// If the element is within an object, then the tag is parsed and mallocd and stored in tag
// The value of the element is parsed, and malloced and stored in value (a null is appended, not included in len)
// The length of the value is stored in len - this is mainly to allow for strings that contain a null
ajl_type_t
ajl_parse (ajl_t j, unsigned char **tag, unsigned char **value, size_t *len)
{
   if (tag)
      *tag = NULL;
   if (value)
      *value = NULL;
   if (len)
      *len = 0;
   if (!j || j->error)
      return AJL_ERROR;
   // TODO
   return 0;
};


// Generate

// Allocate control structure for generating, to file or to memory
ajl_t
ajl_write (FILE * f)
{
	   if (!f)
      return NULL;
   ajl_t j = calloc (1, sizeof (*j));
   if (!j)
      return j;
   j->f = f;
   j->line = 1;
   return j;
};

ajl_t
ajl_write_file (const char *filename)
{
	return ajl_write(fopen(filename,"w"));
};

ajl_t
ajl_write_mem (unsigned char **buffer, size_t *len)
{
   return ajl_write(open_memstream((char**)buffer,len));
};


const char *
ajl_add (ajl_t j, const unsigned char *tag, const unsigned char *value)
{                               // Add pre-formatted value (expects quotes, escapes, etc)
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_string (ajl_t j, const unsigned char *tag, const unsigned char *value, size_t len)
{                               // Note len=-1 means use strlen(value)
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_base64 (ajl_t j, const unsigned char *tag, const unsigned char *buf, size_t len)
{                               // Base64 (URL) coded string
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_colour (ajl_t j, const unsigned char *tag, unsigned int RGB)
{                               // Quoted colour # notation
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_number (ajl_t j, const unsigned char *tag, long long value)
{
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_real (ajl_t j, const unsigned char *tag, long double value)
{
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_boolean (ajl_t j, const unsigned char *tag, unsigned char value)
{
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_null (ajl_t j, const unsigned char *tag)
{
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_object (ajl_t j, const unsigned char *tag)
{                               // Start an object
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_array (ajl_t j, const unsigned char *tag)
{                               // Start an array
   validate (j);
   // TODO
   return NULL;
};

const char *
ajl_add_close (ajl_t j)
{                               // close current array or object
   validate (j);
   // TODO
   return NULL;
};
