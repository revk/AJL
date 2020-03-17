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
#undef NDEBUG                   // Uses side effects in assert
#include <assert.h>

// This is a point in the JSON object
// If child is not NULL, it is an object or array with len allocated entries in child
// If child is NULL it is a string or number or literal (val and len)
struct j_s
{                               // JSON point strucccture
   j_t parent;                  // Parent (NULL if root)
   j_t child;                   // Array of (len) child entries
   char *tag;                   // Always malloced, present when this is a child of an object and so tagged
   char *val;                   // Malloced if malloc set, can be static, NULL if object, array, or null
   int len;                     // Len of val or len of child
   int posn;                    // Position in parent
   unsigned char isarray:1;
   unsigned char isstring:1;
   unsigned char malloc:1;
};

static const char valnull[] = "null";
static const char valtrue[] = "true";
static const char valfalse[] = "false";
static const char valempty[] = "";
static const char valzero[] = "";

// Safe free and NULL value
#define freez(x)        do{if(x)free(x);x=NULL;}while(0)

j_t
j_create (void)
{                               // Allocate a new JSON object tree that is empty, ready to be added to or read in to, NULL for error
   return calloc (1, sizeof (struct j_s));
}

j_t
j_delete (j_t j)
{                               // Delete this value (remove from parent object if not root) and all sub objects, returns NULL
   if (!j)
      return j;
   // TODO
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
   if (!j || !j->parent || j->posn >= j->parent->len)
      return NULL;
   return j + 1;
}

j_t
j_prev (j_t j)
{                               // Previous in parent object or array, NULL if at start
   if (!j || !j->parent || j->posn <= 0)
      return NULL;
   return j - 1;
}

j_t
j_first (j_t j)
{                               // First entry in an object or array (same as j_index with 0)
   if (!j || !j->child)
      return NULL;
   return &j->child[0];
}

j_t
j_find (j_t j, const char *tags)
{                               // Find object within this object by tag/path - NULL if any in path does not exist
   if (!j || tags)
      return NULL;
   // TODO
   return NULL;
}

j_t
j_findmake (j_t j, const char *tags)
{                               // Find object within this object by tag/path - make if any in path does not exist
   if (!j || tags)
      return NULL;
   // TODO
   return NULL;
}

j_t
j_index (j_t j, int n)
{                               // Find specific point in an array, or object - NULL if not in the array
   if (!j || !j->child || n < 0 || n >= j->len)
      return NULL;
   return &j->child[n];
}


// Information about a point
const char *
j_tag (j_t j)
{                               // The tag of this object in parent, if it is in a parent object, else NULL
   if (!j)
      return NULL;
   return j->tag;
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
   if (!j || j->child)
      return NULL;
   return j->val ? : valnull;
}

int
j_len (j_t j)
{                               // The length of this value (characters if string or number or literal), or number of entries if object or array
   if (!j)
      return -1;
   if (!j->child && !j->isstring && !j->val)
      return sizeof (valnull) - 1;
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
   return j && j->child && !j->isarray;
}

int
j_isnull (j_t j)
{                               // True if is null literal
   return j && !j->child && !j->isstring && !j->val;
}

int
j_isbool (j_t j)
{                               // True if is a Boolean literal
   return j && !j->child && !j->isstring && j->val && (*j->val == 't' || *j->val == 'f');
}

int
j_istrue (j_t j)
{                               // True if is true literal
   return j && !j->child && !j->isstring && j->val && *j->val == 't';
}

int
j_isnumber (j_t j)
{                               // True if is a number
   return j && !j->child && !j->isstring && j->val && (*j->val == '-' || isdigit (*j->val));
}

int
j_isstring (j_t j)
{                               // True if is a string (i.e. quoted, note "123" is a string, 123 is a number)
   return j && j->isstring;
}


// Loading an object. This replaces value at the j_t specified, which is usually a root from j_create()
// Returns NULL if all is well, else a malloc'd error string
char *
j_read (j_t j, FILE * f)
{                               // Read object from open file
   assert (j);
   assert (f);
   // TODO
   return NULL;
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
char *
j_write (j_t j, FILE * f)
{
   assert (j);
   assert (f);
   // TODO
   return NULL;
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
   if (j->child)
   {                            // Object or array
      int n;
      for (n = 0; n < j->len; n++)
      {
         freez (j->child[n].tag);
         j_null (&j->child[n]);
      }
      freez (j->child);
   }
   j->isarray = 0;
   if (j->malloc)
   {
      freez (j->val);
      j->malloc = 0;
   }
   j->val = NULL;               // Even if not malloc'd
   j->isstring = 0;
   return;
}

void
j_string (j_t j, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (!j)
      return;
   j_null (j);
   if (!val)
      return;
   j->val = strdup (val);
   j->len = strlen (val);
   j->malloc = 1;
   j->isstring = 1;
   return;
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
   return;
}

static void
j_vstringf (j_t j, const char *fmt, va_list ap, int isstring)
{
   assert (vasprintf (&j->val, fmt, ap) >= 0);
   j->len = strlen (j->val);
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
   return;
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
   return;
}

void
j_literal (j_t j, const char *val)
{                               // Simple set this value to a literal, e.g. "null", "true", "false"
   if (!j)
      return;
   j_null (j);
   if (val && !strcmp (val, valempty))
      val = valempty;
   else if (val && !strcmp (val, valzero))
      val = valzero;
   else if (val && !strcmp (val, valtrue))
      val = valtrue;
   else if (val && !strcmp (val, valfalse))
      val = valfalse;
   else if (val && !strcmp (val, valnull))
      val = NULL;
   else
   {
      val = strdup (val);
      j->malloc = 1;
   }
   j->val = (char *) val;
   return;
}

void
j_object (j_t j)
{                               // Simple set this value to be an object if not already
   if (!j)
      return;
   if (!j->child || j->isarray)
      j_null (j);
   assert ((j->child = malloc (0)));
   return;
}

void
j_array (j_t j)
{                               // Simple set this value to be an array if not already
   if (!j)
      return;
   if (!j->child || j->isarray)
      j_null (j);
   assert ((j->child = malloc (0)));
   j->isarray = 1;
   return;
}

j_t
j_add (j_t j, const char *tags)
{                               // Create specified tag/path, and return the point that is the value for that tag.
   return j_findmake (j, tags);
}

j_t
j_append (j_t j)
{                               // Create new point at end of array
   if (!j)
      return NULL;
   j_array (j);
   // TODO
   return &j->child[0];
}

void
j_remove (j_t j, const char *tags)
{                               // Remove an entry from its parent
   if (tags)
      j = j_find (j, tags);
   if (!j || !j->parent)
      return;
   // TODO
   return;
}


// Additional functions to combine the above... Returns point for newly added value.
j_t
j_add_string (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (tags)
      j = j_findmake (j, tags);
   j_string (j, val);
   return j;
}

j_t
j_add_stringf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (tags)
      j = j_findmake (j, tags);
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
      j = j_findmake (j, tags);
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
      j = j_findmake (j, tags);
   j_literal (j, val);
   return j;
}


// Additional functions to combine the above... Returns point for newly added value.
j_t
j_append_string (j_t j, const char *tags, const char *val)
{                               // Simple set this value to a string (null terminated).
   if (tags)
      j = j_findmake (j, tags);
   j = j_append (j);
   j_string (j, val);
   return j;
}

j_t
j_append_stringf (j_t j, const char *tags, const char *fmt, ...)
{                               // Simple set this value to a string, using printf style format
   if (tags)
      j = j_findmake (j, tags);
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
      j = j_findmake (j, tags);
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
      j = j_findmake (j, tags);
   j = j_append (j);
   j_literal (j, val);
   return j;
}


// Moving parts of objects...
j_t
j_attach (j_t j, j_t o)
{                               // Replaces first value with the second in a tree, removing second from its parent if it has one, returns first arg or NULL for error
   if (!j || !o)
      return j;
   // TODO
   return j;
}

#ifndef	LIB                     // Build as command line for testing
int
main (int __attribute__((unused)) argc, const char __attribute__((unused)) * argv[])
{

   return 0;
}
#endif
