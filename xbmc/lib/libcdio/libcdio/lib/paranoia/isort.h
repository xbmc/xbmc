/*
  $Id: isort.h,v 1.5 2005/10/17 20:56:51 pjcreath Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _ISORT_H_
#define _ISORT_H_

typedef struct sort_link{
  struct sort_link *next;
} sort_link_t;

typedef struct sort_info {
  int16_t *vector;               /* vector (storage doesn't belong to us) */

  long  *abspos;                 /* pointer for side effects */
  long  size;                    /* vector size */

  long  maxsize;                 /* maximum vector size */

  long sortbegin;                /* range of contiguous sorted area */
  long lo,hi;                    /* current post, overlap range */
  int  val;                      /* ...and val */

  /* sort structs */
  sort_link_t **head;           /* sort buckets (65536) */

  long *bucketusage;          /*  of used buckets (65536) */
  long lastbucket;
  sort_link_t *revindex;

} sort_info_t;

/*! ========================================================================
 * sort_alloc()
 *
 * Allocates and initializes a new, empty sort_info object, which can
 * be used to index up to (size) samples from a vector.
 */
extern sort_info_t *sort_alloc(long int size);

/*! ========================================================================
 * sort_unsortall() (internal)
 *
 * This function resets the index for further use with a different
 * vector or range, without the overhead of an unnecessary free/alloc.
 */
extern void sort_unsortall(sort_info_t *i);

/*! ========================================================================
 * sort_setup()
 *
 * This function initializes a previously allocated sort_info_t.  The
 * sort_info_t is associated with a vector of samples of length
 * (size), whose position begins at (*abspos) within the CD's stream
 * of samples.  Only the range of samples between (sortlo, sorthi)
 * will eventually be indexed for fast searching.  (sortlo, sorthi)
 * are absolute sample positions.
 *
 * ???: Why is abspos a pointer?  Why not just store a copy?
 *
 * Note: size *must* be <= the size given to the preceding sort_alloc(),
 * but no error checking is done here.
 */
extern void sort_setup(sort_info_t *i, int16_t *vector, long int *abspos,
		       long int size, long int sortlo, long int sorthi);

/* =========================================================================
 * sort_free()
 *
 * Releases all memory consumed by a sort_info object.
 */
extern void sort_free(sort_info_t *i);

/*! ========================================================================
 * sort_getmatch()
 *
 * This function returns a sort_link_t pointer which refers to the
 * first sample equal to (value) in the vector.  It only searches for
 * hits within (overlap) samples of (post), where (post) is an offset
 * within the vector.  The caller can determine the position of the
 * matched sample using ipos(sort_info *, sort_link *).
 *
 * This function returns NULL if no matches were found.
 */
extern sort_link_t *sort_getmatch(sort_info_t *i, long post, long overlap,
				  int value);

/*! ========================================================================
 * sort_nextmatch()
 *
 * This function returns a sort_link_t pointer which refers to the
 * next sample matching the criteria previously passed to
 * sort_getmatch().  See sort_getmatch() for details.
 *
 * This function returns NULL if no further matches were found.
 */
extern sort_link_t *sort_nextmatch(sort_info_t *i, sort_link_t *prev);

/* ===========================================================================
 * is()
 *
 * This macro returns the size of the vector indexed by the given sort_info_t.
 */
#define is(i) (i->size)

/* ===========================================================================
 * ib()
 *
 * This macro returns the absolute position of the first sample in the vector
 * indexed by the given sort_info_t.
 */
#define ib(i) (*i->abspos)

/* ===========================================================================
 * ie()
 *
 * This macro returns the absolute position of the sample after the last
 * sample in the vector indexed by the given sort_info_t.
 */
#define ie(i) (i->size+*i->abspos)

/* ===========================================================================
 * iv()
 *
 * This macro returns the vector indexed by the given sort_info_t.
 */
#define iv(i) (i->vector)

/* ===========================================================================
 * ipos()
 *
 * This macro returns the relative position (offset) within the indexed vector
 * at which the given match was found.
 *
 * It uses a little-known and frightening aspect of C pointer arithmetic:
 * subtracting a pointer is not an arithmetic subtraction, but rather the
 * additive inverse.  In other words, since
 *   q     = p + n returns a pointer to the nth object in p,
 *   q - p = p + n - p, and
 *   q - p = n, not the difference of the two addresses.
 */
#define ipos(i,l) (l-i->revindex)

#endif /* _ISORT_H_ */

