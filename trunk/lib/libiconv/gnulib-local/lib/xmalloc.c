/* xmalloc.c -- malloc with out of memory checking
   Copyright (C) 1990-1996, 2000-2003, 2005-2007 Free Software Foundation, Inc.

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

#include <stdlib.h>

#include "error.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Exit value when the requested amount of memory is not available.
   The caller may set it to some other value.  */
int xmalloc_exit_failure = EXIT_FAILURE;

void
xalloc_die ()
{
  error (xmalloc_exit_failure, 0, _("memory exhausted"));
  /* The `noreturn' cannot be given to error, since it may return if
     its first argument is 0.  To help compilers understand the
     xalloc_die does terminate, call exit. */
  exit (EXIT_FAILURE);
}

static void *
fixup_null_alloc (size_t n)
{
  void *p;

  p = NULL;
  if (n == 0)
    p = malloc ((size_t) 1);
  if (p == NULL)
    xalloc_die ();
  return p;
}

/* Allocate N bytes of memory dynamically, with error checking.  */

void *
xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    p = fixup_null_alloc (n);
  return p;
}

/* Allocate memory for NMEMB elements of SIZE bytes, with error checking.
   SIZE must be > 0.  */

void *
xnmalloc (size_t nmemb, size_t size)
{
  size_t n;
  void *p;

  if (xalloc_oversized (nmemb, size))
    xalloc_die ();
  n = nmemb * size;
  p = malloc (n);
  if (p == NULL)
    p = fixup_null_alloc (n);
  return p;
}

/* Allocate SIZE bytes of memory dynamically, with error checking,
   and zero it.  */

void *
xzalloc (size_t size)
{
  void *p;

  p = xmalloc (size);
  memset (p, 0, size);
  return p;
}

/* Allocate memory for N elements of S bytes, with error checking,
   and zero it.  */

void *
xcalloc (size_t n, size_t s)
{
  void *p;

  p = calloc (n, s);
  if (p == NULL)
    p = fixup_null_alloc (n);
  return p;
}

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.
   If P is NULL, run xmalloc.  */

void *
xrealloc (void *p, size_t n)
{
  if (p == NULL)
    return xmalloc (n);
  p = realloc (p, n);
  if (p == NULL)
    p = fixup_null_alloc (n);
  return p;
}
