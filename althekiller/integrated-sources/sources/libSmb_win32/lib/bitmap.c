/*
   Unix SMB/CIFS implementation.
   simple bitmap functions
   Copyright (C) Andrew Tridgell 1992-1998

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

/* these functions provide a simple way to allocate integers from a
   pool without repetition */

/****************************************************************************
allocate a bitmap of the specified size
****************************************************************************/
struct bitmap *bitmap_allocate(int n)
{
	struct bitmap *bm;

	bm = SMB_MALLOC_P(struct bitmap);

	if (!bm) return NULL;
	
	bm->n = n;
	bm->b = SMB_MALLOC_ARRAY(uint32, (n+31)/32);
	if (!bm->b) {
		SAFE_FREE(bm);
		return NULL;
	}

	memset(bm->b, 0, sizeof(uint32)*((n+31)/32));

	return bm;
}

/****************************************************************************
free a bitmap.
****************************************************************************/

void bitmap_free(struct bitmap *bm)
{
	if (!bm)
		return;

	SAFE_FREE(bm->b);
	SAFE_FREE(bm);
}

/****************************************************************************
talloc a bitmap
****************************************************************************/
struct bitmap *bitmap_talloc(TALLOC_CTX *mem_ctx, int n)
{
	struct bitmap *bm;

	if (!mem_ctx) return NULL;

	bm = TALLOC_P(mem_ctx, struct bitmap);

	if (!bm) return NULL;
	
	bm->n = n;
	bm->b = TALLOC_ARRAY(mem_ctx, uint32, (n+31)/32);
	if (!bm->b) {
		return NULL;
	}

	memset(bm->b, 0, sizeof(uint32)*((n+31)/32));

	return bm;
}

/****************************************************************************
copy as much of the source bitmap as will fit in the destination bitmap.
****************************************************************************/

int bitmap_copy(struct bitmap * const dst, const struct bitmap * const src)
{
        int count = MIN(dst->n, src->n);

        SMB_ASSERT(dst->b != src->b);
	memcpy(dst->b, src->b, sizeof(uint32)*((count+31)/32));

        return count;
}

/****************************************************************************
set a bit in a bitmap
****************************************************************************/
BOOL bitmap_set(struct bitmap *bm, unsigned i)
{
	if (i >= bm->n) {
		DEBUG(0,("Setting invalid bitmap entry %d (of %d)\n",
		      i, bm->n));
		return False;
	}
	bm->b[i/32] |= (1<<(i%32));
	return True;
}

/****************************************************************************
clear a bit in a bitmap
****************************************************************************/
BOOL bitmap_clear(struct bitmap *bm, unsigned i)
{
	if (i >= bm->n) {
		DEBUG(0,("clearing invalid bitmap entry %d (of %d)\n",
		      i, bm->n));
		return False;
	}
	bm->b[i/32] &= ~(1<<(i%32));
	return True;
}

/****************************************************************************
query a bit in a bitmap
****************************************************************************/
BOOL bitmap_query(struct bitmap *bm, unsigned i)
{
	if (i >= bm->n) return False;
	if (bm->b[i/32] & (1<<(i%32))) {
		return True;
	}
	return False;
}

/****************************************************************************
find a zero bit in a bitmap starting at the specified offset, with
wraparound
****************************************************************************/
int bitmap_find(struct bitmap *bm, unsigned ofs)
{
	unsigned int i, j;

	if (ofs > bm->n) ofs = 0;

	i = ofs;
	while (i < bm->n) {
		if (~(bm->b[i/32])) {
			j = i;
			do {
				if (!bitmap_query(bm, j)) return j;
				j++;
			} while (j & 31 && j < bm->n);
		}
		i += 32;
		i &= ~31;
	}

	i = 0;
	while (i < ofs) {
		if (~(bm->b[i/32])) {
			j = i;
			do {
				if (!bitmap_query(bm, j)) return j;
				j++;
			} while (j & 31 && j < bm->n);
		}
		i += 32;
		i &= ~31;
	}

	return -1;
}
