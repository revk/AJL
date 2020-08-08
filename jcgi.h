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
//
// Formdata:-
// For query string, or posted form data, each tag is a field in JSON, value is string or array if repeated tag
// For multipart posted, any uploaded file is done as an object in the JSON with details of file, and where tmp file stored
// For multipart uploaded JSON the JSON is also read in to the object
// For posted JSON, formdata is the posted JSON
// Temp files deleted on exit, so need renaming, or hard link if to be retained as files

#include "ajl.h"
#define	JCGI_NOTMP	1       // Don't make tmp files, just put raw data in "data":
#define	JCGI_NOCLEAN	2       // Don't clean up tmp files on exit
#define	JCGI_NOJSON	4       // Don't load JSON objects
char *j_cgi(j_t info, j_t formdata, j_t cookie, j_t header, const char *session, int flags);
char *j_parse_formdata_sep(j_t, const char *, char sep);
#define j_parse_formdata(j,f) j_parse_formdata_sep(j,f,'&')
