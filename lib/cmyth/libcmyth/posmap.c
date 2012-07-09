/*
 *  Copyright (C) 2004-2010, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * posmap.c - functions to handle operations on MythTV position maps.
 *            A position map contains a list of key-frames each of
 *            which represents a indexed position in a recording
 *            stream.  These may be markers set by hand, or they may
 *            be markers inserted by commercial detection.  A position
 *            map collects these in one place.
 */
#include <stdlib.h>
#include <stdio.h>
#include <cmyth_local.h>

/*
 * cmyth_posmap_destroy(cmyth_posmap_t pm)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Clean up and free a position map structure.  This should only be done
 * by the ref_release() code.  Everyone else should call
 * ref_release() because position map structures are reference
 * counted.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_posmap_destroy(cmyth_posmap_t pm)
{
	unsigned int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!pm) {
		return;
	}
	if (pm->posmap_list) {
		for (i = 0; i < pm->posmap_count; ++i) {
			ref_release(pm->posmap_list[i]);
		}
		free(pm->posmap_list);
	}
}

/*
 * cmyth_posmap_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Allocate and initialize a position map structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_posmap_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_posmap_t
 */
cmyth_posmap_t
cmyth_posmap_create(void)
{
	cmyth_posmap_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_posmap_destroy);

	ret->posmap_count = 0;
	ret->posmap_list = NULL;
	return ret;
}
