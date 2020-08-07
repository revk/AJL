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
#include "ajl.h"

char *j_cgi(j_t formdata, j_t cookie, j_t header, const char *session)
{                               // Fill in formdata, cookies, headers, and manage cookie session, return is NULL if OK, else error. All args can be NULL if not needed
   if (formdata)
   {                            // Process formdata
      j_null(formdata);
      char *method = getenv("REQUEST_METHOD");
      if (!method)
         return "Not running from apache";

      // Query string
      // Posted url formdata
      // Posted multipart
      // Posted JSON
   }
   if (cookie)
   {                            // Cookies
      j_null(cookie);
      char *c = getenv("HTTP_COOKIE");
      if (c)
      {                         // Proccess cookies

      }

      if (session)
      {                         // Update cookie

      }
   }
   if (header)
   {                            // Headers
      j_null(header);

   }

   return "TODO";
}

char *j_parse_formdata(j_t j, const char *formdata)
{                               // Parse formdata url encoded to JSON object
   if (!j || !formdata)
      return "NULL";

   // TODO onexit for cleanup

   return "TODO";
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
   char *err = j_cgi(j_make(j, "form"), j_make(j, "cookie"), j_make(j, "header"), "JCGITEST");
   if (err)
      errx(1, "Failed: %s", err);

   // TODO

   return 0;
}
#endif
