/*
 * h.c -- Handle allocation module
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: h.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides a simple API to allocate and free handles
 *	It maintains a dynamic array of pointers. These usually point to
 *	per-handle structures.
 */

/********************************* Includes ***********************************/

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/********************************** Defines ***********************************/
/*
 *	The handle list stores the length of the list and the number of used
 *	handles in the first two words.  These are hidden from the caller by
 *	returning a pointer to the third word to the caller
 */

#define H_LEN		0		/* First entry holds length of list */
#define H_USED		1		/* Second entry holds number of used */
#define H_OFFSET	2		/* Offset to real start of list */

#define H_INCR		16		/* Grow handle list in chunks this size */

/*********************************** Code *************************************/
/*
 *	Allocate a new file handle.  On the first call, the caller must set the
 *	handle map to be a pointer to a null pointer.  *map points to the second
 *	element in the handle array.
 */

#ifdef B_STATS
int HALLOC(B_ARGS_DEC, void ***map)
#else
int hAlloc(void ***map)
#endif
{
	int		*mp;
	int		handle, len, memsize, incr;

	a_assert(map);

	if (*map == NULL) {
		incr = H_INCR;
		memsize = (incr + H_OFFSET) * sizeof(void**);
#ifdef B_STATS
		if ((mp = (int*) balloc(B_ARGS, memsize)) == NULL) {
#else
		if ((mp = (int*) balloc(B_L, memsize)) == NULL) {
#endif
			return -1;
		}
		memset(mp, 0, memsize);
		mp[H_LEN] = incr;
		mp[H_USED] = 0;
		*map = (void**) &mp[H_OFFSET];
	} else {
		mp = &((*(int**)map)[-H_OFFSET]);
	}

	len = mp[H_LEN];

/*
 *	Find the first null handle
 */
	if (mp[H_USED] < mp[H_LEN]) {
		for (handle = 0; handle < len; handle++) {
			if (mp[handle+H_OFFSET] == 0) {
				mp[H_USED]++;
				return handle;
			}
		}
	} else {
		handle = len;
	}

/*
 *	No free handle so grow the handle list. Grow list in chunks of H_INCR.
 */
	len += H_INCR;
	memsize = (len + H_OFFSET) * sizeof(void**);
	if ((mp = (int*) brealloc(B_L, (void*) mp, memsize)) == NULL) {
		return -1;
	}
	*map = (void**) &mp[H_OFFSET];
	mp[H_LEN] = len;
	memset(&mp[H_OFFSET + len - H_INCR], 0, sizeof(int*) * H_INCR);
	mp[H_USED]++;
	return handle;
}

/******************************************************************************/
/*
 *	Free a handle.  This function returns the value of the largest
 *	handle in use plus 1, to be saved as a max value.
 */

int hFree(void ***map, int handle)
{
	int		*mp;
	int		len;

	a_assert(map);
	mp = &((*(int**)map)[-H_OFFSET]);
	a_assert(mp[H_LEN] >= H_INCR);

	a_assert(mp[handle + H_OFFSET]);
	a_assert(mp[H_USED]);
	mp[handle + H_OFFSET] = 0;
	if (--(mp[H_USED]) == 0) {
		bfree(B_L, (void*) mp);
		*map = NULL;
	}

/*
 *	Find the greatest handle number in use.
 */
	if (*map == NULL) {
		handle = -1;
	} else {
		len = mp[H_LEN];
		if (mp[H_USED] < mp[H_LEN]) {
			for (handle = len - 1; handle >= 0; handle--) {
				if (mp[handle + H_OFFSET])
					break;
			}
		} else {
			handle = len;
		}
	}
	return handle + 1;
}

/******************************************************************************/
/*
 *	Allocate an entry in the halloc array.
 */

#ifdef B_STATS
int HALLOCENTRY(B_ARGS_DEC, void ***list, int *max, int size)
#else
int hAllocEntry(void ***list, int *max, int size)
#endif
{
	char_t	*cp;
	int		id;

	a_assert(list);
	a_assert(max);

#ifdef B_STATS
	if ((id = HALLOC(B_ARGS, (void***) list)) < 0) {
#else
	if ((id = hAlloc((void***) list)) < 0) {
#endif
		return -1;
	}

	if (size > 0) {
#ifdef B_STATS
		if ((cp = balloc(B_ARGS, size)) == NULL) {
#else
		if ((cp = balloc(B_L, size)) == NULL) {
#endif
			hFree(list, id);
			return -1;
		}
		a_assert(cp);
		memset(cp, 0, size);

		(*list)[id] = (void*) cp;
	}

	if (id >= *max) {
		*max = id + 1;
	}
	return id;
}

/******************************************************************************/


