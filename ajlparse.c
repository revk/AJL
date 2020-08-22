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

struct ajl_s {
   FILE *f;                     // File being parsed or generated
   const char *text;            // Simple input text
   int line;                    // Line number
   int posn;                    // Character position
   int level;                   // Current level
   int maxlevel;                // Max level allocated flags
   const char *error;           // Current error
   unsigned char *flags;        // Flags
   unsigned char peek;          // Next character for read
   unsigned char eof:1;         // Have reached end of file (peek no longer valid)
   unsigned char pretty:1;      // Formatted output
   unsigned char started:1;     // Formatting started
};
#define	COMMA	1               // flags
#define OBJECT	2

#define escapes \
	esc ('"', '"') \
        esc ('\\', '\\') \
        esco ('/', '/') \
	esc ('b', '\b') \
	esc ('f', '\f') \
	esc ('n', '\n') \
	esc ('r', '\r') \
	esc ('t', '\t') \


// Local functions
#define validate(j) if(!j)return "NULL control passed"; if(j->error)return j->error;
int ajl_peek(const ajl_t j)
{
   if (!j)
      return -3;
   if (j->error)
      return -2;
   if (j->eof)
      return -1;
   return j->peek;
}

void ajl_next(const ajl_t j, FILE * o)
{
   if (!j || j->error || j->eof)
      return;
   if (o)
      fputc(j->peek, o);
   int c;
   if (j->text)
   {                            // Simple text parsing
      c = *++j->text;
      if (!c)
         c = -1;                // EOF in text
   } else
      c = fgetc(j->f);
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

int ajl_isws(unsigned char c)
{
   return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void ajl_skip_ws(const ajl_t j)
{                               // Skip white space
   if (!j || j->error)
      return;
   while (!j->eof && ajl_isws(j->peek))
      ajl_next(j, NULL);
}

static inline const char *skip_comma(const ajl_t j)
{                               // Skip initial comma and whitespace
   validate(j);
   ajl_skip_ws(j);
   if (j->peek == ',')
   {                            // skip comma
      if (!(j->flags[j->level] & COMMA))
         return j->error = "Unexpected comma";
      ajl_next(j, NULL);
      ajl_skip_ws(j);
   } else if (j->flags[j->level] & COMMA)
      return j->error = "Expecting comma";
   return NULL;
}

const char *ajl_string(const ajl_t j, FILE * o)
{                               // Process a string (i.e. starting and ending with quotes and using escapes), writing decoded string to file if not zero
   validate(j);
   if (j->eof || j->peek != '"')
      return j->error = "Missing quote";
   ajl_next(j, NULL);
   while (!j->eof && j->peek != '"')
   {
      if (j->peek == '\\')
      {                         // Escaped character
         ajl_next(j, NULL);
         if (j->eof)
            return j->error = "Bad escape at EOF";
         if (j->peek == 'u')
         {                      // hex
            ajl_next(j, NULL);
            if (!isxdigit(j->peek))
               return j->error = "Bad hex escape";
            unsigned int c = (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
            ajl_next(j, NULL);
            if (!isxdigit(j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
            ajl_next(j, NULL);
            if (!isxdigit(j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
            ajl_next(j, NULL);
            if (!isxdigit(j->peek))
               return j->error = "Bad hex escape";
            c = (c << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
            ajl_next(j, NULL);
            if (c >= 0xDC00 && c <= 0xDFFF)
               return "Unexpected UTF-16 low order";
            if (c >= 0xD800 && c <= 0xDBFF)
            {                   // UTF-16
               if (j->eof || j->peek != '\\')
                  return "Bad UTF-16, missing second part";
               ajl_next(j, NULL);
               if (j->eof || j->peek != 'u')
                  return "Bad UTF-16, missing second part";
               ajl_next(j, NULL);
               if (!isxdigit(j->peek))
                  return j->error = "Bad hex escape";
               unsigned int c2 = (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
               ajl_next(j, NULL);
               if (!isxdigit(j->peek))
                  return j->error = "Bad hex escape";
               c2 = (c2 << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
               ajl_next(j, NULL);
               if (!isxdigit(j->peek))
                  return j->error = "Bad hex escape";
               c2 = (c2 << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
               ajl_next(j, NULL);
               if (!isxdigit(j->peek))
                  return j->error = "Bad hex escape";
               c2 = (c2 << 4) + (isalpha(j->peek) ? 9 : 0) + (j->peek & 0xF);
               ajl_next(j, NULL);
               if (c2 < 0xDC00 || c2 > 0xDFFF)
                  return "Bad UTF-16, second part invalid";
               c = ((c & 0x3FF) << 10) + (c2 & 0x3FF) + 0x10000;
            }
            if (o)
            {
               if (c >= 0x1000)
               {
                  fputc(0xF0 + (c >> 18), o);
                  fputc(0x80 + ((c >> 12) & 0x3F), o);
                  fputc(0x80 + ((c >> 6) & 0x3F), o);
                  fputc(0x80 + (c & 0x3F), o);
               } else if (c >= 0x800)
               {
                  fputc(0xE0 + (c >> 12), o);
                  fputc(0x80 + ((c >> 6) & 0x3F), o);
                  fputc(0x80 + (c & 0x3F), o);
               } else if (c >= 0x80)
               {
                  fputc(0xC0 + (c >> 6), o);
                  fputc(0x80 + (c & 0x3F), o);
               } else
                  fputc(c, o);
            }
         }
#define esc(a,b) else if(j->peek==a){ajl_next(j,NULL);if(o)fputc(b,o);}
#define	esco(a,b) esc(a,b)      // optional
         escapes
#undef esco
#undef esc
             else
            return j->error = "Bad escape";
      } else if (j->peek >= 0xF8)
         return j->error = "Bad UTF-8";
      else if (j->peek >= 0xF0)
      {                         // 4 byte
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
      } else if (j->peek >= 0xE0)
      {                         // 3 byte
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
      } else if (j->peek >= 0xC0)
      {                         // 2 byte
         ajl_next(j, o);
         if (j->peek < 0x80 || j->peek >= 0xC0)
            return j->error = "Bad UTF-8";
         ajl_next(j, o);
      } else if (j->peek >= 0x80)
         return j->error = "Bad UTF-8";
      else
         ajl_next(j, o);
   }
   if (j->error)
      return j->error;
   if (j->eof || j->peek != '"')
      return j->error = "Missing quote";
   ajl_next(j, NULL);
   return NULL;
}

#define checkerr if(j->error)return AJL_ERROR
#define checkeof if(j->eof&&!j->error)j->error="Unexpected EOF";checkerr
#define makeerr(e) do{j->error=e;return AJL_ERROR;}while(0)

const char *ajl_number(const ajl_t j, FILE * o)
{                               // Process a number strictly to JSON spec for a number, writing to file if not null
   validate(j);
   if (j->peek == '-')
      ajl_next(j, o);           // Optional minus
   if (j->peek == '0')
   {
      ajl_next(j, o);
      if (!j->eof && isdigit(j->peek))
         return j->error = "Invalid int starting 0";
   } else if (j->eof || !isdigit(j->peek))
      return j->error = "Invalid number";
   while (!j->eof && isdigit(j->peek))
      ajl_next(j, o);
   if (!j->eof && j->peek == '.')
   {                            // Fraction
      ajl_next(j, o);
      if (j->eof || !isdigit(j->peek))
         return j->error = "Invalid fraction";
      while (!j->eof && isdigit(j->peek))
         ajl_next(j, o);
   }
   if (!j->eof && (j->peek == 'e' || j->peek == 'E'))
   {                            // Exponent
      ajl_next(j, o);
      if (!j->eof && (j->peek == '-' || j->peek == '+'))
         ajl_next(j, o);
      if (j->eof || !isdigit(j->peek))
         return j->error = "Invalid exponent";
      while (!j->eof && isdigit(j->peek))
         ajl_next(j, o);
   }
   return NULL;
}

// Common functions
const char *ajl_end_free(ajl_t j)
{                               // Close control structure (DOES NOT CLOSE file). Free j
   validate(j);
   if (j->pretty)
      fputc('\n', j->f);
   fflush(j->f);
   if (j->flags)
      free(j->flags);
   free(j);
   return NULL;
};

const char *ajl_end(const ajl_t j)
{                               // Close control structure (DOES NOT CLOSE file).
   validate(j);
   if (j->pretty)
      fputc('\n', j->f);
   fflush(j->f);
   j->f = NULL;
   return NULL;
};

const char *ajl_close(const ajl_t j)
{                               // Close control structure (closes file). For write_mem, this sets buffer and len correctly and adds a NULL after len.
   validate(j);
   if (j->f)
   {
      if (j->pretty)
         fputc('\n', j->f);
      fclose(j->f);
      j->f = NULL;
   }
   return NULL;
};

ajl_t ajl_delete(const ajl_t j)
{                               // Free the handle, returns NULL
   if (j)
   {
      if (j->f)
         fclose(j->f);
      if (j->flags)
         free(j->flags);
      free(j);
   }
   return NULL;
};

const char *ajl_error(const ajl_t j)
{                               // Return if error set in JSON object, or NULL if not error
   validate(j);
   return NULL;
};

// Allocate control structure for parsing, from file or from memory
ajl_t ajl_text(const char *text)
{                               // Simple text parse
   if (!text)
      return NULL;
   ajl_t j = calloc(1, sizeof(*j));
   if (!j)
      return j;
   j->text = text;
   if (!(j->peek = *text))
      j->eof = 1;
   return j;
}

const char *ajl_done(ajl_t j)
{                               // Get end of parse from text and free j
   if (!j || !j->text)
      return NULL;
   const char *e = j->text;
   free(j);
   return e;
}

ajl_t ajl_read(FILE * f)
{
   if (!f)
      return NULL;
   ajl_t j = calloc(1, sizeof(*j));
   if (!j)
      return j;
   assert((j->flags = malloc(j->maxlevel = 10)));
   j->flags[j->level] = 0;
   j->f = f;
   j->posn = 1;
   ajl_next(j, NULL);
   return j;
};

ajl_t ajl_read_file(const char *filename)
{
   return ajl_read(fopen(filename, "r"));
};

ajl_t ajl_read_mem(unsigned char *buffer, size_t len)
{
   return ajl_read(fmemopen(buffer, len, "r"));
};

int ajl_line(const ajl_t j)
{                               // Return current line number in source
   if (!j)
      return -1;
   return j->line;
};

int ajl_char(const ajl_t j)
{                               // Return current character position in source
   if (!j)
      return -1;
   return j->posn;
};

int ajl_level(const ajl_t j)
{                               // return current level of nesting
   if (!j)
      return -1;
   return j->level;
}

FILE *ajl_file(const ajl_t j)
{
   if (!j)
      return NULL;
   return j->f;
}

int ajl_isobject(const ajl_t j)
{
   return j->flags[j->level] & OBJECT;
}

// The basic parsing function consumes next element, and returns a type as above
// If the element is within an object, then the tag is parsed and mallocd and stored in tag
// The value of the element is parsed, and malloced and stored in value (a null is appended, not included in len)
// The length of the value is stored in len - this is mainly to allow for strings that contain a null
ajl_type_t ajl_parse(const ajl_t j, unsigned char **tag, unsigned char **value, size_t *len)
{
   if (tag)
      *tag = NULL;
   if (value)
      *value = NULL;
   if (len)
      *len = 0;
   if (!j || j->error)
      return AJL_ERROR;
   ajl_skip_ws(j);
   if (j->eof)
   {
      if (!j->level)
         return AJL_EOF;        // Normal end
      checkeof;
   }
   if (j->peek == ((j->flags[j->level] & OBJECT) ? '}' : ']'))
   {                            // end of object or array
      if (!j->level)
         makeerr("Too many closes");
      j->level--;
      ajl_next(j, NULL);
      return AJL_CLOSE;
   }
   skip_comma(j);
   checkeof;
   if (j->flags[j->level] & OBJECT)
   {                            // skip tag
      size_t len;
      FILE *o = NULL;
      if (tag)
         o = open_memstream((char **) tag, &len);
      ajl_string(j, o);
      if (o)
         fclose(o);
      checkerr;
      ajl_skip_ws(j);
      checkeof;
      if (j->peek == ':')
      {                         // found colon, skip it and white space
         ajl_next(j, NULL);
         ajl_skip_ws(j);
      } else
         makeerr("Missing colon");
      checkeof;
   }
   j->flags[j->level] |= COMMA;
   if (j->peek == '{')
   {                            // Start object
      if (j->level + 1 >= j->maxlevel)
         assert((j->flags = realloc(j->flags, j->maxlevel += 10)));
      j->level++;
      j->flags[j->level] = OBJECT;
      ajl_next(j, NULL);
      return AJL_OBJECT;
   }
   if (j->peek == '[')
   {                            // Start array
      if (j->level + 1 >= j->maxlevel)
         assert((j->flags = realloc(j->flags, j->maxlevel += 10)));
      j->level++;
      j->flags[j->level] = 0;
      ajl_next(j, NULL);
      return AJL_ARRAY;
   }
   // Get the value
   FILE *o = NULL;
   if (value)
      o = open_memstream((char **) value, len);
   if (j->peek == '"')
   {
      ajl_string(j, o);
      if (o)
         fclose(o);
      checkerr;
      return AJL_STRING;
   }
   if (j->peek == '-' || isdigit(j->peek))
   {
      ajl_number(j, o);
      if (o)
         fclose(o);
      checkerr;
      return AJL_NUMBER;
   }
   // All that is left is a literal (null, true, false)
   char l[10],
   *p = l;
   while (!j->eof && isalpha(j->peek) && p < l + sizeof(l) - 1)
   {
      *p++ = j->peek;
      ajl_next(j, o);
   }
   *p = 0;
   fclose(o);
   if (!strcmp(l, "true") || !strcmp(l, "false"))
      return AJL_BOOLEAN;
   if (!strcmp(l, "null"))
      return AJL_NULL;
   makeerr("Unexpected token");
   return AJL_ERROR;
};

// Generate
// Allocate control structure for generating, to file or to memory
ajl_t ajl_write(FILE * f)
{
   if (!f)
      return NULL;
   ajl_t j = calloc(1, sizeof(*j));
   if (!j)
      return j;
   assert((j->flags = malloc(j->maxlevel = 10)));
   j->flags[j->level] = 0;
   j->f = f;
   j->line = 1;
   j->posn = 1;
   return j;
};

ajl_t ajl_write_file(const char *filename)
{
   return ajl_write(fopen(filename, "w"));
};

ajl_t ajl_write_mem(unsigned char **buffer, size_t *len)
{
   return ajl_write(open_memstream((char **) buffer, len));
};

void ajl_pretty(const ajl_t j)
{
   j->pretty = 1;
}

void ajl_write_string(FILE * o, const unsigned char *value, size_t len)
{                               // Add UTF-8 string, escaped as necessary
   fputc('"', o);
   while (len--)
   {
      unsigned char c = *value++;
#define esc(a,b) if(c==b){fputc('\\',o);fputc(a,o);} else
#define	esco(a,b)               // optional
      escapes
#undef esco
#undef esc
          if (c < ' ')
         fprintf(o, "\\u00%02X", c);
      else
         fputc(c, o);
   }
   fputc('"', o);
}

static void add_binary(const ajl_t j, const unsigned char *value, size_t len)
{                               // Add binary data, escaped as necessary
   if (!value)
   {
      fprintf(j->f, "null");
      return;
   }
   fputc('"', j->f);
   while (len--)
   {
      unsigned char c = *value++;
#define esc(a,b) if(c==b){fputc('\\',j->f);fputc(a,j->f);} else
#define	esco(a,b)               // optional
      escapes
#undef esco
#undef esc
          if (c < ' ' || c >= 0x80)
         fprintf(j->f, "\\u00%02X", c);
      else
         fputc(c, j->f);
   }
   fputc('"', j->f);
}

static void j_indent(const ajl_t j)
{
   if (j->started && j->pretty)
   {
      fputc('\n', j->f);
      fflush(j->f);
   }
   j->started = 1;
   if (j->pretty)
      for (int q = 0; q < j->level; q++)
         fputc(' ', j->f);
}

static const char *add_tag(const ajl_t j, const unsigned char *tag)
{                               // Add prefix tag or comma
   validate(j);
   if (j->flags[j->level] & COMMA)
      fputc(',', j->f);
   j->flags[j->level] |= COMMA;
   j_indent(j);
   if (tag)
   {
      if (!(j->flags[j->level] & OBJECT))
         return j->error = "Not in object";
      ajl_write_string(j->f, tag, strlen((char *) tag));
      fputc(':', j->f);
   } else if (j->flags[j->level] & OBJECT)
      return j->error = "Tag required";
   return j->error;
}

const char *ajl_add(const ajl_t j, const unsigned char *tag, const unsigned char *value)
{                               // Add pre-formatted value (expects quotes, escapes, etc)
   validate(j);
   add_tag(j, tag);
   while (*value)
      fputc(*value++, j->f);
   return j->error;
};

const char *ajl_add_string(const ajl_t j, const unsigned char *tag, const unsigned char *value)
{                               // Add UTF-8 String, escaped for JSON
   validate(j);
   add_tag(j, tag);
   if (!value)
      fprintf(j->f, "null");
   else
      ajl_write_string(j->f, value, strlen((char *) value));
   return j->error;
};

const char *ajl_add_stringn(const ajl_t j, const unsigned char *tag, const unsigned char *value, size_t len)
{                               // Add UTF-8 String, escaped for JSON
   validate(j);
   add_tag(j, tag);
   if (!value)
      fprintf(j->f, "null");
   else
      ajl_write_string(j->f, value, len);
   return j->error;
};

const char *ajl_add_binary(const ajl_t j, const unsigned char *tag, const unsigned char *value, size_t len)
{                               // Add binary data as string, escaped for JSON
   validate(j);
   add_tag(j, tag);
   add_binary(j, value, len);
   return j->error;
};

const char *ajl_add_number(const ajl_t j, const unsigned char *tag, const char *fmt, ...)
{                               // Add number (formattted)
   validate(j);
   add_tag(j, tag);
   va_list ap;
   va_start(ap, fmt);
   vfprintf(j->f, fmt, ap);
   va_end(ap);
   return j->error;
}

const char *ajl_add_boolean(const ajl_t j, const unsigned char *tag, unsigned char value)
{
   validate(j);
   add_tag(j, tag);
   fprintf(j->f, value ? "true" : "false");
   return j->error;
};

const char *ajl_add_null(const ajl_t j, const unsigned char *tag)
{
   validate(j);
   add_tag(j, tag);
   fprintf(j->f, "null");
   return j->error;
};

const char *ajl_add_object(const ajl_t j, const unsigned char *tag)
{                               // Start an object
   validate(j);
   add_tag(j, tag);
   fputc('{', j->f);
   if (j->level + 1 >= j->maxlevel)
      j->flags = realloc(j->flags, j->maxlevel += 10);
   j->level++;
   j->flags[j->level] = OBJECT;
   return j->error;
};

const char *ajl_add_array(const ajl_t j, const unsigned char *tag)
{                               // Start an array
   validate(j);
   add_tag(j, tag);
   fputc('[', j->f);
   if (j->level + 1 >= j->maxlevel)
      j->flags = realloc(j->flags, j->maxlevel += 10);
   j->level++;
   j->flags[j->level] = 0;
   return j->error;
};

const char *ajl_add_close(const ajl_t j)
{                               // close current array or object
   validate(j);
   if (!j->level)
      return j->error = "Too many closes";
   j->level--;
   j_indent(j);
   fputc((j->flags[j->level + 1] & OBJECT) ? '}' : ']', j->f);
   return j->error;
};
