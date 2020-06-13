// ==========================================================================
//
//                       Adrian's JSON Library - ajl.c
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

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <time.h>
#include "ajl.h"
#include "ajlparse.c"
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <err.h>
#undef NDEBUG                   // Uses side effects in assert
#include <assert.h>
#ifdef	JCURL
#include <curl/curl.h>
#endif

// This is a point in the JSON object
// If ->children is not NULL, it is an array of pointers to child objects
// If ->children is NULL, this point is a string or number or literal (using val and len)
struct j_s {                    // JSON point strucccture
   j_t parent;                  // Parent (NULL if root)
   j_t *children;               // Array of (len) child pointer entries
   unsigned char *tag;          // Always malloced, present when this is a child of an object and so tagged
   unsigned char *val;          // Malloced if malloc set, can be static, NULL if object, array, or null
   int len;                     // Len of val or len of child
   int posn;                    // Position in parent
   unsigned char isarray:1;     // This is an array rather than an object (children is set)
   unsigned char isstring:1;    // This is a string rather than a literal (children is NULL)
   unsigned char malloc:1;      // val is malloc'd
};

static unsigned char valnull[] = "null";
static unsigned char valtrue[] = "true";
static unsigned char valfalse[] = "false";
static unsigned char valempty[] = "";
static unsigned char valzero[] = "";

const char JBASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char JBASE32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const char JBASE16[] = "0123456789ABCDEF";

// Safe free and NULL value
#define freez(x)        do{if(x)free(x);x=NULL;}while(0)

time_t j_timez(const char *t, int z)    // convert iso time to time_t
{                               // Can do HH:MM:SS, or YYYY-MM-DD or YYYY-MM-DD HH:MM:SS, 0 for invalid
   if (!t)
      return 0;
   unsigned int Y = 0,
       M = 0,
       D = 0,
       h = 0,
       m = 0,
       s = 0;
   int hms(void) {
      while (isdigit(*t))
         h = h * 10 + *t++ - '0';
      if (*t++ != ':')
         return 0;
      while (isdigit(*t))
         m = m * 10 + *t++ - '0';
      if (*t++ != ':')
         return 0;
      while (isdigit(*t))
         s = s * 10 + *t++ - '0';
      if (*t == '.')
      {                         // fractions
         t++;
         while (isdigit(*t))
            t++;
      }
      return 1;
   }
   if (isdigit(t[0]) && isdigit(t[1]) && t[2] == ':')
   {
      if (!hms())
         return 0;
      return h * 3600 + m * 60 + s;
   } else
   {
      while (isdigit(*t))
         Y = Y * 10 + *t++ - '0';
      if (*t++ != '-')
         return 0;
      while (isdigit(*t))
         M = M * 10 + *t++ - '0';
      if (*t++ != '-')
         return 0;
      while (isdigit(*t))
         D = D * 10 + *t++ - '0';
      if (*t == 'T' || *t == ' ')
      {                         // time
         t++;
         if (!hms())
            return 0;
      }
   }
   if (!Y && !M && !D)
      return 0;
   struct tm tm = {
    tm_year: Y - 1900, tm_mon: M - 1, tm_mday: D, tm_hour: h, tm_min: m, tm_sec:s
   };
   if (*t == 'Z' || z)
      return timegm(&tm);       // UTC
   tm.tm_isdst = -1;            // work it out
   return mktime(&tm);          // Local time
}

size_t j_based(char *src, char **buf, const char *alphabet, unsigned int bits)
{                               // Base16/32/64 string to binary
   if (!buf || !src)
      return -1;
   *buf = NULL;
   int b = 0,
       v = 0;
   size_t len = 0;
   FILE *out = open_memstream(buf, &len);
   while (*src && *src != '=')
   {
      char *q = strchr(alphabet, bits < 6 ? toupper(*src) : *src);
      if (!q)
      {                         // Bad character
         if (isspace(*src) || *src == '\r' || *src == '\n')
         {
            src++;
            continue;
         }
         if (*buf)
            free(*buf);
         return -1;
      }
      v = (v << bits) + (q - alphabet);
      b += bits;
      src++;
      if (b >= 8)
      {                         // output byte
         b -= 8;
         fputc(v >> b, out);
      }
   }
   fclose(out);
   return len;
}

char *j_baseN(size_t slen, const unsigned char *src, size_t dmax, char *dst, const char *alphabet, unsigned int bits)
{                               // base 16/32/64 binary to string
   unsigned int i = 0,
       o = 0,
       b = 0,
       v = 0;
   while (i < slen)
   {
      b += 8;
      v = (v << 8) + src[i++];
      while (b >= bits)
      {
         b -= bits;
         if (o < dmax)
            dst[o++] = alphabet[(v >> b) & ((1 << bits) - 1)];
      }
   }
   if (b)
   {                            // final bits
      b += 8;
      v <<= 8;
      b -= bits;
      if (o < dmax)
         dst[o++] = alphabet[(v >> b) & ((1 << bits) - 1)];
      while (b)
      {                         // padding
         while (b >= bits)
         {
            b -= bits;
            if (o < dmax)
               dst[o++] = '=';
         }
         if (b)
            b += 8;
      }
   }
   if (o >= dmax)
      return NULL;
   dst[o] = 0;
   return dst;
}

j_t j_create(void)
{                               // Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
   return calloc(1, sizeof(struct j_s));
}

static void j_unlink(j_t j)
{                               // Unlink from parent
   j_t p = j->parent;
   if (!p)
      return;                   // No parent
   assert(p->len);
   p->len--;
   for (int q = j->posn; q < p->len; q++)
   {
      p->children[q] = p->children[q + 1];
      p->children[q]->posn = q;
   }
}

static j_t j_extend(j_t j)
{                               // Extend children
   if (!j)
      return NULL;
   assert((j->children = realloc(j->children, sizeof(*j->children) * (j->len + 1))));
   j_t n = NULL;
   assert((j->children[j->len] = n = calloc(1, sizeof(*n))));
   n->parent = j;
   n->posn = j->len++;
   return n;
}

static j_t j_findtag(j_t j, const unsigned char *tag)
{
   if (!j || !j->children || j->isarray)
      return NULL;
   for (int q = 0; q < j->len; q++)
      if (!strcmp((char *) j->children[q]->tag, (char *) tag))
         return j->children[q];
   return NULL;
}

j_t j_delete(j_t j)
{                               // Delete this value (remove from parent object if not root) and all sub objects, returns NULL
   if (!j)
      return j;
   j_null(j);                   // Clear all sub content, etc.
   j_unlink(j);
   freez(j->tag);
   freez(j);
   return NULL;
}


// Moving around the tree, these return the j_t of the new point (or NULL if does not exist)
j_t j_root(j_t j)
{                               // Return root point
   if (!j)
      return j;
   while (j->parent)
      j = j->parent;
   return j;
}

j_t j_parent(j_t j)
{                               // Parent of this point (NULL if this point is root)
   if (!j)
      return j;
   return j->parent;
}

j_t j_next(j_t j)
{                               // Next in parent object or array, NULL if at end
   if (!j || !j->parent || j->posn + 1 >= j->parent->len)
      return NULL;
   return j->parent->children[j->posn + 1];
}

j_t j_prev(j_t j)
{                               // Previous in parent object or array, NULL if at start
   if (!j || !j->parent || j->posn <= 0)
      return NULL;
   return j->parent->children[j->posn - 1];
}

j_t j_first(j_t j)
{                               // First entry in an object or array (same as j_index with 0)
   if (!j || !j->children || !j->len)
      return NULL;
   return j->children[0];
}

static j_t j_findmake(j_t j, const char *path, int make)
{                               // Find object within this object by tag/path - make if any in path does not exist and make is set
   if (!j || !path)
      return NULL;
   unsigned char *t = (unsigned char *) strdupa(path);
   while (*t)
   {
      if (!make && !j->children)
         return NULL;           // Not array or object
      if (*t == '[')
      {                         // array
         t++;
         if (make)
            j_array(j);         // Ensure is an array
         if (*t == ']')
         {                      // append
            if (!make)
               return NULL;     // Not making, so cannot append
            j = j_append(j);
         } else
         {                      // 
            int i = 0;
            while (isdigit(*t))
               i = i * 10 + *t++ - '0'; // Index
            if (*t != ']')
               return NULL;
            if (!make && i >= j->len)
               return NULL;
            if (make)
               while (i < j->len)
                  j_append(j);
            j = j_index(j, i);
         }
         t++;
      } else
      {                         // tag
         if (!make && j->isarray)
            return NULL;
         if (make)
            j_object(j);        // Ensure is an object
         unsigned char *e = (unsigned char *) t;
         while (*e && *e != '.' && *e != '[')
            e++;
         unsigned char q = *e;
         *e = 0;
         j_t n = j_findtag(j, t);
         if (!make && !n)
            return NULL;        // Not found
         if (!n)
         {
            n = j_extend(j);
            n->tag = (unsigned char *) strdup((char *) t);
         }
         *e = q;
         t = e;
         j = n;
      }
      if (*t == '.')
         t++;
   }
   return j;
}

j_t j_find(j_t j, const char *path)
{                               // Find object within this object by tag/path - NULL if any in path does not exist
   return j_findmake(j, path, 0);
}

j_t j_index(j_t j, int n)
{                               // Find specific point in an array, or object - NULL if not in the array
   if (!j || !j->children || n < 0 || n >= j->len)
      return NULL;
   return j->children[n];
}

j_t j_named(j_t j, const char *name)
{                               // Find named entry in an object
   return j_findtag(j, (const unsigned char *)name);
}


// Information about a point
const char *j_name(j_t j)
{                               // The tag of this object in parent, if it is in a parent object, else NULL
   if (!j)
      return NULL;
   return (char *) j->tag;
}

int j_pos(j_t j)
{                               // Position in parent array or object, -1 if this is the root object so not in another array/object
   if (!j)
      return -1;
   return j->posn;
}

const char *j_val(j_t j)
{                               // The value of this object as a string. NULL if not found. Note that a "null" string is a valid literal value
   if (!j || j->children)
      return NULL;
   return (char *) (j->val ? : valnull);
}

int j_len(j_t j)
{                               // The length of this value (characters if string or number or literal), or number of entries if object or array
   if (!j)
      return -1;
   if (!j->children && !j->isstring && !j->val)
      return sizeof(valnull) - 1;       // NULL val is literal NULL
   return j->len;
}

const char *j_get(j_t j, const char *path)
{                               // Find and get val using path, NULL for not found
   if (path)
      j = j_find(j, path);
   return j_val(j);
}

const char *j_get_not_null(j_t j, const char *path)
{                               // Find and get val using path, NULL for not found or null
   if (path)
      j = j_find(j, path);
   if (j_isnull(j))
      return NULL;
   return j_val(j);
}

// Information about data type of this point
int j_isarray(j_t j)
{                               // True if is an array
   return j && j->isarray;
}

int j_isobject(j_t j)
{                               // True if is an object
   return j && j->children && !j->isarray;
}

int j_isnull(j_t j)
{                               // True if is null literal
   return j && !j->children && !j->isstring && !j->val;
}

int j_isbool(j_t j)
{                               // True if is a Boolean literal
   return j && !j->children && !j->isstring && j->val && (*j->val == 't' || *j->val == 'f');
}

int j_istrue(j_t j)
{                               // True if is true literal
   return j && !j->children && !j->isstring && j->val && *j->val == 't';
}

int j_isnumber(j_t j)
{                               // True if is a number
   return j && !j->children && !j->isstring && j->val && (*j->val == '-' || isdigit(*j->val));
}

int j_isstring(j_t j)
{                               // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)
   return j && j->isstring;
}


// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char *j_read(j_t root, FILE * f)
{                               // Read object from open file
   const char *e = NULL;
   assert(root);
   assert(f);
   j_null(root);
   j_t j = NULL;
   ajl_t p = ajl_read(f);
   ajl_type_t t = 0;
   while (1)
   {
      unsigned char *tag = NULL;
      unsigned char *value = NULL;
      size_t len = 0;
      t = ajl_parse(p, &tag, &value, &len);     // We expect logical use of tag and value
      if (t <= AJL_EOF)
         break;
      if (t == AJL_CLOSE)
      {                         // End of object or array
         if (j == root)
            break;
         j = j_parent(j);
         continue;
      }
      j_t n = root;
      if (j)
         n = j_extend(j);
      if (tag)
      {                         // Tag in parent
         freez(n->tag);
         n->tag = tag;
         if (j_findtag(j, tag) != n)
         {
            j = n;
            e = "Duplicate tag";
            break;
         }
      }
      if (value)
      {                         // The value
         if (n->malloc)
            freez(n->val);
         n->malloc = 0;
         n->val = value;
         if (!strcmp((char *) value, (char *) valempty))
            n->val = valempty;
         else if (!strcmp((char *) value, (char *) valzero))
            n->val = valzero;
         else if (!strcmp((char *) value, (char *) valtrue))
            n->val = valtrue;
         else if (!strcmp((char *) value, (char *) valfalse))
            n->val = valfalse;
         else if (!strcmp((char *) value, (char *) valnull))
            n->val = NULL;
         else
            n->malloc = 1;
         n->len = strlen((char *) (n->val ? : valnull));
         if (!n->malloc)
            freez(value);
         if (t == AJL_STRING)
            n->isstring = 1;
      }
      if (t == AJL_OBJECT || t == AJL_ARRAY)
      {
         assert((n->children = realloc(n->children, n->len = 0)));
         if (t == AJL_ARRAY)
            n->isarray = 1;
         j = n;
      } else if (n == root)
         break;
   }

   if (t > AJL_EOF)
      t = ajl_parse(p, NULL, NULL, NULL);
   if (!e && t > AJL_EOF)
      e = "Extra data";
   if (!e && ajl_error(p))
      e = ajl_error(p);
   char *ret = NULL;
   if (e)
   {                            // report where in object tree we got to
      size_t len;
      FILE *f = open_memstream(&ret, &len);
      fprintf(f, "Parse fail at line %d posn %d: %s\n", ajl_line(p), ajl_char(p), e);
      while (j && j != root)
      {
         if (j->tag)
            fprintf(f, "%s", j->tag);
         else
            fprintf(f, "[%d]", j->posn);
         j = j_parent(j);
         if (j != root)
            fprintf(f, " in ");
      }
      fclose(f);
   }
   ajl_end(p);
   return ret;
}

char *j_read_close(j_t root, FILE * f)
{                               // Read object from open file
   char *e = j_read(root, f);
   fclose(f);
   return e;
}

char *j_read_file(j_t j, const char *filename)
{                               // Read object from named file
   assert(j);
   assert(filename);
   FILE *f = fopen(filename, "r");
   if (!f)
   {
      char *e = NULL;
      assert(asprintf(&e, "Failed to open %s (%s)", filename, strerror(errno)) >= 0);
      return e;
   }
   return j_read_close(j, f);
}

char *j_read_mem(j_t j, const char *buffer)
{                               // Read object from string in memory (NULL terminated)
   assert(j);
   assert(buffer);
   return j_read_close(j, fmemopen((char *) buffer, strlen(buffer), "r"));
}


// Output an object - note this allows output of a raw value, e.g. string or number, if point specified is not an object itself
// Returns NULL if all is well, else a malloc'd error string
static char *j_write_flags(j_t root, FILE * f, int pretty)
{
   assert(root);
   assert(f);
   ajl_t p = ajl_write(f);
   if (pretty)
      ajl_pretty(p);
   j_t j = root;
   do
   {
      if (j->children)
      {
         if (j->isarray)
            ajl_add_array(p, j->tag);
         else
            ajl_add_object(p, j->tag);
         if (!j->len)
            ajl_add_close(p);   // Empty object/array
         else
         {                      // Go in to array
            j = j->children[0];
            continue;
         }
      } else if (j->isstring)
         ajl_add_string(p, j->tag, j->val, j->len);
      else
         ajl_add(p, j->tag, j->val ? : (unsigned char *) valnull);
      // Next
      j_t n = NULL;
      while (j && j != root && !(n = j_next(j)))
      {
         ajl_add_close(p);
         j = j_parent(j);
      }
      j = n;
   } while (j && j != root);
   char *e = (char *) ajl_error(p);
   if (e)
      e = strdup(e);
   ajl_end(p);
   return e;
}

char *j_write(j_t root, FILE * f)
{
   return j_write_flags(root, f, 0);
}

char *j_write_close(j_t root, FILE * f)
{
   char *e = j_write_flags(root, f, 0);
   fclose(f);
   return e;
}

char *j_write_pretty(j_t root, FILE * f)
{
   return j_write_flags(root, f, 1);
}

char *j_write_pretty_close(j_t root, FILE * f)
{
   char *e = j_write_flags(root, f, 1);
   fclose(f);
   return e;
}

char *j_write_file(j_t j, const char *filename)
{
   assert(j);
   assert(filename);
   FILE *f = fopen(filename, "w");
   if (!f)
   {
      char *e = NULL;
      assert(asprintf(&e, "Failed to open %s (%s)", filename, strerror(errno)) >= 0);
      return e;
   }
   return j_write(j, f);
}

char *j_write_mem(j_t j, char **buffer, size_t *len)
{
   assert(j);
   assert(buffer);
   assert(len);
   return j_write(j, open_memstream(buffer, len));;
}

// Changing an object/value
j_t j_null(j_t j)
{                               // Null this point - used a lot internally to clear a point before setting to correct type
   assert(j);
   if (j->children)
   {                            // Object or array
      int n;
      for (n = 0; n < j->len; n++)
      {
         j_null(j->children[n]);
         freez(j->children[n]->tag);
         freez(j->children[n]);
      }
      freez(j->children);
   }
   j->isarray = 0;
   if (j->malloc)
   {
      freez(j->val);
      j->malloc = 0;
   }
   j->val = NULL;               // Even if not malloc'd
   j->len = sizeof(valnull) - 1;
   j->isstring = 0;
   return j;
}

j_t j_string(j_t j, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (!j)
      return j;
   j_null(j);
   if (!val)
      return j;
   j->val = (void *) strdup(val);
   j->len = strlen(val);
   j->malloc = 1;
   j->isstring = 1;
   return j;
}

j_t j_stringn(j_t j, const char *val, size_t len)
{                               // Simple set this value to a string with specified length (allows embedded nulls in string)
   if (!j)
      return j;
   if (len)
      assert(val);
   j_null(j);
   assert((j->val = malloc(len + 1)));
   memcpy(j->val, val, len);
   j->val[len] = 0;
   j->len = len;
   j->malloc = 1;
   j->isstring = 1;
   return j;
}

static void j_vstringf(j_t j, const char *fmt, va_list ap, int isstring)
{
   assert(vasprintf((char **) &j->val, fmt, ap) >= 0);
   j->len = strlen((char *) j->val);
   j->malloc = 1;
   j->isstring = isstring;
}

j_t j_stringf(j_t j, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (!j)
      return j;
   j_null(j);
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j, fmt, ap, 1);
   va_end(ap);
   return j;
}

j_t j_utc(j_t j, time_t t)
{
   char v[30];
   struct tm tm;
   gmtime_r(&t, &tm);
   strftime(v, sizeof(v), "%FT%TZ", &tm);
   return j_string(j, v);
}

j_t j_datetime(j_t j, time_t t)
{
   char v[30];
   struct tm tm;
   localtime_r(&t, &tm);
   strftime(v, sizeof(v), "%FT%T", &tm);
   return j_string(j, v);
}

j_t j_literalf(j_t j, const char *fmt, ...)
{                               // Simple set this value to a number, i.e. unquoted, using printf style format
   if (!j)
      return j;
   j_null(j);
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j, fmt, ap, 0);
   va_end(ap);
   return j;
}

j_t j_literal(j_t j, const char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false"
   if (!j)
      return j;
   j_null(j);
   if (val && !strcmp(val, (char *) valempty))
      val = (char *) valempty;
   else if (val && !strcmp(val, (char *) valzero))
      val = (char *) valzero;
   else if (val && !strcmp(val, (char *) valtrue))
      val = (char *) valtrue;
   else if (val && !strcmp(val, (char *) valfalse))
      val = (char *) valfalse;
   else if (val && !strcmp(val, (char *) valnull))
      val = NULL;
   else
   {
      val = strdup(val);
      j->malloc = 1;
   }
   j->val = (void *) val;
   return j;
}

j_t j_literal_free(j_t j, char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false" - free arg
   j = j_literal(j, val);
   freez(val);
   return j;
}

j_t j_object(j_t j)
{                               // Simple set this value to be an object if not already
   if (!j)
      return j;
   if (!j->children || j->isarray)
      j_null(j);
   if (!j->children)
      assert((j->children = malloc(j->len = 0)));
   return j;
}

j_t j_array(j_t j)
{                               // Simple set this value to be an array if not already
   if (!j)
      return j;
   if (!j->children || !j->isarray)
      j_null(j);
   if (!j->children)
      assert((j->children = malloc(j->len = 0)));
   j->isarray = 1;
   return j;
}

j_t j_path(j_t j, const char *path)
{                               // Find or create a path from j
   return j_findmake(j, path, 1);
}

j_t j_append(j_t j)
{                               // Create new point at end of array
   if (!j)
      return j;
   j_array(j);
   return j_extend(j);
}

static int j_sort_tag(const void *a, const void *b)
{
   return strcmp((char *) (*(j_t *) a)->tag, (char *) (*(j_t *) b)->tag);
}

void j_sort_f(j_t j, j_sort_func * f, int recurse)
{                               // Apply a recursive sort
   if (!j || !j->children)
      return;
   if (recurse && j->children)
      for (int q = 0; q < j->len; q++)
         j_sort_f(j->children[q], f, 1);
   if ((recurse && j->isarray) || !j->len)
      return;
   qsort(j->children, j->len, sizeof(*j->children), f);
   for (int q = 0; q < j->len; q++)
      j->children[q]->posn = q;
}

void j_sort(j_t j)
{                               // Recursive tag sort
   j_sort_f(j, j_sort_tag, 1);
}

j_t j_make(j_t j, const char *name)
{
   if (!name)
      return j_append(j);
   j_t n = j_findtag(j, (const unsigned char *) name);
   if (!n)
   {
      n = j_extend(j);
      n->tag = (unsigned char *) strdup(name);
   }
   return n;
}

// Additional functions to combine the above... Returns point for newly added value.

j_t j_remove(j_t j, const char *name)
{
   j_t n = j_findtag(j, (const unsigned char *) name);
   if (n)
      n = j_delete(n);
   return n;
}

j_t j_store_array(j_t j, const char *name)
{                               // Store an array at specified name in object
   return j_array(j_make(j, name));
}

j_t j_store_object(j_t j, const char *name)
{                               // Store an object at specified name in an object
   return j_object(j_make(j, name));
}

j_t j_store_string(j_t j, const char *name, const char *val)
{                               // Store a string at specified name in an object
   return j_string(j_make(j, name), val);
}

j_t j_store_stringf(j_t j, const char *name, const char *fmt, ...)
{                               // Store a (formatted) string at specified name in an object
   if (!j)
      return NULL;
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j_make(j, name), fmt, ap, 1);
   va_end(ap);
   return j;
}

j_t j_store_utc(j_t j, const char *name, time_t t)
{                               // Store a UTC time string at specified name in an object
   return j_utc(j_make(j, name), t);
}

j_t j_store_datetime(j_t j, const char *name, time_t t)
{                               // Store a localtime string at a specified name in an object
   return j_datetime(j_make(j, name), t);
}

j_t j_store_literalf(j_t j, const char *name, const char *fmt, ...)
{                               // Store a formatted literal, normally for numbers, at a specified name in an object
   if (!j)
      return NULL;
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j_make(j, name), fmt, ap, 0);
   va_end(ap);
   return j;
}

j_t j_store_literal(j_t j, const char *name, const char *val)
{                               // Store a literal (typically for true/false) at a specified name in an object
   return j_literal(j_make(j, name), val);
}

j_t j_store_literal_free(j_t j, const char *name, char *val)
{                               // Store a literal (typically for true/false) at a specified name in an object, freeing the value
   j = j_store_literal(j, name, val);
   freez(val);
   return j;
}

// Additional functions to combine the above... Returns point for newly added value.
j_t j_append_object(j_t j)
{                               // Append a new (empty) object to an array
   return j_object(j_append(j));
}

j_t j_append_array(j_t j)
{                               // Append a new (empty) array to an array
   return j_array(j_append(j));
}

j_t j_append_string(j_t j, const char *val)
{                               // Append a new string to an array
   return j_string(j_append(j), val);
}

j_t j_append_stringf(j_t j, const char *fmt, ...)
{                               // Append a new (formatted) string to an array
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j_append(j), fmt, ap, 1);
   va_end(ap);
   return j;
}

j_t j_append_utc(j_t j, time_t t)
{                               // Append a UTC time string to an array
   return j_utc(j_append(j), t);
}

j_t j_append_datetime(j_t j, time_t t)
{                               // Append a local time string to an array
   return j_datetime(j_append(j), t);
}

j_t j_append_literalf(j_t j, const char *fmt, ...)
{                               // Append a formatted literal (usually a number) to an array
   va_list ap;
   va_start(ap, fmt);
   j_vstringf(j_append(j), fmt, ap, 0);
   va_end(ap);
   return j;
}

j_t j_append_literal(j_t j, const char *val)
{                               // Append a literal (usually true/false) to an array
   return j_literal(j_append(j), val);
}

j_t j_append_literal_free(j_t j, char *val)
{                               // Append a literal and free it
   j = j_append_literal(j, val);
   freez(val);
   return j;
}

// Moving parts of objects...
j_t j_attach(j_t j, j_t o)
{                               // Replaces j with o, unlinking o from its parent, returns o
   if (!j || !o)
      return j;
   j_null(j);
   j_unlink(o);
   o->posn = j->posn;
   o->parent = j->parent;
   if (j->parent)
      j->parent->children[j->posn] = o;
   freez(j);
   return o;
}

#ifdef	JCURL
j_t j_curl(CURL * curlv, j_t input, const char *bearer, const char *url, ...)
{                               // Submit curl, get curl response
   j_t output = NULL;
   CURL *curl = curlv;
   if (!curl)
      curl = curl_easy_init();
   char *reply = NULL,
       *request = NULL;
   size_t replylen = 0,
       requestlen = 0;
   FILE *o = open_memstream(&reply, &replylen);
   char *fullurl = NULL;
   va_list ap;
   va_start(ap, url);
   if (vasprintf(&fullurl, url, ap) < 0)
      errx(1, "malloc at line %d", __LINE__);
   va_end(ap);
   curl_easy_setopt(curl, CURLOPT_URL, fullurl);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, o);
   struct curl_slist *headers = NULL;
   if (input)
   {                            // posting JSON input
      headers = curl_slist_append(headers, "Content-Type: application/json");   // posting JSON
      if (bearer)
      {
         char *sa;
         if (asprintf(&sa, "Authorization: Bearer %s", bearer) < 0)
            errx(1, "malloc at line %d", __LINE__);
         headers = curl_slist_append(headers, sa);
         free(sa);
      }
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      FILE *i = open_memstream(&request, &requestlen);
      j_write(input, i);
      fclose(i);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) requestlen);
   }                            // if not input, then assume a GET or args preset before call
   CURLcode result = curl_easy_perform(curl);
   fclose(o);
   // Put back to GET as default
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
   curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
   free(fullurl);
   if (headers)
      curl_slist_free_all(headers);
   if (request)
      free(request);
   if (result)
   {
      if (reply)
         free(reply);
      if (!curlv)
         curl_easy_cleanup(curl);
      return NULL;              // not a normal result
   }
   if (replylen)
   {
      output = j_create();
      const char *e = j_read_mem(output, reply);
      if (e)
      {
         fprintf(stderr, "Failed: %s\n%s\n", e, reply);
         output = j_delete(output);
      }
      free(reply);
   }
   if (!curlv)
      curl_easy_cleanup(curl);
   return output;
}
#endif

#ifndef	LIB                     // Build as command line for testing
int main(int __attribute__((unused)) argc, const char __attribute__((unused)) * argv[])
{
   for (int a = 1; a < argc; a++)
   {
      j_t j = j_create();
      char *e = j_read_file(j, argv[a]);
      if (e)
      {
         fprintf(stderr, "%s\n", e);
         free(e);
      } else
      {
         j_sort(j);
         j_write_pretty(j, stdout);
      }
      j = j_delete(j);
   }
   return 0;
}
#endif
