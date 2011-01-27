/*
  $Id: isort.h,v 1.2 2005/01/07 02:42:29 rocky Exp $

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
} sort_link;

typedef struct sort_info {
  int16_t *vector;               /* vector (storage doesn't belong to us) */

  long  *abspos;                 /* pointer for side effects */
  long  size;                    /* vector size */

  long  maxsize;                 /* maximum vector size */

  long sortbegin;                /* range of contiguous sorted area */
  long lo,hi;                    /* current post, overlap range */
  int  val;                      /* ...and val */

  /* sort structs */
  sort_link **head;           /* sort buckets (65536) */

  long *bucketusage;          /*  of used buckets (65536) */
  long lastbucket;
  sort_link *revindex;

} sort_info_t;

extern sort_info_t *sort_alloc(long size);
extern void sort_unsortall(sort_info_t *i);
extern void sort_setup(sort_info_t *i,int16_t *vector,long *abspos,long size,
		       long sortlo, long sorthi);
extern void sort_free(sort_info_t *i);
extern sort_link *sort_getmatch(sort_info_t *i, long post, long overlap,
				int value);
extern sort_link *sort_nextmatch(sort_info_t *i, sort_link *prev);

#define is(i) (i->size)
#define ib(i) (*i->abspos)
#define ie(i) (i->size+*i->abspos)
#define iv(i) (i->vector)
#define ipos(i,l) (l-i->revindex)

#endif

