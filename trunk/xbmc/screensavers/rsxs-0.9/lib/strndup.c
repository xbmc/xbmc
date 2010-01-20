/* Copyright (C) 1996, 1997, 1998, 2001, 2002, 2003, 2005, 2006 Free
   Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C Library.
   Bugs can be reported to bug-glibc@prep.ai.mit.edu.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

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
#if !_LIBC
# include "strndup.h"
#endif

#include <stdlib.h>
#include <string.h>

#if !_LIBC
# include "strnlen.h"
# ifndef __strnlen
#  define __strnlen strnlen
# endif
#endif

#undef __strndup
#if _LIBC
# undef strndup
#endif

#ifndef weak_alias
# define __strndup strndup
#endif

char *
__strndup (s, n)
     const char *s;
     size_t n;
{
  size_t len = __strnlen (s, n);
  char *new = malloc (len + 1);

  if (new == NULL)
    return NULL;

  new[len] = '\0';
  return memcpy (new, s, len);
}
#ifdef libc_hidden_def
libc_hidden_def (__strndup)
#endif
#ifdef weak_alias
weak_alias (__strndup, strndup)
#endif
