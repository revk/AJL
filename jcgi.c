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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "jcgi.h"

extern char **environ;

typedef struct fn_s fn_t;
static struct fn_s {
   fn_t *next;
   char *fn;
} *cleanup = NULL;

static void docleanup(void)
{
   while (cleanup)
   {
      fn_t *next = cleanup->next;
      unlink(cleanup->fn);
      free(cleanup->fn);
      free(cleanup);
      cleanup = next;
   }
}

static char *newtmpfile(void)
{                               // Make temp file
   char temp[] = "/tmp/jcgi-XXXXXX";
   int f = mkstemp(temp);
   if (f < 0)
      return NULL;
   close(f);
   if (!cleanup)
      atexit(docleanup);
   fn_t *c = malloc(sizeof(*c));
   c->fn = strdup(temp);
   c->next = cleanup;
   cleanup = c;
   return c->fn;
}

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
         char *ct = getenv("CONTENT_TYPE");
         if (!ct)
            return "No content type for POST";

         char *data = NULL;
         size_t len = 0;
         FILE *o = open_memstream(&data, &len);
         {
            char buf[1024];
            long l = -1;
            char *cl = getenv("CONTENT_LENGTH");
            if (cl)
               strtoul(cl, NULL, 0);
            while (1)
            {
               size_t r = fread(buf, 1, sizeof(buf), stdin);
               if (!r)
                  break;
               fwrite(buf, r, 1, o);
               if (l >= 0 && (l -= r) <= 0)
                  break;
            }
            if (l > 0)
               return "Read stopped before end";
         }
         fclose(o);

         if (!strcasecmp(ct, "application/x-www-form-urlencoded"))
         {                      // Simple URL encoding
            char *e = j_parse_formdata(formdata, data);
            if (e)
               return e;
         } else if (!strcasecmp(ct, "application/json"))
         {                      // JSON
            char *e = j_read_mem(formdata, data);
            if (e)
               return e;
         } else if(!strncasecmp(ct,"multipart/form-data",19))
	 { // Form data...

	 }
	 else
         {                      // Something else.
            char *t = newtmpfile();
            if (!t)
               return "Cannot make temp";
            FILE *o = fopen(t, "w");
            if (!o)
               return "Tmp file failed";
            if (fwrite(data, len, 1, o) != 1)
               return "Tmp write failed";
            fclose(o);
            j_store_string(formdata, "tmpfile", t);
            j_store_string(formdata, "type", ct);
         }
	 j_stringn(j_make(formdata,"file"),data,len);// TODO debug
         if (data)
            free(data);
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
         j_parse_formdata_sep(cookie, c, ';');
      if (session)
      {                         // Update cookie
         const char *u = j_get(cookie, session);
         if (!u || strlen(u) != 36)
         {
            int f = open("/dev/urandom", O_RDONLY);
            if (f < 0)
               warn("Random open failed");
            else
            {
               unsigned char v[16];
               if (read(f, &v, sizeof(v)) != sizeof(v))
                  warn("Random read failed");
               else
               {
                  v[6] = 0x40 | (v[6] & 0x0F);  // Version 4: Random
                  v[8] = 0x80 | (v[8] & 0x3F);  // Variant 1
                  char uuid[37];
                  snprintf(uuid, sizeof(uuid), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
                  int max = 365 * 86400;
                  time_t now = time(0) + max;
                  char temp[50];
                  strftime(temp, sizeof(temp), "%a, %d-%b-%Y %T GMT", gmtime(&now));
                  printf("Set-Cookie: %s=%s; Path=/; Max-Age=%d; Expires=%s; HTTPOnly;%s\r\n", session, uuid, max, temp, getenv("HTTPS") ? " Secure" : "");
                  j_store_string(cookie, session, uuid);
               }
               close(f);
            }
         }
      }
      j_sort(cookie);
   }
   if (header)
   {                            // Headers
      j_null(header);
      for (char **ep = environ; *ep; ep++)
      {
         char *e = *ep;
         char *v = strchr(e, '=');
         if (!v)
            continue;
         v++;
         fprintf(stderr, "%s\n", e);
         if (!strncmp(e, "HTTP_", 5) && (!cookie || strncmp(e, "HTTP_COOKIE=", 12)))
         {                      // Header from Apache
            char *name = malloc(v - e - 5);
            memcpy(name, e + 5, v - e - 6);
            name[v - e - 6] = 0;
            for (char *q = name; *q; q++)
            {                   // Change to match normal header case and characters
               if (*q == '_')
                  *q = '-';
               else if (isupper(*q) && q != name && isalpha(q[-1]))
                  *q = tolower(*q);
            }
            j_store_string(header, name, v);
            free(name);
            continue;
         }
         if (!strncmp(e, "PATH_", 5) || !strncmp(e, "SERVER_", 7) || !strncmp(e, "REMOTE_", 7) || !strncmp(e, "REQUEST_", 8) || !strncmp(e, "SCRIPT_", 7) || (!strncmp(e, "QUERY_", 6) && (!formdata || strncmp(e, "QUERY_STRING=", 13))))
         {                      // Just lower case these
            char *name = malloc(v - e);
            memcpy(name, e, v - e - 1);
            name[v - e - 1] = 0;
            for (char *q = name; *q; q++)
               if (isupper(*q))
                  *q = tolower(*q);
            j_store_string(header, name, v);
            free(name);
            continue;
         }
      }
      if (getenv("HTTPS"))
         j_store_string(header, "https", getenv("SSL_TLS_SNI"));
      j_sort(header);
   }

   return NULL;
}

char *j_parse_formdata_sep(j_t j, const char *f, char sep)
{                               // Parse formdata url encoded to JSON object
   if (!j || !f)
      return "NULL";
   j_null(j);
   while (*f)
   {
      while (isspace(*f))
         f++;                   // Should not actually have spaces
      if (*f == sep || *f == '=')
         return "Bad form data";
      size_t lname,
       lvalue;
      char *name = NULL,
          *value = NULL;
      void get(FILE * o) {
         while (*f && *f != '=' && *f != sep)
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
      } else
         n = j_make(j, name);
      if (value)
         j_stringn(n, value, lvalue);   // Allows for nulls in string
      if (name)
         free(name);
      if (value)
         free(value);
      if (!*f)
         break;
      if (*f != sep)
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
   printf("\n");
   for (char **e = environ; *e; e++)
      printf("%s\n", *e);
   return 0;
}
#endif
