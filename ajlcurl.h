// ==========================================================================
//
//                       Adrian's JSON Library - ajlcurl.h
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
#pragma once
#include "ajl.h"
#include <curl/curl.h>

// These can be passed existing CURL or NULL to make one.
// Headers are not set unless bearer specified so can be pre-filled in curl
// tx is what to send, or NULL
// rx is what is received, or NULL. If response is text non json then json string returned (can reuse tx if required)
// Response is NULL is 2xx response with JSON, else malloc'd error string

char *j_curl(int type, CURL * crl, j_t tx, j_t rx, const char *bearer, const char *url, ...);
#define	j_curl_get(curl,tx,rx,bearer,...) j_curl(0,curl,tx,rx,bearer,__VA_ARGS__)       // GET, formdata
#define	j_curl_post(curl,tx,rx,bearer,...) j_curl(1,curl,tx,rx,bearer,__VA_ARGS__)      // POST, formdata
#define	j_curl_send(curl,tx,rx,bearer,...) j_curl(2,curl,tx,rx,bearer,__VA_ARGS__)      // POST, json
