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
#include <stdarg.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#undef NDEBUG                   // Uses side effects in asset
#include <assert.h>

struct ajl_s
{
   FILE *f;                     // File being parsed or generated
   int line;                    // Line number
   int posn;                    // Character position
   int level;                   // Current level
   int maxlevel;                // Max level allocated flags
   const char *error;           // Current error
   unsigned char *flags;        // Flags
   unsigned char peek;          // Next character for read
   unsigned char eof:1;
};
#define	COMMA	1               // flags
#define OBJECT	2

#define escapes \
         esc ('\\', '\\') \
	esc ('"', '"') \
	esc ('t', '\t') \
	esc ('b', '\b') \
	esc ('f', '\f') \
	esc ('r', '\r') \
	esc ('n', '\n')

// Local functions
#define validate(j) if(!j)return "NULL control passed"; if(j->error)return j->error;
static inline void
next (ajl_t j, FILE * o)
{
   if (!j || j->error || j->eof)
      return;
   if (o)
      fputc (j->peek, o);
   int c = fgetc (j->f);
   if (c < 0)
      j->eof = 1;
   else if ((c >= 0x20 && c < 0x80) || c >= 0xC0)
      j->posn++;                // Count character (UTF-8 as one)
   else if (c == '\n')
   {
      j->line++;
      j->posn = 1;
   }
   j->peek = c;
   return;
}

static inline int
isws (unsigned char c)
{
   return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static inline const char *
skip_ws (ajl_t j)
{                               // Skip white space
   validate (j);
   while (!j->eof && isws (j->peek))
      next (j, NULL);
   return NULL;
}

static inline const char *
skip_comma (ajl_t j)
{                               // Skip initial comma and whitespace
   validate (j);
   skip_ws (j);
   if (j->peek == ',')
   {                            // skip comma
      if (!(j->flags[j->level] & COMMA))
         return j->error = "Unexpected comma";
      next (j, NULL);
      skip_ws (j);
   } else if (j->flags[j->level] & COMMA)
      return j->error = "Expecting comma";
   return NULL;
}

static inline const char *
check_string (ajl_t j, FILE * o)
{                               // Process a string (i.e. starting and ending with quotes and using escapes), writing decoded string to file if not zero
   validate (j);
   if (j->eof || j->peek != '"')
      return j->error = "Missing quote";
   next (j, NULL);
   while (!j->eof && j->peek != '"')
   {
      if (j->peek == '\\')
      {                         // Escaped character
         next (j, NULL);
         if (j->eof)
            return j->error = "Bad escape at EOF";
         if (j->peek == 'u')
         {                      // hex
            next (j, NULL);
            if (!isxdigit (j->peek))
               return j->error = "Bad hex escape";
            unsigned int c = (isalpha (j->peek) ? 9 : 0) + (j->peek & 0xF);
            next (j, NULL);
            if (!isxdigit (j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha (j->peek) ? 9 : 0) + (j->peek & 0xF);
            next (j, NULL);
            if (!isxdigit (j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha (j->peek) ? 9 : 0) + (j->peek & 0xF);
            next (j, NULL);
            if (!isxdigit (j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha (j->peek) ? 9 : 0) + (j->peek & 0xF);
            next (j, NULL);
            if (o)
            {
               if (c >= 0x800)
               {
                  fputc (0xE0 + (c >> 12), o);
                  fputc (0x80 + ((c >> 6) & 0x3F), o);
                  fputc (0x80 + (c & 0x3F), o);
               } else if (c >= 0x80)
               {
                  fputc (0xC0 + (c >> 6), o);
                  fputc (0x80 + (c & 0x3F), o);
               } else
                  fputc (c, o);
            }
         }
#define esc(a,b) else if(j->peek==a){next(j,NULL);if(o)fputc(b,o);}
         escapes
#undef esc
            else
            return j->error = "Bad escape";
      } else if (j->peek >= 0xF8)
         return j->error = "Bad UTF-8";
      else if (j->peek >= 0xF0)
      {                         // 4 byte
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
      } else if (j->peek >= 0xE0)
      {                         // 3 byte
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
      } else if (j->peek >= 0xC0)
      {                         // 2 byte
         next (j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         next (j, o);
      } else if (j->peek >= 0x80)
         return j->error = "Bad UTF-8";
      else
         next (j, o);
   }
   if (j->error)
      return j->error;
   if (j->eof || j->peek != '"')
      return j->error = "Missing quote";
   next (j, NULL);
   return NULL;
}

#define checkerr if(j->error)return AJL_ERROR
#define checkeof if(j->eof&&!j->error)j->error="Unexpected EOF";checkerr
#define makeerr(e) do{j->error=e;return AJL_ERROR;}while(0)

static inline const char *
check_number (ajl_t j, FILE * o)
{                               // Process a number strictly to JSON spec for a number, writing to file if not null
   validate (j);
   if (j->peek == '-')
      next (j, o);              // Optional minus
   if (j->peek == '0')
   {
      next (j, o);
      if (!j->eof && isdigit (j->peek))
         return j->error = "Invalid int starting 0";
   } else if (!isdigit (j->peek))
      return j->error = "Invalid number";
   while (!j->eof && isdigit (j->peek))
      next (j, o);
   if (!j->eof && j->peek == '.')
   {                            // Fraction
      next (j, o);
      checkeof;
      if (!isdigit (j->peek))
         return j->error = "Invalid fraction";
      while (!j->eof && isdigit (j->peek))
         next (j, o);
   }
   if (!j->eof && (j->peek == 'e' || j->peek == 'E'))
   {                            // Exponent
      next (j, o);
      checkeof;
      if (j->peek == '-' || j->peek == '+')
         next (j, o);
      if (!isdigit (j->peek))
         return j->error = "Invalid exponent";
      while (!j->eof && isdigit (j->peek))
         next (j, o);
   }
   return NULL;
}

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
   assert ((j->flags = malloc (j->maxlevel = 10)));
   j->flags[j->level] = 0;
   j->f = f;
   j->line = 1;
   j->posn = 1;
   int c = fgetc (f);
   if (c < 0)
      j->eof = 1;
   j->peek = c;
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

int
ajl_level (ajl_t j)
{                               // return current level of nesting
   if (!j)
      return -1;
   return j->level;
}

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
   skip_ws (j);
   if (j->eof)
   {
      if (!j->level)
         return AJL_EOF;        // Normal end
      checkeof;
   }
   if (j->peek == ((j->flags[j->level] & OBJECT) ? '}' : ']'))
   {                            // end of object or array
      if (!j->level)
         makeerr ("Too many closes");
      j->level--;
      next (j, NULL);
      return AJL_CLOSE;
   }
   skip_comma (j);
   checkeof;
   if (j->flags[j->level] & OBJECT)
   {                            // skip tag
      size_t len;
      FILE *o = NULL;
      if (tag)
         o = open_memstream ((char **) tag, &len);
      check_string (j, o);
      if (o)
         fclose (o);
      checkerr;
      skip_ws (j);
      checkeof;
      if (j->peek == ':')
      {                         // found colon, skip it and white space
         next (j, NULL);
         skip_ws (j);
      } else
         makeerr ("Missing colon");
      checkeof;
   }
   j->flags[j->level] |= COMMA;
   if (j->peek == '{')
   {                            // Start object
      if (j->level + 1 >= j->maxlevel)
         assert ((j->flags = realloc (j->flags, j->maxlevel += 10)));
      j->level++;
      j->flags[j->level] = OBJECT;
      next (j, NULL);
      return AJL_OBJECT;
   }
   if (j->peek == '[')
   {                            // Start array
      if (j->level + 1 >= j->maxlevel)
         assert ((j->flags = realloc (j->flags, j->maxlevel += 10)));
      j->level++;
      j->flags[j->level] = 0;
      next (j, NULL);
      return AJL_ARRAY;
   }
   // Get the value
   FILE *o = NULL;
   if (value)
      o = open_memstream ((char **) value, len);
   if (j->peek == '"')
   {
      check_string (j, o);
      if (o)
         fclose (o);
      checkerr;
      return AJL_STRING;
   }
   if (j->peek == '-' || isdigit (j->peek))
   {
      check_number (j, o);
      if (o)
         fclose (o);
      checkerr;
      return AJL_NUMBER;
   }
   // All that is left is a literal (null, true, false)
   char l[10],
    *p = l;
   while (!j->eof && isalpha (j->peek) && p < l + sizeof (l) - 1)
   {
      *p++ = j->peek;
      next (j, o);
   }
   *p = 0;
   fclose (o);
   if (!strcmp (l, "true") || !strcmp (l, "false"))
      return AJL_BOOLEAN;
   if (!strcmp (l, "null"))
      return AJL_NULL;
   makeerr ("Unexpected token");
   return AJL_ERROR;
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
   assert ((j->flags = malloc (j->maxlevel = 10)));
   j->flags[j->level] = 0;
   j->f = f;
   j->line = 1;
   j->posn = 1;
   return j;
};

ajl_t
ajl_write_file (const char *filename)
{
   return ajl_write (fopen (filename, "w"));
};

ajl_t
ajl_write_mem (unsigned char **buffer, size_t *len)
{
   return ajl_write (open_memstream ((char **) buffer, len));
};

static void
add_string (ajl_t j, const unsigned char *value, ssize_t len)
{                               // Add escaped string
   if (!value)
   {
      fprintf (j->f, "null");
      return;
   }
   if (len < 0)
      len = strlen ((char *) value);
   fputc ('"', j->f);
   while (len--)
   {
      unsigned char c = *value++;
      if (c < ' ')
         fprintf (j->f, "\\u00%02X", c);
#define esc(a,b) else if(c==b){fputc('\\',j->f);fputc(a,j->f);}
      escapes
#undef esc
         else
         fputc (c, j->f);
   }
   fputc ('"', j->f);
}

static const char *
add_tag (ajl_t j, const unsigned char *tag)
{                               // Add prefix tag or comma
   validate (j);
   if (j->flags[j->level] & COMMA)
      fputc (',', j->f);
   j->flags[j->level] |= COMMA;
   if (tag)
   {
      if (!(j->flags[j->level] & OBJECT))
         return j->error = "Not in object";
      add_string (j, tag, -1);
      fputc (':', j->f);
   } else if (j->flags[j->level] & OBJECT)
      return j->error = "Not in array";
   return j->error;
}

const char *
ajl_add (ajl_t j, const unsigned char *tag, const unsigned char *value)
{                               // Add pre-formatted value (expects quotes, escapes, etc)
   validate (j);
   add_tag (j, tag);
   while (*value)
      fputc (*value++, j->f);
   return j->error;
};

const char *
ajl_add_string (ajl_t j, const unsigned char *tag, const unsigned char *value, ssize_t len)
{                               // Note len=-1 means use strlen(value)
   validate (j);
   add_tag (j, tag);
   add_string (j, value, len);

   return j->error;
};

const char *
ajl_add_number (ajl_t j, const unsigned char *tag, const char *fmt, ...)
{                               // Add number (formattted)
   validate (j);
   add_tag (j, tag);
   va_list ap;
   va_start (ap, fmt);
   vfprintf (j->f, fmt, ap);
   va_end (ap);
   return j->error;
}

const char *
ajl_add_boolean (ajl_t j, const unsigned char *tag, unsigned char value)
{
   validate (j);
   add_tag (j, tag);
   fprintf (j->f, value ? "true" : "false");
   return j->error;
};

const char *
ajl_add_null (ajl_t j, const unsigned char *tag)
{
   validate (j);
   add_tag (j, tag);
   fprintf (j->f, "null");
   return j->error;
};

const char *
ajl_add_object (ajl_t j, const unsigned char *tag)
{                               // Start an object
   validate (j);
   add_tag (j, tag);
   fputc ('{', j->f);
   if (j->level + 1 >= j->maxlevel)
      j->flags = realloc (j->flags, j->maxlevel += 10);
   j->level++;
   j->flags[j->level] = OBJECT;
   return j->error;
};

const char *
ajl_add_array (ajl_t j, const unsigned char *tag)
{                               // Start an array
   validate (j);
   add_tag (j, tag);
   fputc ('[', j->f);
   if (j->level + 1 >= j->maxlevel)
      j->flags = realloc (j->flags, j->maxlevel += 10);
   j->level++;
   j->flags[j->level] = 0;
   return j->error;
};

const char *
ajl_add_close (ajl_t j)
{                               // close current array or object
   validate (j);
   if (!j->level)
      return j->error = "Too many closes";
   fputc ((j->flags[j->level] & OBJECT) ? '}' : ']', j->f);
   j->level--;
   return j->error;
};
