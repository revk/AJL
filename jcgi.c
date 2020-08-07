// ==========================================================================
//
//                       Adrian's JSON Library - jcgi.c
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

#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include "jcgi.h"

char *j_cgi(j_t formdata, j_t cookie, j_t header, const char *session)
{                               // Fill in formdata, cookies, headers, and manage cookie session, return is NULL if OK, else error. All args can be NULL if not needed
   char *method = getenv("REQUEST_METHOD");
   if (!method)
      return "Not running from apache";

   if (formdata)
   {                            // Process formdata
      j_null(formdata);
      if (!strcasecmp(method, "GET"))
      {                         // Handle query string formdata
         const char *q = getenv("QUERY_STRING");
         if (q)
         {
            char *e = j_parse_formdata(formdata, q);
            if (e)
               return e;
         }
      } else if (!strcasecmp(method, "POST"))
      {                         // Handle posted (could be url formdata, multipart, or JSON)

         // TODO onexit for cleanup
      }
      // TODO Query string
      // TODO Posted url formdata
      // TODO Posted multipart
      // TODO Posted JSON
   }
   if (cookie)
   {                            // Cookies
      j_null(cookie);
      char *c = getenv("HTTP_COOKIE");
      if (c)
      {                         // Process cookies

      }

      if (session)
      {                         // Update cookie

      }
   }
   if (header)
   {                            // Headers
      j_null(header);

   }

   return NULL;
}

char *j_parse_formdata(j_t j, const char *f)
{                               // Parse formdata url encoded to JSON object
   if (!j || !f)
      return "NULL";
   j_null(j);
   while (*f)
   {
      if (*f == '&' || *f == '=')
         return "Bad form data";
      size_t lname,
       lvalue;
      char *name = NULL,
          *value = NULL;
      void get(FILE * o) {
         while (*f && *f != '=' && *f != '&')
         {
            if (*f == '%' && isxdigit(f[1]) && isxdigit(f[2]))
            {
               fputc((((f[1] & 0xF) + (isalpha(f[1]) ? 9 : 0)) << 4) + ((f[2] & 0xF) + (isalpha(f[2]) ? 9 : 0)), o);
               f += 2;
            } else
               fputc(*f, o);
            f++;
         }
         fclose(o);
      }
      get(open_memstream(&name, &lname));
      if (*f == '=')
      {                         // Value
         f++;
         get(open_memstream(&value, &lvalue));
      }
      j_t n = j_find(j, name);
      if (n)
      {                         // Exists
         if (!j_isarray(n))
         {                      // Make array
            j_t was = j_detach(n);
            n = j_make(j, name);
            j_append_json(n, &was);
         }
         n = j_append(n);
      }
      n = j_make(j, name);
      if (value)
         j_stringn(n, value, lvalue);   // Allows for nulls in string
      if (name)
         free(name);
      if (value)
         free(value);
      if (!*f)
         break;
      if (*f != '&')
         return "Bad form data";
      f++;
   }
   return NULL;
}

#ifndef LIB                     // Build as command line for testing
#include <err.h>
#include <popt.h>
int main(int __attribute__((unused)) argc, const char __attribute__((unused)) * argv[])
{
   int debug = 0;
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         { "debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug", NULL },
         POPT_AUTOHELP { }
      };
      optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp(optCon, "[filename]");
      int c;
      if ((c = poptGetNextOpt(optCon)) < -1)
         errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));
      poptFreeContext(optCon);
   }

   j_t j = j_create();
   char *err = j_cgi(j_make(j, "formdata"), j_make(j, "cookie"), j_make(j, "header"), "JCGITEST");
   if (err)
   {
      printf("Status: 500\r\nContent-Type: text/plain\r\n\r\n%s", err);
      return 1;
   }

   printf("Content-Type: text/plain\r\n\r\n");
   j_err(j_write_pretty(j, stdout));
   j_delete(&j);
   return 0;
}
#endif
