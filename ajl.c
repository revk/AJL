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
#include "ajl.h"
#include "ajlparse.c"
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#undef NDEBUG                   // Uses side effects in assert
#include <assert.h>

// This is a point in the JSON object
// If ->children is not NULL, it is an array of pointers to child objects
// If ->children is NULL, this point is a string or number or literal (using val and len)
struct j_s
{                               // JSON point strucccture
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

// Safe free and NULL value
#define freez(x)        do{if(x)free(x);x=NULL;}while(0)

j_t
j_create (void)
{                               // Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
   return calloc (1, sizeof (struct j_s));
}

static void
j_unlink (j_t j)
{                               // Unlink from parent
   j_t p = j->parent;
   if (!p)
      return;                   // No parent
   assert (p->len);
   p->len--;
   for (int q = j->posn; q < p->len; q++)
   {
      p->children[q] = p->children[q + 1];
      p->children[q]->posn = q;
   }
}

static j_t
j_extend (j_t j)
{                               // Extend children
   if (!j)
      return NULL;
   assert ((j->children = realloc (j->children, sizeof (*j->children) * (j->len + 1))));
   j_t n = NULL;
   assert ((j->children[j->len] = n = calloc (1, sizeof (*n))));
   n->parent = j;
   n->posn = j->len++;
   return n;
}

static j_t
j_findtag (j_t j, const unsigned char *tag)
{
   if (!j || !j->children || j->isarray)
      return NULL;
   for (int q = 0; q < j->len; q++)
      if (!strcmp ((char *) j->children[q]->tag, (char *) tag))
         return j->children[q];
   return NULL;
}

j_t
j_delete (j_t j)
{                               // Delete this value (remove from parent object if not root) and all sub objects, returns NULL
   if (!j)
      return j;
   j_null (j);                  // Clear all sub content, etc.
   j_unlink (j);
   freez (j->tag);
   freez (j);
   return NULL;
}


// Moving around the tree, these return the j_t of the new point (or NULL if does not exist)
j_t
j_root (j_t j)
{                               // Return root point
   if (!j)
      return j;
   while (j->parent)
      j = j->parent;
   return j;
}

j_t
j_parent (j_t j)
{                               // Parent of this point (NULL if this point is root)
   if (!j)
      return j;
   return j->parent;
}

j_t
j_next (j_t j)
{                               // Next in parent object or array, NULL if at end
   if (!j || !j->parent || j->posn + 1 >= j->parent->len)
      return NULL;
   return j->parent->children[j->posn + 1];
}

j_t
j_prev (j_t j)
{                               // Previous in parent object or array, NULL if at start
   if (!j || !j->parent || j->posn <= 0)
      return NULL;
   return j->parent->children[j->posn - 1];
}

j_t
j_first (j_t j)
{                               // First entry in an object or array (same as j_index with 0)
   if (!j || !j->children || !j->len)
      return NULL;
   return j->children[0];
}

static j_t
j_findmake (j_t j, const char *tags, int make)
{                               // Find object within this object by tag/path - make if any in path does not exist and make is set
   if (!j || !tags)
      return NULL;
   unsigned char *t = (unsigned char *) strdupa (tags);
   while (*t)
   {
      if (!make && !j->children)
         return NULL;           // Not array or object
      if (*t == '[')
      {                         // array
         t++;
         if (make)
            j_array (j);        // Ensure is an array
         if (*t == ']')
         {                      // append
            if (!make)
               return NULL;     // Not making, so cannot append
            j = j_append (j);
         } else
         {                      // 
            int i = 0;
            while (isdigit (*t))
               i = i * 10 + *t++ - '0'; // Index
            if (*t != ']')
               return NULL;
            if (!make && i >= j->len)
               return NULL;
            if (make)
               while (i < j->len)
                  j_append (j);
            j = j_index (j, i);
         }
         t++;
      } else
      {                         // tag
         if (!make && j->isarray)
            return NULL;
         if (make)
            j_object (j);       // Ensure is an object
         unsigned char *e = (unsigned char *) t;
         while (*e && *e != '.' && *e != '[')
            e++;
         unsigned char q = *e;
         *e = 0;
         j_t n = j_findtag (j, t);
         if (!make && !n)
            return NULL;        // Not found
         if (!n)
         {
            n = j_extend (j);
            n->tag = (unsigned char *) strdup ((char *) t);
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

j_t
j_find (j_t j, const char *tags)
{                               // Find object within this object by tag/path - NULL if any in path does not exist
   return j_findmake (j, tags, 0);
}

j_t
j_index (j_t j, int n)
{                               // Find specific point in an array, or object - NULL if not in the array
   if (!j || !j->children || n < 0 || n >= j->len)
      return NULL;
   return j->children[n];
}


// Information about a point
const char *
j_tag (j_t j)
{                               // The tag of this object in parent, if it is in a parent object, else NULL
   if (!j)
      return NULL;
   return (char *) j->tag;
}

int
j_pos (j_t j)
{                               // Position in parent array or object, -1 if this is the root object so not in another array/object
   if (!j)
      return -1;
   return j->posn;
}

const char *
j_val (j_t j)
{                               // The value of this object as a string. NULL if not found. Note that a "null" string is a valid literal value
   if (!j || j->children)
      return NULL;
   return (char *) (j->val ? : valnull);
}

int
j_len (j_t j)
{                               // The length of this value (characters if string or number or literal), or number of entries if object or array
   if (!j)
      return -1;
   if (!j->children && !j->isstring && !j->val)
      return sizeof (valnull) - 1;      // NULL val is literal NULL
   return j->len;
}

const char *
j_get (j_t j, const char *tags)
{                               // Find and get val using tags, NULL for not found
   if (tags)
      j = j_find (j, tags);
   return j_val (j);
}

// Information about data type of this point
int
j_isarray (j_t j)
{                               // True if is an array
   return j && j->isarray;
}

int
j_isobject (j_t j)
{                               // True if is an object
   return j && j->children && !j->isarray;
}

int
j_isnull (j_t j)
{                               // True if is null literal
   return j && !j->children && !j->isstring && !j->val;
}

int
j_isbool (j_t j)
{                               // True if is a Boolean literal
   return j && !j->children && !j->isstring && j->val && (*j->val == 't' || *j->val == 'f');
}

int
j_istrue (j_t j)
{                               // True if is true literal
   return j && !j->children && !j->isstring && j->val && *j->val == 't';
}

int
j_isnumber (j_t j)
{                               // True if is a number
   return j && !j->children && !j->isstring && j->val && (*j->val == '-' || isdigit (*j->val));
}

int
j_isstring (j_t j)
{                               // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)
   return j && j->isstring;
}


// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char *
j_read (j_t root, FILE * f)
{                               // Read object from open file
   const char *e = NULL;
   assert (root);
   assert (f);
   j_null (root);
   j_t j = NULL;
   ajl_t p = ajl_read (f);
   ajl_type_t t = 0;
   while (1)
   {
      unsigned char *tag = NULL;
      unsigned char *value = NULL;
      size_t len = 0;
      t = ajl_parse (p, &tag, &value, &len);    // We expect logical use of tag and value
      if (t <= AJL_EOF)
         break;
      if (t == AJL_CLOSE)
      {                         // End of object or array
         if (j == root)
            break;
         j = j_parent (j);
         continue;
      }
      j_t n = root;
      if (j)
         n = j_extend (j);
      if (tag)
      {                         // Tag in parent
         freez (n->tag);
         n->tag = tag;
         if (j_findtag (j, tag) != n)
         {
            j = n;
            e = "Duplicate tag";
            break;
         }
      }
      if (value)
      {                         // The value
         if (n->malloc)
            freez (n->val);
         n->malloc = 0;
         n->val = value;
         if (!strcmp ((char *) value, (char *) valempty))
            n->val = valempty;
         else if (!strcmp ((char *) value, (char *) valzero))
            n->val = valzero;
         else if (!strcmp ((char *) value, (char *) valtrue))
            n->val = valtrue;
         else if (!strcmp ((char *) value, (char *) valfalse))
            n->val = valfalse;
         else if (!strcmp ((char *) value, (char *) valnull))
            n->val = NULL;
         else
            n->malloc = 1;
         n->len = strlen ((char *) (n->val ? : valnull));
         if (!n->malloc)
            freez (value);
         if (t == AJL_STRING)
            n->isstring = 1;
      }
      if (t == AJL_OBJECT || t == AJL_ARRAY)
      {
         assert ((n->children = realloc (n->children, n->len = 0)));
         if (t == AJL_ARRAY)
            n->isarray = 1;
         j = n;
      } else if (n == root)
         break;
   }

   if (t > AJL_EOF)
      t = ajl_parse (p, NULL, NULL, NULL);
   if (!e && t > AJL_EOF)
      e = "Extra data";
   if (!e && ajl_error (p))
      e = ajl_error (p);
   char *ret = NULL;
   if (e)
   {                            // report where in object tree we got to
      size_t len;
      FILE *f = open_memstream (&ret, &len);
      fprintf (f, "Parse fail at line %d posn %d: %s\n", ajl_line (p), ajl_char (p), e);
      while (j && j != root)
      {
         if (j->tag)
            fprintf (f, "%s", j->tag);
         else
            fprintf (f, "[%d]", j->posn);
         j = j_parent (j);
         if (j != root)
            fprintf (f, " in ");
      }
      fclose (f);
   }
   ajl_close (p);
   return ret;
}

char *
j_read_file (j_t j, const char *filename)
{                               // Read object from named file
   assert (j);
   assert (filename);
   FILE *f = fopen (filename, "r");
   if (!f)
   {
      char *e = NULL;
      assert (asprintf (&e, "Failed to open %s (%s)", filename, strerror (errno)) >= 0);
      return e;
   }
   return j_read (j, f);
}

char *
j_read_mem (j_t j, char *buffer)
{                               // Read object from string in memory (NULL terminated)
   assert (j);
   assert (buffer);
   return j_read (j, fmemopen (buffer, strlen (buffer), "r"));
}


// Output an object - note this allows output of a raw value, e.g. string or number, if point specified is not an object itself
// Returns NULL if all is well, else a malloc'd error string
static char *
j_write_flags (j_t root, FILE * f, int pretty)
{
   assert (root);
   assert (f);
   ajl_t p = ajl_write (f);
   if (pretty)
      ajl_pretty (p);
   j_t j = root;
   do
   {
      if (j->children)
      {
         if (j->isarray)
            ajl_add_array (p, j->tag);
         else
            ajl_add_object (p, j->tag);
         if (!j->len)
            ajl_add_close (p);  // Empty object/array
         else
         {                      // Go in to array
            j = j->children[0];
            continue;
         }
      } else if (j->isstring)
         ajl_add_string (p, j->tag, j->val, j->len);
      else
         ajl_add (p, j->tag, j->val ? : (unsigned char *) valnull);
      // Next
      j_t n = NULL;
      while (j && j != root && !(n = j_next (j)))
      {
         ajl_add_close (p);
         j = j_parent (j);
      }
      j = n;
   } while (j && j != root);
   char *e = (char *) ajl_error (p);
   if (e)
      e = strdup (e);
   ajl_close (p);
   return e;
}

char *
j_write (j_t root, FILE * f)
{
   return j_write_flags (root, f, 0);
}

char *
j_write_pretty (j_t root, FILE * f)
{
   return j_write_flags (root, f, 1);
}

char *
j_write_file (j_t j, char *filename)
{
   assert (j);
   assert (filename);
   FILE *f = fopen (filename, "w");
   if (!f)
   {
      char *e = NULL;
      assert (asprintf (&e, "Failed to open %s (%s)", filename, strerror (errno)) >= 0);
      return e;
   }
   return j_write (j, f);
}

char *
j_write_mem (j_t j, char **buffer, size_t *len)
{
   assert (j);
   assert (buffer);
   assert (len);
   return j_write (j, open_memstream (buffer, len));;
}

// Changing an object/value
void
j_null (j_t j)
{                               // Null this point - used a lot internally to clear a point before setting to correct type
   assert (j);
   if (j->children)
   {                            // Object or array
      int n;
      for (n = 0; n < j->len; n++)
      {
         j_null (j->children[n]);
         freez (j->children[n]->tag);
         freez (j->children[n]);
      }
      freez (j->children);
   }
   j->isarray = 0;
   if (j->malloc)
   {
      freez (j->val);
      j->malloc = 0;
   }
   j->val = NULL;               // Even if not malloc'd
   j->len = sizeof (valnull) - 1;
   j->isstring = 0;
}

void
j_string (j_t j, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (!j)
      return;
   j_null (j);
   if (!val)
      return;
   j->val = (void *) strdup (val);
   j->len = strlen (val);
   j->malloc = 1;
   j->isstring = 1;
}

void
j_stringn (j_t j, const char *val, size_t len)
{                               // Simple set this value to a string with specified length (allows embedded nulls in string)
   if (!j)
      return;
   if (len)
      assert (val);
   j_null (j);
   assert ((j->val = malloc (len + 1)));
   memcpy (j->val, val, len);
   j->val[len] = 0;
   j->len = len;
   j->malloc = 1;
   j->isstring = 1;
}

static void
j_vstringf (j_t j, const char *fmt, va_list ap, int isstring)
{
   assert (vasprintf ((char **) &j->val, fmt, ap) >= 0);
   j->len = strlen ((char *) j->val);
   j->malloc = 1;
   j->isstring = isstring;
}

void
j_stringf (j_t j, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (!j)
      return;
   j_null (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 1);
   va_end (ap);
}

void
j_numberf (j_t j, const char *fmt, ...)
{                               // Simple set this value to a number, i.e. unquoted, using printf style format
   if (!j)
      return;
   j_null (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 0);
   va_end (ap);
}

void
j_literal (j_t j, const char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false"
   if (!j)
      return;
   j_null (j);
   if (val && !strcmp (val, (char *) valempty))
      val = (char *) valempty;
   else if (val && !strcmp (val, (char *) valzero))
      val = (char *) valzero;
   else if (val && !strcmp (val, (char *) valtrue))
      val = (char *) valtrue;
   else if (val && !strcmp (val, (char *) valfalse))
      val = (char *) valfalse;
   else if (val && !strcmp (val, (char *) valnull))
      val = NULL;
   else
   {
      val = strdup (val);
      j->malloc = 1;
   }
   j->val = (void *) val;
}

void
j_object (j_t j)
{                               // Simple set this value to be an object if not already
   if (!j)
      return;
   if (!j->children || j->isarray)
      j_null (j);
   if (!j->children)
      assert ((j->children = malloc (j->len = 0)));
}

void
j_array (j_t j)
{                               // Simple set this value to be an array if not already
   if (!j)
      return;
   if (!j->children || j->isarray)
      j_null (j);
   if (!j->children)
      assert ((j->children = malloc (j->len = 0)));
   j->isarray = 1;
}

j_t
j_add (j_t j, const char *tags)
{                               // Create specified tag/path, and return the point that is the value for that tag.
   return j_findmake (j, tags, 1);
}

j_t
j_append (j_t j)
{                               // Create new point at end of array
   if (!j)
      return NULL;
   j_array (j);
   return j_extend (j);
}

void
j_remove (j_t j, const char *tags)
{                               // Remove an entry from its parent
   if (tags)
      j = j_find (j, tags);
   if (j)
      j_delete (j);
}

static int
j_sort_tag (const void *a, const void *b)
{
   return strcmp ((char *) (*(j_t *) a)->tag, (char *) (*(j_t *) b)->tag);
}

void
j_sort (j_t j)
{                               // Apply a recursive sort
   if (!j || !j->children)
      return;
   if (j->children)
      for (int q = 0; q < j->len; q++)
         j_sort (j->children[q]);
   if (j->isarray || !j->len)
      return;
   qsort (j->children, j->len, sizeof (*j->children), j_sort_tag);
   for (int q = 0; q < j->len; q++)
      j->children[q]->posn = q;
}


// Additional functions to combine the above... Returns point for newly added value.
j_t
j_add_string (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (tags)
      j = j_findmake (j, tags, 1);
   j_string (j, val);
   return j;
}

j_t
j_add_stringf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (tags)
      j = j_findmake (j, tags, 1);
   if (!j)
      return NULL;
   j_null (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 1);
   va_end (ap);
   return j;
}

j_t
j_add_numberf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a number, i.e. unquoted, using printf style format
   if (tags)
      j = j_findmake (j, tags, 1);
   if (!j)
      return NULL;
   j_null (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 0);
   va_end (ap);
   return j;
}

j_t
j_add_literal (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false"
   if (tags)
      j = j_findmake (j, tags, 1);
   j_literal (j, val);
   return j;
}


// Additional functions to combine the above... Returns point for newly added value.
j_t
j_append_string (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (tags)
      j = j_findmake (j, tags, 1);
   j = j_append (j);
   j_string (j, val);
   return j;
}

j_t
j_append_stringf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (tags)
      j = j_findmake (j, tags, 1);
   j = j_append (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 1);
   va_end (ap);
   return j;
}

j_t
j_append_numberf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a number, i.e. unquoted, using printf style format
   if (tags)
      j = j_findmake (j, tags, 1);
   j = j_append (j);
   va_list ap;
   va_start (ap, fmt);
   j_vstringf (j, fmt, ap, 0);
   va_end (ap);
   return j;
}

j_t
j_append_literal (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false"
   if (tags)
      j = j_findmake (j, tags, 1);
   j = j_append (j);
   j_literal (j, val);
   return j;
}


// Moving parts of objects...
j_t
j_attach (j_t j, j_t o)
{                               // Replaces j with o, unlinking o from its parent, returns o
   if (!j || !o)
      return j;
   j_null (j);
   j_unlink (o);
   o->posn = j->posn;
   o->parent = j->parent;
   if (j->parent)
      j->parent->children[j->posn] = o;
   freez (j);
   return o;
}

#ifndef	LIB                     // Build as command line for testing
int
main (int __attribute__((unused)) argc, const char __attribute__((unused)) * argv[])
{
   for (int a = 1; a < argc; a++)
   {
      j_t j = j_create ();
      char *e = j_read_file (j, argv[a]);
      if (e)
      {
         fprintf (stderr, "%s\n", e);
         free (e);
      } else
      {
         j_sort (j);
         j_write_pretty (j, stdout);
      }
      j = j_delete (j);
   }
   return 0;
}
#endif
