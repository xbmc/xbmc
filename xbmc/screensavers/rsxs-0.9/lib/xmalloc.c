/* xmalloc.c -- malloc with out of memory checking

   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

/* 1 if calloc is known to be compatible with GNU calloc.  This
   matters if we are not also using the calloc module, which defines
   HAVE_CALLOC and supports the GNU API even on non-GNU platforms.  */
#if defined HAVE_CALLOC || defined __GLIBC__
enum { HAVE_GNU_CALLOC = 1 };
#else
enum { HAVE_GNU_CALLOC = 0 };
#endif

/* Allocate an array of N objects, each with S bytes of memory,
   dynamically, with error checking.  S must be nonzero.  */

static inline void *
xnmalloc_inline (size_t n, size_t s)
{
  void *p;
  if (xalloc_oversized (n, s) || (! (p = malloc (n * s)) && n != 0))
    xalloc_die ();
  return p;
}

void *
xnmalloc (size_t n, size_t s)
{
  return xnmalloc_inline (n, s);
}

/* Allocate N bytes of memory dynamically, with error checking.  */

void *
xmalloc (size_t n)
{
  return xnmalloc_inline (n, 1);
}

/* Change the size of an allocated block of memory P to an array of N
   objects each of S bytes, with error checking.  S must be nonzero.  */

static inline void *
xnrealloc_inline (void *p, size_t n, size_t s)
{
  if (xalloc_oversized (n, s) || (! (p = realloc (p, n * s)) && n != 0))
    xalloc_die ();
  return p;
}

void *
xnrealloc (void *p, size_t n, size_t s)
{
  return xnrealloc_inline (p, n, s);
}

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  */

void *
xrealloc (void *p, size_t n)
{
  return xnrealloc_inline (p, n, 1);
}


/* If P is null, allocate a block of at least *PN such objects;
   otherwise, reallocate P so that it contains more than *PN objects
   each of S bytes.  *PN must be nonzero unless P is null, and S must
   be nonzero.  Set *PN to the new number of objects, and return the
   pointer to the new block.  *PN is never set to zero, and the
   returned pointer is never null.

   Repeated reallocations are guaranteed to make progress, either by
   allocating an initial block with a nonzero size, or by allocating a
   larger block.

   In the following implementation, nonzero sizes are doubled so that
   repeated reallocations have O(N log N) overall cost rather than
   O(N**2) cost, but the specification for this function does not
   guarantee that sizes are doubled.

   Here is an example of use:

     int *p = NULL;
     size_t used = 0;
     size_t allocated = 0;

     void
     append_int (int value)
       {
	 if (used == allocated)
	   p = x2nrealloc (p, &allocated, sizeof *p);
	 p[used++] = value;
       }

   This causes x2nrealloc to allocate a block of some nonzero size the
   first time it is called.

   To have finer-grained control over the initial size, set *PN to a
   nonzero value before calling this function with P == NULL.  For
   example:

     int *p = NULL;
     size_t used = 0;
     size_t allocated = 0;
     size_t allocated1 = 1000;

     void
     append_int (int value)
       {
	 if (used == allocated)
	   {
	     p = x2nrealloc (p, &allocated1, sizeof *p);
	     allocated = allocated1;
	   }
	 p[used++] = value;
       }

   */

static inline void *
x2nrealloc_inline (void *p, size_t *pn, size_t s)
{
  size_t n = *pn;

  if (! p)
    {
      if (! n)
	{
	  /* The approximate size to use for initial small allocation
	     requests, when the invoking code specifies an old size of
	     zero.  64 bytes is the largest "small" request for the
	     GNU C library malloc.  */
	  enum { DEFAULT_MXFAST = 64 };

	  n = DEFAULT_MXFAST / s;
	  n += !n;
	}
    }
  else
    {
      if (SIZE_MAX / 2 / s < n)
	xalloc_die ();
      n *= 2;
    }

  *pn = n;
  return xrealloc (p, n * s);
}

void *
x2nrealloc (void *p, size_t *pn, size_t s)
{
  return x2nrealloc_inline (p, pn, s);
}

/* If P is null, allocate a block of at least *PN bytes; otherwise,
   reallocate P so that it contains more than *PN bytes.  *PN must be
   nonzero unless P is null.  Set *PN to the new block's size, and
   return the pointer to the new block.  *PN is never set to zero, and
   the returned pointer is never null.  */

void *
x2realloc (void *p, size_t *pn)
{
  return x2nrealloc_inline (p, pn, 1);
}

/* Allocate S bytes of zeroed memory dynamically, with error checking.
   There's no need for xnzalloc (N, S), since it would be equivalent
   to xcalloc (N, S).  */

void *
xzalloc (size_t s)
{
  return memset (xmalloc (s), 0, s);
}

/* Allocate zeroed memory for N elements of S bytes, with error
   checking.  S must be nonzero.  */

void *
xcalloc (size_t n, size_t s)
{
  void *p;
  /* Test for overflow, since some calloc implementations don't have
     proper overflow checks.  But omit overflow and size-zero tests if
     HAVE_GNU_CALLOC, since GNU calloc catches overflow and never
     returns NULL if successful.  */
  if ((! HAVE_GNU_CALLOC && xalloc_oversized (n, s))
      || (! (p = calloc (n, s)) && (HAVE_GNU_CALLOC || n != 0)))
    xalloc_die ();
  return p;
}

/* Clone an object P of size S, with error checking.  There's no need
   for xnmemdup (P, N, S), since xmemdup (P, N * S) works without any
   need for an arithmetic overflow check.  */

void *
xmemdup (void const *p, size_t s)
{
  return memcpy (xmalloc (s), p, s);
}

/* Clone STRING.  */

char *
xstrdup (char const *string)
{
  return xmemdup (string, strlen (string) + 1);
}
