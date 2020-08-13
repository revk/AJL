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
#include <arpa/inet.h>
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

static char *newtmpfile(int flags)
{                               // Make temp file
   char temp[] = "/tmp/jcgi-XXXXXX";
   int f = mkstemp(temp);
   if (f < 0)
      return NULL;
   close(f);
   if (!(flags & JCGI_NOCLEAN) && !cleanup)
      atexit(docleanup);
   fn_t *c = malloc(sizeof(*c));
   c->fn = strdup(temp);
   c->next = cleanup;
   cleanup = c;
   return c->fn;
}

char *j_cgi_get(j_t info, j_t formdata, j_t cookie, j_t header, const char *session, int flags)
{                               // Fill in formdata, cookies, headers, and manage cookie session, return is NULL if OK, else error. All args can be NULL if not needed
   char *method = getenv("REQUEST_METHOD");
   if (!method)
      return j_errs("Not running from apache");

   if (cookie)
   {                            // Cookies
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
   if (info || header)
   {                            // Headers
      for (char **ep = environ; *ep; ep++)
      {
         char *e = *ep;
         char *v = strchr(e, '=');
         if (!v)
            continue;
         v++;
         if (header && !strncmp(e, "HTTP_", 5) && (!cookie || strncmp(e, "HTTP_COOKIE=", 12)))
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
         if (info
             && (!strncmp(e, "SSL_CLIENT_", 11) || !strncmp(e, "PATH_", 5) || !strncmp(e, "SERVER_", 7) || !strncmp(e, "REMOTE_", 7) || (!strncmp(e, "REQUEST_", 8) && (!formdata || strncmp(e, "REQUEST_URI=", 12))) || !strncmp(e, "SCRIPT_", 7)
                 || (!strncmp(e, "QUERY_", 6) && (!formdata || strncmp(e, "QUERY_STRING=", 13)))))
         {                      // Just lower case these
            char *name = malloc(v - e);
            memcpy(name, e, v - e - 1);
            name[v - e - 1] = 0;
            for (char *q = name; *q; q++)
               if (isupper(*q))
                  *q = tolower(*q);
            j_store_string(info, name, v);
            free(name);
            continue;
         }
      }
      if (info && getenv("HTTPS"))
         j_store_string(info, "https", getenv("SSL_TLS_SNI"));

      if (info)
      {
         char *x = getenv("HTTP_X_FORWARDED_FOR");
         if (x && *x)
         {                      // Seems to be relayed
            const char *i = j_get(info, "remote_addr");
            if (i)
            {
               // Integrity check - how do we know to trust this header - it has to be one of our machines relaying
               // Logic we are using is IPv6 and same /48 for server and relay, then trust forwarded for (works for A&A)
               unsigned char i6[16],
                s6[16];
               const char *s = j_get(info, "server_addr");
               if (i && s && inet_pton(AF_INET6, i, i6) > 0 && inet_pton(AF_INET6, s, s6) > 0 && !memcmp(i6, s6, 48 / 8))
               {
                  char *c = strrchr(x, ',');    // Last entry is ours, earlier ones could be sent by client
                  if (c)
                  {
                     x = c + 1;
                     while (isspace(*x))
                        x++;
                  }
                  j_store_string(info, "relay_addr", i);
                  j_store_string(info, "remote_addr", x);
               }
            }
         }
      }

      if (info)
         j_sort(info);
      if (header)
         j_sort(header);
   }
   if (formdata)
   {                            // Process formdata
      if (!strcasecmp(method, "POST"))
      {                         // Handle posted (could be url formdata, multipart, or JSON)
         char *ct = getenv("CONTENT_TYPE");
         if (!ct)
            return j_errs("No content type for POST");

         char *data = NULL;
         size_t len = 0;
         FILE *o = open_memstream(&data, &len);
         {
            char buf[16 * 1024];
            long l = -1;
            char *cl = getenv("CONTENT_LENGTH");
            if (cl)
               l = strtoul(cl, NULL, 0);
            long m = -1;
            if ((flags & JCGI_LIMIT) == JCGI_SMALL)
               m = 50000;
            else if ((flags & JCGI_LIMIT) == JCGI_MEDIUM)
               m = 1000000;
            else if ((flags & JCGI_LIMIT) == JCGI_LARGE)
               m = 50000000;
            if (m > 0 && l > 0 && l > m)
               return j_errs("Post too big %ld>%ld", l, m);
            while (1)
            {
               size_t r = fread(buf, 1, sizeof(buf), stdin);
               if (!r)
                  break;
               fwrite(buf, r, 1, o);
               if (l >= 0 && (l -= r) <= 0)
                  break;
               if (m >= 0 && (m -= l) <= 0)
                  break;        // Too big
            }
            if (l > 0)
               return j_errs("Read stopped before end (%ld)", l);
         }
         fclose(o);
         if (!data)
            return j_errs("No data in post");
         do
         {
            char *er = NULL;
            if (!strcasecmp(ct, "application/x-www-form-urlencoded"))
            {                   // Simple URL encoding
               char *e = j_parse_formdata(formdata, data);
               free(data);
               if (e)
                  return e;
               continue;
            }
            if (!(flags & JCGI_NOJSON) && !strcasecmp(ct, "application/json"))
            {                   // JSON
               er = j_read_mem(formdata, data, len);
               free(data);
               if (er && (flags & JCGI_JSONERR))
                  return er;
               if (!er)
                  continue;     // Load as non json if error
            }
            if (!strncasecmp(ct, "multipart/form-data", 19))
            {                   // Form data...
               char *boundary = NULL;
               size_t bl = 0;
               char *p = ct;
               while (*p)
               {
                  while (*p && *p != ';')
                     p++;
                  if (*p == ';')
                     p++;
                  while (isspace(*p))
                     p++;
                  if (!strncasecmp(p, "boundary=", 9))
                  {
                     p += 9;
                     boundary = p;
                     while (*p && *p != ';')
                        p++;
                     bl = p - boundary;
                     break;
                  }
               }
               if (!boundary)
                  return j_errs("No boundary on form data post");
               if (len < bl + 2)
                  return j_errs("Short post form data (%d)", (int) len);
               char *end = data + len;  // Note, we can rely on final null, so no need to check every test
               p = data;
               if (p[0] != '-' || p[1] != '-')
                  return j_errs("Form data does not start --");
               if (memcmp(p + 2, boundary, bl))
                  return j_errs("Form data does not start with boundary");
               p += bl + 2;
               while (p < end)
               {                // Scan the data, extracting each field.
                  if (p + 2 >= end || (p[0] == '-' && p[1] == '-'))
                     break;     // End
                  if (*p == '\r')
                     p++;
                  if (*p == '\n')
                     p++;
                  char *ct = NULL;
                  size_t ctl = 0;
                  char *name = NULL;
                  size_t namel = 0;
                  char *fn = NULL;
                  size_t fnl = 0;
                  // Header lines
                  while (p < end && *p != '\r' && *p != '\n')
                  {
                     char *e = p;
                     while (e < end && *e != '\r' && *e != '\n' && *e != ':')
                        e++;
                     int hl = e - p;
                     if (*e == ':')
                        e++;
                     while (e < end && isspace(*e))
                        e++;
                     char *v = e;
                     while (e < end && *e != '\r' && *e != '\n')
                        e++;
                     int vl = e - v;
                     if (hl == 19 && !strncasecmp(p, "Content-Disposition", hl))
                     {          // Should be form-data
                        if (vl >= 9 && !strncasecmp(v, "form-data", 9) && (vl == 9 || v[9] == ';'))
                        {       // look for name= and filename=
                           char *e = v + vl;
                           v += 9;
                           while (v < e)
                           {
                              if (*v == ';')
                                 v++;
                              while (isspace(*v))
                                 v++;
                              // Values RFC2047 coded FFS - Not sure I care
                              FILE *o = NULL;
                              if (!name && !strncasecmp(v, "name=", 5))
                                 o = open_memstream(&name, &namel);
                              else if (!fn && !strncasecmp(v, "filename=", 9))
                                 o = open_memstream(&fn, &fnl);
                              while (v < e && *v != '=')
                                 v++;
                              if (*v == '=')
                                 v++;
                              if (*v != '"')
                                 return j_errs("Unquoted tag value [%.*s]", (int) hl, p);
                              v++;
                              while (v < e && *v != '"')
                              {
                                 if (*v == '+')
                                    fputc(' ', o);
                                 else if (*v == '\\' && v[1] == '"')
                                 {      // Firefox does this
                                    v++;
                                    fputc(*v, o);
                                 } else if (*v == '%' && isxdigit(v[1]) && isxdigit(v[2]))
                                 {
                                    if (o)
                                       fputc((((v[1] & 0xF) + (isalpha(v[1]) ? 9 : 0)) << 4) + ((v[2] & 0xF) + (isalpha(v[2]) ? 9 : 0)), o);
                                    v += 2;
                                 } else if (o)
                                    fputc(*v, o);
                                 v++;
                              }
                              if (o)
                                 fclose(o);
                              if (*v != '"')
                                 return j_errs("Unquoted tag value [%.*s]", (int) hl, p);
                              v++;
                              while (v < end && isspace(*v))
                                 v++;
                           }
                        } else
                           return j_errs("Unexpected Content-Disposition [%.*s]", (int) hl, p);
                     } else if (hl == 12 && !strncasecmp(p, "Content-Type", hl))
                     {
                        ct = v;
                        ctl = vl;
                     }
                     p = e;
                     if (*p == '\r')
                        p++;
                     if (*p == '\n')
                        p++;
                  }
                  if (*p == '\r')
                     p++;
                  if (*p == '\n')
                     p++;
                  // The content
                  char *e = p;
                  while (e < end && (e[0] != '-' || e[1] != '-' || memcmp(e + 2, boundary, bl)))
                     e++;
                  char *next = e + bl + 2;
                  if (e > p && e[-1] == '\n')
                     e--;
                  if (e > p && e[-1] == '\r')
                     e--;
                  // Actual end of file
                  if (!name)
                     return j_errs("No name in form-data");
                  // Store
                  j_t n = j_find(formdata, name);
                  if (n)
                  {             // Exists
                     if (!j_isarray(n))
                     {          // Make array
                        j_t was = j_detach(n);
                        n = j_make(formdata, name);
                        j_append_json(n, &was);
                     }
                     n = j_append(n);
                  } else
                     n = j_make(formdata, name);
                  if (ct)
                  {             // Has a content type
                     if (fn && fnl)
                        j_store_stringn(n, "filename", fn, fnl);
                     j_store_stringn(n, "type", ct, ctl);
                     j_store_literalf(n, "size", "%d", (int) (e - p));
                     if (e > p)
                     {
                        j_t j = NULL;
                        if (!(flags & JCGI_NOJSON) && ctl == 16 && !strncasecmp(ct, "application/json", ctl))
                        {
                           j = j_make(n, "json");
                           char *er = j_read_mem(j, p, e - p);
                           if (er)
                           {
                              j_delete(&j);
                              if ((flags & JCGI_JSONERR))
                                 return er;
                              j_store_string(n, "error", er);
                              free(er);
                           }
                        }
                        if ((flags & JCGI_NOTMP) || (!(flags & JCGI_JSONTMP) && j))
                        {
                           if (!j)
                              j_store_stringn(n, "data", p, e - p);
                        } else
                        {
                           char *t = newtmpfile(flags);
                           if (!t)
                              return j_errs("Cannot make temp");
                           FILE *o = fopen(t, "w");
                           if (!o)
                              return j_errs("Tmp file failed %s", t);
                           if (fwrite(p, e - p, 1, o) != 1)
                              return j_errs("Tmp write failed %s", t);
                           fclose(o);
                           j_store_string(n, "tmpfile", t);
                        }
                     }
                  } else
                  {
                     int lvalue = e - p;
                     char c = *e;
                     *e = 0;
                     if ((lvalue == 4 && !strcmp(p, "true")) || (lvalue == 5 && !strcmp(p, "false")) || !j_number_ok(p))
                        j_literal(n, p);
                     else
                        j_stringn(n, p, lvalue);        // Allows for nulls in string
                     *e = c;
                  }
                  if (name)
                     free(name);
                  if (fn)
                     free(fn);
                  p = next;
               }
               free(data);
               continue;
            }
            // Something else.
            if (len)
            {
               if (flags & JCGI_NOTMP)
                  j_store_stringn(formdata, "data", data, len);
               else
               {
                  char *t = newtmpfile(flags);
                  if (!t)
                     return j_errs("Cannot make temp");
                  FILE *o = fopen(t, "w");
                  if (!o)
                     return j_errs("Tmp file failed %s", t);
                  if (fwrite(data, len, 1, o) != 1)
                     return j_errs("Tmp write failed %s", t);
                  fclose(o);
                  j_store_string(formdata, "tmpfile", t);
               }
               j_store_literalf(formdata, "size", "%d", (int) len);
            }
            j_store_string(formdata, "type", ct);
            if (er)
            {
               j_store_string(formdata, "error", er);
               free(er);
            }
            free(data);
         } while (0);
      }
      const char *q = getenv("QUERY_STRING");
      if (q && *q)
      {                         // Overload query string on top of formdata
         char *e = j_parse_formdata(formdata, q);
         if (e)
            return e;
      }
   }
   return NULL;
}

char *j_parse_formdata_sep(j_t j, const char *f, char sep)
{                               // Parse formdata url encoded to JSON object
   if (!j || !f)
      return j_errs("NULL");
   while (*f)
   {
      while (isspace(*f))
         f++;                   // Should not actually have spaces
      if (*f == sep || *f == '=')
         return j_errs("Bad form data [%.10s]", f);
      size_t lname,
       lvalue;
      char *name = NULL,
          *value = NULL;
      void get(FILE * o) {
         while (*f && *f != '=' && *f != sep)
         {
            if (*f == '+')
               fputc(' ', o);
            else if (*f == '%' && isxdigit(f[1]) && isxdigit(f[2]))
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
      {
         if ((lvalue == 4 && !strcmp(value, "true")) || (lvalue == 5 && !strcmp(value, "false")) || !j_number_ok(value))
            j_literal(n, value);
         else
            j_stringn(n, value, lvalue);        // Allows for nulls in string
      }
      if (name)
         free(name);
      if (value)
         free(value);
      if (!*f)
         break;
      if (*f != sep)
         return j_errs("Bad form data [%.10s]", f);
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
   int quiet = 0;
   int noclean = 0;
   int nojson = 0;
   int notmp = 0;
   int jsontmp = 0;
   int jsonerr = 0;
   int text = 0;
   int env = 0;
   int log = 0;
   int header = 0;
   int info = 0;
   int cookie = 0;
   int formdata = 0;
   const char *outfile = NULL;
   if (argc == 1 && getenv("REQUEST_METHOD"))
      text = 1;                 // Default for debugging as direct call
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         { "formdata", 'f', POPT_ARG_NONE, &formdata, 0, "formdata object", NULL },
         { "info", 'i', POPT_ARG_NONE, &info, 0, "info object", NULL },
         { "cookie", 'c', POPT_ARG_NONE, &cookie, 0, "cookie object", NULL },
         { "header", 'h', POPT_ARG_NONE, &header, 0, "header object", NULL },
         { "no-clean", 'C', POPT_ARG_NONE, &noclean, 0, "No cleanup tmp", NULL },
         { "no-tmp", 'T', POPT_ARG_NONE, &notmp, 0, "No tmp files", NULL },
         { "no-json", 'J', POPT_ARG_NONE, &nojson, 0, "Don't load JSON", NULL },
         { "json-tmp", 0, POPT_ARG_NONE, &jsontmp, 0, "Make tmp file even if JSON", NULL },
         { "json-err", 0, POPT_ARG_NONE, &jsonerr, 0, "Fail if JSON load error", NULL },
         { "text", 't', POPT_ARG_NONE, &text, 0, "Text header", NULL },
         { "env", 'e', POPT_ARG_NONE, &env, 0, "Dump environment", NULL },
         { "log", 'l', POPT_ARG_NONE, &log, 0, "Log", NULL },
         { "out-file", 'o', POPT_ARG_STRING, &outfile, 0, "Outfile", "filename" },
         { "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "Quiet", NULL },
         { "debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug", NULL },
         POPT_AUTOHELP { NULL, 0, 0, NULL, 0, NULL, NULL }
      };
      optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp(optCon, "[outfile]");
      int c;
      if ((c = poptGetNextOpt(optCon)) < -1)
         errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));
      if (!outfile && poptPeekArg(optCon))
         outfile = poptGetArg(optCon);

      poptFreeContext(optCon);
   }
   if (!header && !info && !cookie && !formdata)
      header = info = cookie = formdata = 1;    // Default
   int flags = (noclean ? JCGI_NOCLEAN : 0) + (nojson ? JCGI_NOJSON : 0) + (notmp ? JCGI_NOTMP : 0) + (jsontmp ? JCGI_JSONTMP : 0) + (jsonerr ? JCGI_JSONERR : 0);
   j_t j = j_create();
   char *e = j_cgi_get(info ? j_make(j, "info") : NULL, formdata ? j_make(j, "formdata") : NULL, cookie ? j_make(j, "cookie") : NULL, header ? j_make(j, "header") : NULL, "JCGITEST", flags);
   if (e)
   {
      if (text)
         printf("Status: 500\r\n");
      warnx("JCGI: %s", e);
   }
   if (text)
   {
      printf("Content-Type: text/plain; charset=\"utf-8\"\r\n\r\n");
      if (e)
         printf("Failed: %s\n\n", e);
   }
   if (log)
      j_log(debug, "jcgi", j_get(j, "info.script_name"), j, NULL);
   j_t o = j;
   if (j_len(j) == 1)
      o = j_first(j);           // Only one thing asked for
   if (!quiet)
   {
      FILE *of = stdout;
      if (outfile && strcmp(outfile, "-"))
         of = fopen(outfile, "w");
      if (!of)
         err(1, "Cannot open %s", outfile);
      if (text)
         j_err(j_write_pretty(o, of));
      else
         j_err(j_write(o, of));
      if (outfile)
         fclose(of);
   }
   j_delete(&j);
   if (env)
   {
      printf("\n");
      for (char **e = environ; *e; e++)
         printf("%s\n", *e);
   }
   return 0;
}
#endif
