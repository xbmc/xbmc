/*
     This file is part of libmicrohttpd
     (C) 2007 Lymba

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
 * @file reason_phrase.c
 * @brief  Tables of the string response phrases
 * @author Elliot Glaysher
 * @author Christian Grothoff (minor code clean up)
 */

#include "reason_phrase.h"

#ifndef NULL
#define NULL (void*)0
#endif  // !NULL

static const char *invalid_hundred[] = { NULL };

static const char *one_hundred[] = {
  "Continue",
  "Switching Protocols",
  "Processing"
};

static const char *two_hundred[] = {
  "OK",
  "Created",
  "Accepted",
  "Non-Authoritative Information",
  "No Content",
  "Reset Content",
  "Partial Content",
  "Multi Status"
};

static const char *three_hundred[] = {
  "Multiple Choices",
  "Moved Permanently",
  "Moved Temporarily",
  "See Other",
  "Not Modified",
  "Use Proxy",
  "Switch Proxy",
  "Temporary Redirect"
};

static const char *four_hundred[] = {
  "Bad Request",
  "Unauthorized",
  "Payment Required",
  "Forbidden",
  "Not Found",
  "Method Not Allowed",
  "Not Acceptable",
  "Proxy Authentication Required",
  "Request Time-out",
  "Conflict",
  "Gone",
  "Length Required",
  "Precondition Failed",
  "Request Entity Too Large",
  "Request-URI Too Large",
  "Unsupported Media Type",
  "Requested Range Not Satisfiable",
  "Expectation Failed",
  "Unprocessable Entity",
  "Locked",
  "Failed Dependency",
  "Unordered Collection",
  "Upgrade Required",
  "Retry With"
};

static const char *five_hundred[] = {
  "Internal Server Error",
  "Not Implemented",
  "Bad Gateway",
  "Service Unavailable",
  "Gateway Time-out",
  "HTTP Version not supported",
  "Variant Also Negotiates",
  "Insufficient Storage",
  "Bandwidth Limit Exceeded",
  "Not Extended"
};


struct MHD_Reason_Block
{
  unsigned int max;
  const char **data;
};

#define BLOCK(m) { (sizeof(m) / sizeof(char*)), m }

static const struct MHD_Reason_Block reasons[] = {
  BLOCK (invalid_hundred),
  BLOCK (one_hundred),
  BLOCK (two_hundred),
  BLOCK (three_hundred),
  BLOCK (four_hundred),
  BLOCK (five_hundred),
};

const char *
MHD_get_reason_phrase_for (unsigned int code)
{
  if ((code >= 100 && code < 600) && (reasons[code / 100].max > code % 100))
    return reasons[code / 100].data[code % 100];
  return "Unknown";
}
