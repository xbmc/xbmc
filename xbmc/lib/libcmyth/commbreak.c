/*
 *  Copyright (C) 2005-2006, Jon Gettler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mvp_refmem.h>
#include <cmyth.h>
#include <cmyth_local.h>

static void
cmyth_commbreaklist_destroy(cmyth_commbreaklist_t cbl)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!cbl) {
		return;
	}
	for (i = 0; i < cbl->commbreak_count; ++i) {
		if (cbl->commbreak_list[i]) {
			ref_release(cbl->commbreak_list[i]);
		}
		cbl->commbreak_list[i] = NULL;
	}
	if (cbl->commbreak_list) {
		free(cbl->commbreak_list);
	}
}

cmyth_commbreaklist_t
cmyth_commbreaklist_create(void)
{
	cmyth_commbreaklist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_commbreaklist_destroy);

	ret->commbreak_list = NULL;
	ret->commbreak_count = 0;
	return ret;
}

void
cmyth_commbreak_destroy(cmyth_commbreak_t b)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!b) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
                return;
        }
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

cmyth_commbreak_t
cmyth_commbreak_create(void)
{
	cmyth_commbreak_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_commbreak_destroy);

	ret->start_mark = 0;
	ret->start_offset = 0;
	ret->end_mark = 0;
	ret->end_offset = 0;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

int cmyth_rcv_commbreaklist(cmyth_conn_t conn, int *err, 
			cmyth_commbreaklist_t breaklist, int count)
{
	int consumed;
	int total = 0;
	long rows;
	char *failed = NULL;
	cmyth_commbreak_t commbreak;
	unsigned short type;
	int i;
	int j;

	if (count <= 0) {
		*err = EINVAL;
		return 0;
	}

	/*
	 * Get number of rows
	 */
	consumed = cmyth_rcv_long(conn, err, &rows, count);
	count -= consumed;
	total += consumed;
	if (*err) {
		failed = "cmyth_rcv_long";
		goto fail;
	}

	/*
	 * Uneven row counts don't make sense!
	 */
	if ((rows % 2) != 0) {
		*err = EINVAL;
		return 0;
	} else {
		breaklist->commbreak_count = rows / 2;
	}

	breaklist->commbreak_list = malloc(breaklist->commbreak_count * 
					sizeof(cmyth_commbreak_t));
	if (!breaklist->commbreak_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			__FUNCTION__);
		*err = ENOMEM;
		return consumed;
	}
	memset(breaklist->commbreak_list, 0, breaklist->commbreak_count * sizeof(cmyth_commbreak_t));

	for (i = 0; i < breaklist->commbreak_count; i++) {
		commbreak = cmyth_commbreak_create();

		for (j = 0; j < 2; j++) {
			consumed = cmyth_rcv_ushort(conn, err, &type, count);
			count -= consumed;
			total += consumed;
			if (*err) {
				failed = "cmyth_rcv_ushort";
				goto fail;
			}
			/*
			 * Do a little sanity-checking.
			 */
			if (j == 0 && type != CMYTH_COMMBREAK_START) {
				return 0;
			} else if (j == 1 && type != CMYTH_COMMBREAK_END) {
				return 0;
			}

			if (j == 0) {
				consumed = cmyth_rcv_long_long(conn, err, &commbreak->start_offset, count);
			} else {
				consumed = cmyth_rcv_long_long(conn, err, &commbreak->end_offset, count);
			}	
			count -= consumed;
			total += consumed;
			if (*err) {
				failed = "cmyth_rcv_long";
				goto fail;
			}

			if (j == 0) {
				consumed = cmyth_rcv_long(conn, err, &commbreak->start_mark, count);
			} else {
				consumed = cmyth_rcv_long(conn, err, &commbreak->end_mark, count);
			}
				
			count -= consumed;
			total += consumed;
			if (*err) {
				failed = "cmyth_rcv_long";
				goto fail;
			}

		}

		breaklist->commbreak_list[i] = commbreak;
	}

	return total;

    fail:
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: %s() failed (%d)\n",
		__FUNCTION__, failed, *err);
	return total;
}

/*
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_LONGLONG_LEN + 18;
	int err;
	int count;
	char *buf;

        cmyth_datetime_to_string(start_ts_dt, prog->proginfo_start_ts, 0);
	buf = alloca(len);
	if (!buf) {
		return breaklist;
	}
	sprintf(buf,"%s %ld %s","QUERY_COMMBREAK",prog->proginfo_chanId,
		start_ts_dt);
	pthread_mutex_lock(&mutex);
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
		goto out;
	}

	int r;
	if ((r=cmyth_rcv_commbreaklist(conn, &err, breaklist, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}
*/

cmyth_commbreaklist_t
cmyth_get_commbreaklist(cmyth_database_t db, cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	cmyth_commbreaklist_t breaklist = cmyth_commbreaklist_create();
        char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	int r;

        cmyth_timestamp_to_display_string(start_ts_dt, prog->proginfo_rec_start_ts, 0);
	pthread_mutex_lock(&mutex);
	if ((r=cmyth_mysql_get_commbreak_list(db, prog->proginfo_chanId, start_ts_dt, breaklist)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_mysql_get_commbreak_list() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}

	fprintf(stderr, "Found %li commercial breaks for current program.\n", breaklist->commbreak_count);

	out:
	pthread_mutex_unlock(&mutex);
	return breaklist;
}
