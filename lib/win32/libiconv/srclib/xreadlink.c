/* xreadlink.c -- readlink wrapper to return the link name in malloc'd storage

   Copyright (C) 2001, 2003-2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering <jim@meyering.net>
   and Bruno Haible <bruno@clisp.org>.  */

#include <config.h>

/* Specification.  */
#include "xreadlink.h"

#include <errno.h>

#include "areadlink.h"
#include "xalloc.h"

/* Call readlink to get the symbolic link value of FILENAME.
   Return a pointer to that NUL-terminated string in malloc'd storage.
   If readlink fails, return NULL and set errno.
   If realloc fails, or if the link value is longer than SIZE_MAX :-),
   give a diagnostic and exit.  */

char *
xreadlink (char const *filename)
{
  char *result = areadlink (filename);
  if (result == NULL && errno == ENOMEM)
    xalloc_die ();
  return result;
}
