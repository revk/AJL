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

// This library allows C code to work as apache cgi, checking the environment and
// if necessary stdin for posted data. It uses the JSON library to provide the data
// and handle posted JSON.

char *j_cgi(j_p formdata, j_p cookie, j_p header, const char *session)
{                               // Fill in formdata, cookies, headers, and manage cookie session, return is NULL if OK, else error. All args can be NULL if not needed


   return "TODO";
}

char *h_parse_formdata(j_p j, const char *formdata)
{                               // Parse formdata url encoded to JSON object
   if (!j)
      return "NULL";


   return "TODO";
}
