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
 * freespace.c - functions to manage freespace structures.
 */
#include <stdlib.h>
#include <cmyth_local.h>

/*
 * cmyth_freespace_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a key frame structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_freespace_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_freespace_t
 */
cmyth_freespace_t
cmyth_freespace_create(void)
{
	cmyth_freespace_t ret = ref_alloc(sizeof(*ret));
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
 	if (!ret) {
	       return NULL;
	}

	ret->freespace_total = 0;
	ret->freespace_used = 0;
	return ret;
}
