/* Duplicate a bounded initial segment of a string, with out-of-memory
   checking.
   Copyright (C) 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "xstrndup.h"

#include "strndup.h"
#include "xalloc.h"

/* Return a newly allocated copy of at most N bytes of STRING.
   In other words, return a copy of the initial segment of length N of
   STRING.  */
char *
xstrndup (const char *string, size_t n)
{
  char *s = strndup (string, n);
  if (! s)
    xalloc_die ();
  return s;
}
