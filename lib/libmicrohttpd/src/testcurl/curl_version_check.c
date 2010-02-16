/*
     This file is part of libmicrohttpd
     (C) 2007 Christian Grothoff

     libmicrohttpd is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
     option) any later version.

     libmicrohttpd is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libmicrohttpd; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file curl_version_check.c
 * @brief  verify required cURL version is available to run tests
 * @author Sagie Amir
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

static int
parse_version_number (const char **s)
{
  int i = 0;
  char num[17];

  while (i < 16 && ((**s >= '0') & (**s <= '9')))
    {
      num[i] = **s;
      (*s)++;
      i++;
    }

  num[i] = '\0';

  return atoi (num);
}

const char *
parse_version_string (const char *s, int *major, int *minor, int *micro)
{
  if (!s)
    return NULL;
  *major = parse_version_number (&s);
  if (!s || *s != '.')
    return NULL;
  s++;
  *minor = parse_version_number (&s);
  if (*s != '.')
    return NULL;
  s++;
  *micro = parse_version_number (&s);
  return s;
}


/*
 * check local libcurl version matches required version
 */
int
curl_check_version (const char *req_version)
{
  const char *ver;
  const char *curl_ver;
#if HTTPS_SUPPORT
  const char *ssl_ver;
  const char *req_ssl_ver;
#endif

  int loc_major, loc_minor, loc_micro;
  int rq_major, rq_minor, rq_micro;

  ver = curl_version ();
#if HAVE_MESSAGES
  fprintf (stderr, "curl version: %s\n", ver);
#endif
  /*
   * this call relies on the cURL string to be of the exact following format :
   * 'libcurl/7.16.4 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/0.6.5' OR
   * 'libcurl/7.18.2 GnuTLS/2.4.0 zlib/1.2.3.3 libidn/0.6.5'
   */
  curl_ver = strchr (ver, '/') + 1;

  /* Parse version numbers */
  parse_version_string (req_version, &rq_major, &rq_minor, &rq_micro);
  parse_version_string (curl_ver, &loc_major, &loc_minor, &loc_micro);

  /* Compare version numbers.  */
  if ((loc_major > rq_major
       || (loc_major == rq_major && loc_minor > rq_minor)
       || (loc_major == rq_major && loc_minor == rq_minor
           && loc_micro > rq_micro) || (loc_major == rq_major
                                        && loc_minor == rq_minor
                                        && loc_micro == rq_micro)) == 0)
    {
      fprintf (stderr,
               "Error: running curl test depends on local libcurl version > %s\n",
               req_version);
      return -1;
    }

  /*
   * enforce required gnutls/openssl version.
   * TODO use curl version string to assert use of gnutls
   */
#if HTTPS_SUPPORT
  ssl_ver = strchr (curl_ver, ' ') + 1;

  if (strncmp ("GnuTLS", ssl_ver, strlen ("GNUtls")) == 0)
    {
      ssl_ver = strchr (ssl_ver, '/') + 1;
      req_ssl_ver = MHD_REQ_CURL_GNUTLS_VERSION;
    }
  else if (strncmp ("OpenSSL", ssl_ver, strlen ("OpenSSL")) == 0)
    {
      ssl_ver = strchr (ssl_ver, '/') + 1;
      req_ssl_ver = MHD_REQ_CURL_OPENSSL_VERSION;
    }
  else
    {
      fprintf (stderr, "Error: unrecognized curl ssl library\n");
      return -1;
    }

  parse_version_string (req_ssl_ver, &rq_major, &rq_minor, &rq_micro);
  parse_version_string (ssl_ver, &loc_major, &loc_minor, &loc_micro);

  if ((loc_major > rq_major
       || (loc_major == rq_major && loc_minor > rq_minor)
       || (loc_major == rq_major && loc_minor == rq_minor
           && loc_micro > rq_micro) || (loc_major == rq_major
                                        && loc_minor == rq_minor
                                        && loc_micro == rq_micro)) == 0)
    {
      fprintf (stderr,
               "Error: running curl test depends on local libcurl SSL version > %s\n",
               req_ssl_ver);
      return -1;
    }
#endif
  return 0;
}
