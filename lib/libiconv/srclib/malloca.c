/* Safe automatic memory allocation.
   Copyright (C) 2003, 2006-2007 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

/* Specification.  */
#include "malloca.h"

/* The speed critical point in this file is freea() applied to an alloca()
   result: it must be fast, to match the speed of alloca().  The speed of
   mmalloca() and freea() in the other case are not critical, because they
   are only invoked for big memory sizes.  */

#if HAVE_ALLOCA

/* Store the mmalloca() results in a hash table.  This is needed to reliably
   distinguish a mmalloca() result and an alloca() result.

   Although it is possible that the same pointer is returned by alloca() and
   by mmalloca() at different times in the same application, it does not lead
   to a bug in freea(), because:
     - Before a pointer returned by alloca() can point into malloc()ed memory,
       the function must return, and once this has happened the programmer must
       not call freea() on it anyway.
     - Before a pointer returned by mmalloca() can point into the stack, it
       must be freed.  The only function that can free it is freea(), and
       when freea() frees it, it also removes it from the hash table.  */

#define MAGIC_NUMBER 0x1415fb4a
#define MAGIC_SIZE sizeof (int)
/* This is how the header info would look like without any alignment
   considerations.  */
struct preliminary_header { void *next; char room[MAGIC_SIZE]; };
/* But the header's size must be a multiple of sa_alignment_max.  */
#define HEADER_SIZE \
  (((sizeof (struct preliminary_header) + sa_alignment_max - 1) / sa_alignment_max) * sa_alignment_max)
struct header { void *next; char room[HEADER_SIZE - sizeof (struct preliminary_header) + MAGIC_SIZE]; };
/* Verify that HEADER_SIZE == sizeof (struct header).  */
typedef int verify1[2 * (HEADER_SIZE == sizeof (struct header)) - 1];
/* We make the hash table quite big, so that during lookups the probability
   of empty hash buckets is quite high.  There is no need to make the hash
   table resizable, because when the hash table gets filled so much that the
   lookup becomes slow, it means that the application has memory leaks.  */
#define HASH_TABLE_SIZE 257
static void * mmalloca_results[HASH_TABLE_SIZE];

#endif

void *
mmalloca (size_t n)
{
#if HAVE_ALLOCA
  /* Allocate one more word, that serves as an indicator for malloc()ed
     memory, so that freea() of an alloca() result is fast.  */
  size_t nplus = n + HEADER_SIZE;

  if (nplus >= n)
    {
      char *p = (char *) malloc (nplus);

      if (p != NULL)
	{
	  size_t slot;

	  p += HEADER_SIZE;

	  /* Put a magic number into the indicator word.  */
	  ((int *) p)[-1] = MAGIC_NUMBER;

	  /* Enter p into the hash table.  */
	  slot = (unsigned long) p % HASH_TABLE_SIZE;
	  ((struct header *) (p - HEADER_SIZE))->next = mmalloca_results[slot];
	  mmalloca_results[slot] = p;

	  return p;
	}
    }
  /* Out of memory.  */
  return NULL;
#else
# if !MALLOC_0_IS_NONNULL
  if (n == 0)
    n = 1;
# endif
  return malloc (n);
#endif
}

#if HAVE_ALLOCA
void
freea (void *p)
{
  /* mmalloca() may have returned NULL.  */
  if (p != NULL)
    {
      /* Attempt to quickly distinguish the mmalloca() result - which has
	 a magic indicator word - and the alloca() result - which has an
	 uninitialized indicator word.  It is for this test that sa_increment
	 additional bytes are allocated in the alloca() case.  */
      if (((int *) p)[-1] == MAGIC_NUMBER)
	{
	  /* Looks like a mmalloca() result.  To see whether it really is one,
	     perform a lookup in the hash table.  */
	  size_t slot = (unsigned long) p % HASH_TABLE_SIZE;
	  void **chain = &mmalloca_results[slot];
	  for (; *chain != NULL;)
	    {
	      if (*chain == p)
		{
		  /* Found it.  Remove it from the hash table and free it.  */
		  char *p_begin = (char *) p - HEADER_SIZE;
		  *chain = ((struct header *) p_begin)->next;
		  free (p_begin);
		  return;
		}
	      chain = &((struct header *) ((char *) *chain - HEADER_SIZE))->next;
	    }
	}
      /* At this point, we know it was not a mmalloca() result.  */
    }
}
#endif
