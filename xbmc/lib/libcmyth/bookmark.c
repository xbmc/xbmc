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


long long cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	char *buf;
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_LONGLONG_LEN + 18;
	int err;
	long long ret;
	int count;
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	cmyth_datetime_to_string(start_ts_dt, prog->proginfo_rec_start_ts);
	buf = alloca(len);
	if (!buf) {
		return -ENOMEM;
	}
	sprintf(buf,"%s %ld %s","QUERY_BOOKMARK",prog->proginfo_chanId,
		start_ts_dt);
	pthread_mutex_lock(&mutex);
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		ret = err;
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
		ret = count;
		goto out;
	}
	long long ll;
	int r;
	if ((r=cmyth_rcv_long_long(conn, &err, &ll, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_longlong() failed (%d)\n",
			__FUNCTION__, r);
		ret = err;
		goto out;
	}

	ret = ll;
   out:
	pthread_mutex_unlock(&mutex);
	return ret;
}
	
int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog, long long bookmark)
{
	char *buf;
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_LONGLONG_LEN + 18;
	char resultstr[3];
	int r,err;
	int ret;
	int count;
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	cmyth_datetime_to_string(start_ts_dt, prog->proginfo_rec_start_ts);
	buf = alloca(len);
	if (!buf) {
		return -ENOMEM;
	}
	sprintf(buf,"%s %ld %s %lld %lld","SET_BOOKMARK",prog->proginfo_chanId,
		start_ts_dt, bookmark >> 32,(bookmark & 0xffffffff));
	pthread_mutex_lock(&mutex);
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		ret = err;
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if ((r=cmyth_rcv_string(conn,&err,resultstr,sizeof(resultstr),count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, count);
		ret = count;
		goto out;
	}
	ret = strcmp(resultstr,"OK");
   out:
	pthread_mutex_unlock(&mutex);
	return ret;
}
