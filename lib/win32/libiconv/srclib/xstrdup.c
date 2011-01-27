/* xstrdup.c -- copy a string with out of memory checking
   Copyright (C) 1990, 1996, 2000-2003, 2005-2006 Free Software Foundation, Inc.

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

#include <config.h>

/* Specification.  */
#include "xalloc.h"

#include <string.h>

/* Return a newly allocated copy of the N bytes of memory starting at P.  */

void *
xmemdup (const void *p, size_t n)
{
  void *q = xmalloc (n);
  memcpy (q, p, n);
  return q;
}

/* Return a newly allocated copy of STRING.  */

char *
xstrdup (const char *string)
{
  return strcpy (XNMALLOC (strlen (string) + 1, char), string);
}
