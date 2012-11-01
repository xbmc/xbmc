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
 * keyframe.c - functions to manage key frame structures.  Mostly
 *              just allocating, freeing, and filling them out.
 */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cmyth_local.h>

/*
 * cmyth_keyframe_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a key frame structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_keyframe_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_keyframe_t
 */
cmyth_keyframe_t
cmyth_keyframe_create(void)
{
	cmyth_keyframe_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !\n", __FUNCTION__);
		return NULL;
	}
	ret->keyframe_number = 0;
	ret->keyframe_pos = 0;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_keyframe_fill(cmyth_keyframe_t kf,
 * 	               unsigned long keynum,
 *                     unsigned long long pos)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Fill out the contents of the recorder number structure 'rn' using
 * the values 'keynum' and 'pos'.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
cmyth_keyframe_t
cmyth_keyframe_fill(unsigned long keynum, unsigned long long pos)
{
	cmyth_keyframe_t ret = cmyth_keyframe_create();

	if (!ret) {
		return NULL;
	}

	ret->keyframe_number = keynum;
	ret->keyframe_pos = pos;
	return ret;
}

/*
 * cmyth_keyframe_string(cmyth_keyframe_t kf)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Compose a MythTV protocol string from a keyframe structure and
 * return a pointer to a malloc'ed buffer containing the string.
 *
 * Return Value:
 *
 * Success: A non-NULL malloc'ed character buffer pointer.
 *
 * Failure: NULL
 */
char *
cmyth_keyframe_string(cmyth_keyframe_t kf)
{
	unsigned len = sizeof("[]:[]");
	char key[32];
	char pos[32];
	char *ret;

	if (!kf) {
		return NULL;
	}
	sprintf(pos, "%lld", kf->keyframe_pos);
	len += strlen(pos);
	sprintf(key, "%ld", kf->keyframe_number);
	len += strlen(key);
	ret = malloc(len * sizeof(char));
	if (!ret) {
		return NULL;
	}
	strcpy(ret, key);
	strcat(ret, "[]:[]");
	strcat(ret, pos);
	return ret;
}
