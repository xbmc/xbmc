/*
 *  Copyright (C) 2005-2012, Jon Gettler
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

cmyth_commbreaklist_t
cmyth_get_commbreaklist(cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	unsigned int len = CMYTH_UTC_LEN + CMYTH_LONGLONG_LEN + 19;
	int err;
	int count;
	char *buf;
	int r;

	cmyth_commbreaklist_t breaklist = cmyth_commbreaklist_create();

	buf = alloca(len);
	if (!buf) {
		return breaklist;
	}

	sprintf(buf,"%s %ld %i", "QUERY_COMMBREAK", prog->proginfo_chanId, 
	        (int)cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts));
	pthread_mutex_lock(&mutex);
	if ((err = cmyth_send_message(conn, buf)) < 0) {
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

	if ((r = cmyth_rcv_commbreaklist(conn, &err, breaklist, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}

	out:
	pthread_mutex_unlock(&mutex);
	return breaklist;
}

cmyth_commbreaklist_t
cmyth_get_cutlist(cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	unsigned int len = CMYTH_UTC_LEN + CMYTH_LONGLONG_LEN + 17;
	int err;
	int count;
	char *buf;
	int r;

	cmyth_commbreaklist_t breaklist = cmyth_commbreaklist_create();

	buf = alloca(len);
	if (!buf) {
		return breaklist;
	}

	sprintf(buf,"%s %ld %i", "QUERY_CUTLIST", prog->proginfo_chanId, 
	        (int)cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts));

	if ((err = cmyth_send_message(conn, buf)) < 0) {
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

	if ((r = cmyth_rcv_commbreaklist(conn, &err, breaklist, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}

	out:
	pthread_mutex_unlock(&mutex);
	return breaklist;
}

int cmyth_rcv_commbreaklist(cmyth_conn_t conn, int *err, 
			cmyth_commbreaklist_t breaklist, int count)
{
	int consumed;
	int total = 0;
	long rows;
	int64_t mark;
	long long start = -1;
	char *failed = NULL;
	cmyth_commbreak_t commbreak;
	unsigned short type;
	unsigned short start_type;
	int i;

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

	if (rows < 0) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: no commercial breaks found.\n",
			__FUNCTION__);
		return 0;
	}

	for (i = 0; i < rows; i++) {
		consumed = cmyth_rcv_ushort(conn, err, &type, count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_ushort";
			goto fail;
		}

		consumed = cmyth_rcv_int64(conn, err, &mark, count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_long long";
			goto fail;
		}
		if (type == CMYTH_COMMBREAK_START || type == CMYTH_CUTLIST_START) {
			start = mark;
			start_type = type;
		} else if (type == CMYTH_COMMBREAK_END || type == CMYTH_CUTLIST_END) {
			if (start >= 0 &&
			    ((type == CMYTH_COMMBREAK_END && start_type == CMYTH_COMMBREAK_START)
			     || (type == CMYTH_CUTLIST_END && start_type == CMYTH_CUTLIST_START)))
			{
				commbreak = cmyth_commbreak_create();
				commbreak->start_mark = start;
				commbreak->end_mark = mark;
				start = -1;
				breaklist->commbreak_list = realloc(breaklist->commbreak_list,
					(++breaklist->commbreak_count) * sizeof(cmyth_commbreak_t));
				breaklist->commbreak_list[breaklist->commbreak_count - 1] = commbreak;
			} else {
				cmyth_dbg(CMYTH_DBG_WARN,
					"%s: ignoring 'end' marker without a 'start' marker at %lld\n",
					__FUNCTION__, type, mark);
			}
		} else {
				cmyth_dbg(CMYTH_DBG_WARN,
					"%s: type (%d) is not a COMMBREAK or CUTLIST\n",
					__FUNCTION__, type);
		}
	}

	/*
	 * If the last entry is a start marker then it doesn't have an associated end marker. In this
	 * case we choose to simply ignore it. Another option is to put in a really large fake end marker
	 * but that may cause strange seek behaviour in a client application.
	 */

	return total;

	fail:
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: %s() failed (%d)\n",
		__FUNCTION__, failed, *err);
	return total;
}

cmyth_commbreaklist_t
cmyth_mysql_get_commbreaklist(cmyth_database_t db, cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	cmyth_commbreaklist_t breaklist = cmyth_commbreaklist_create();
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	int r;

	cmyth_timestamp_to_display_string(start_ts_dt, prog->proginfo_rec_start_ts, 0);
	pthread_mutex_lock(&mutex);
	if ((r=cmyth_mysql_get_commbreak_list(db, prog->proginfo_chanId, start_ts_dt, breaklist, conn->conn_version)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_mysql_get_commbreak_list() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}

	fprintf(stderr, "Found %li commercial breaks for current program.\n", breaklist->commbreak_count);
	if (r != breaklist->commbreak_count) {
		fprintf(stderr, "commbreak error.  Setting number of commercial breaks to zero\n");
		cmyth_dbg(CMYTH_DBG_ERROR, "%s  - returned rows=%d commbreak_count=%li\n",__FUNCTION__, r,breaklist->commbreak_count);
		breaklist->commbreak_count = 0;
	}
	out:
	pthread_mutex_unlock(&mutex);
	return breaklist;
}
