/* malloc with out of memory checking.
   Copyright (C) 2001-2004, 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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

#ifndef _XALLOC_H
#define _XALLOC_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Defined in xmalloc.c.  */

/* Allocate SIZE bytes of memory dynamically, with error checking.  */
extern void *xmalloc (size_t size);

/* Allocate memory for NMEMB elements of SIZE bytes, with error checking.
   SIZE must be > 0.  */
extern void *xnmalloc (size_t nmemb, size_t size);

/* Allocate SIZE bytes of memory dynamically, with error checking,
   and zero it.  */
extern void *xzalloc (size_t size);

/* Allocate memory for NMEMB elements of SIZE bytes, with error checking,
   and zero it.  */
extern void *xcalloc (size_t nmemb, size_t size);

/* Change the size of an allocated block of memory PTR to SIZE bytes,
   with error checking.  If PTR is NULL, run xmalloc.  */
extern void *xrealloc (void *ptr, size_t size);
#ifdef __cplusplus
}
template <typename T>
  inline T * xrealloc (T * ptr, size_t size)
  {
    return (T *) xrealloc ((void *) ptr, size);
  }
extern "C" {
#endif

/* This function is always triggered when memory is exhausted.  It is
   in charge of honoring the three previous items.  This is the
   function to call when one wants the program to die because of a
   memory allocation failure.  */
extern void xalloc_die (void)
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)) && !__STRICT_ANSI__
     __attribute__ ((__noreturn__))
#endif
     ;

/* In the following macros, T must be an elementary or structure/union or
   typedef'ed type, or a pointer to such a type.  To apply one of the
   following macros to a function pointer or array type, you need to typedef
   it first and use the typedef name.  */

/* Allocate an object of type T dynamically, with error checking.  */
/* extern T *XMALLOC (typename T); */
#define XMALLOC(T) \
  ((T *) xmalloc (sizeof (T)))

/* Allocate memory for NMEMB elements of type T, with error checking.  */
/* extern T *XNMALLOC (size_t nmemb, typename T); */
#if HAVE_INLINE
/* xnmalloc performs a division and multiplication by sizeof (T).  Arrange to
   perform the division at compile-time and the multiplication with a factor
   known at compile-time.  */
# define XNMALLOC(N,T) \
   ((T *) (sizeof (T) == 1 \
	   ? xmalloc (N) \
	   : xnboundedmalloc(N, (size_t) (sizeof (ptrdiff_t) <= sizeof (size_t) ? -1 : -2) / sizeof (T), sizeof (T))))
static inline void *
xnboundedmalloc (size_t n, size_t bound, size_t s)
{
  if (n > bound)
    xalloc_die ();
  return xmalloc (n * s);
}
#else
# define XNMALLOC(N,T) \
   ((T *) (sizeof (T) == 1 ? xmalloc (N) : xnmalloc (N, sizeof (T))))
#endif

/* Allocate an object of type T dynamically, with error checking,
   and zero it.  */
/* extern T *XZALLOC (typename T); */
#define XZALLOC(T) \
  ((T *) xzalloc (sizeof (T)))

/* Allocate memory for NMEMB elements of type T, with error checking,
   and zero it.  */
/* extern T *XCALLOC (size_t nmemb, typename T); */
#define XCALLOC(N,T) \
  ((T *) xcalloc (N, sizeof (T)))

/* Return a pointer to a new buffer of N bytes.  This is like xmalloc,
   except it returns char *.  */
#define xcharalloc(N) \
  XNMALLOC (N, char)


/* Defined in xstrdup.c.  */

/* Return a newly allocated copy of the N bytes of memory starting at P.  */
extern void *xmemdup (const void *p, size_t n);
#ifdef __cplusplus
}
template <typename T>
  inline T * xmemdup (const T * p, size_t n)
  {
    return (T *) xmemdup ((const void *) p, n);
  }
extern "C" {
#endif

/* Return a newly allocated copy of STRING.  */
extern char *xstrdup (const char *string);


/* Return 1 if an array of N objects, each of size S, cannot exist due
   to size arithmetic overflow.  S must be positive and N must be
   nonnegative.  This is a macro, not an inline function, so that it
   works correctly even when SIZE_MAX < N.

   By gnulib convention, SIZE_MAX represents overflow in size
   calculations, so the conservative dividend to use here is
   SIZE_MAX - 1, since SIZE_MAX might represent an overflowed value.
   However, malloc (SIZE_MAX) fails on all known hosts where
   sizeof (ptrdiff_t) <= sizeof (size_t), so do not bother to test for
   exactly-SIZE_MAX allocations on such hosts; this avoids a test and
   branch when S is known to be 1.  */
# define xalloc_oversized(n, s) \
    ((size_t) (sizeof (ptrdiff_t) <= sizeof (size_t) ? -1 : -2) / (s) < (n))


#ifdef __cplusplus
}
#endif


#endif /* _XALLOC_H */
