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

cmyth_event_t
cmyth_event_get(cmyth_conn_t conn, char * data, int len)
{
	int count, err, consumed, i;
	char tmp[1024];
	cmyth_event_t event;
	cmyth_proginfo_t proginfo = NULL;

	if (conn == NULL)
		goto fail;

	if ((count=cmyth_rcv_length(conn)) <= 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		return CMYTH_EVENT_CLOSE;
	}

	consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
	count -= consumed;
	if (strcmp(tmp, "BACKEND_MESSAGE") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}

	consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
	count -= consumed;
	if (strcmp(tmp, "RECORDING_LIST_CHANGE") == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE;
	} else if (strncmp(tmp, "RECORDING_LIST_CHANGE ADD", 25) == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD;
		strncpy(data, tmp + 26, len);
	} else if (strcmp(tmp, "RECORDING_LIST_CHANGE UPDATE") == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE;
		/* receive a proginfo structure - do nothing with it (yet?)*/
		proginfo = cmyth_proginfo_create();
		if (!proginfo) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				"%s: cmyth_proginfo_create() failed\n",
				__FUNCTION__);
			goto fail;
		}
		consumed = cmyth_rcv_proginfo(conn, &err, proginfo, count);
		ref_release(proginfo);
		proginfo = NULL;
		count -= consumed;
	} else if (strncmp(tmp, "RECORDING_LIST_CHANGE DELETE", 28) == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE;
		strncpy(data, tmp + 29, len);
	} else if (strcmp(tmp, "SCHEDULE_CHANGE") == 0) {
		event = CMYTH_EVENT_SCHEDULE_CHANGE;
	} else if (strncmp(tmp, "DONE_RECORDING", 14) == 0) {
		event = CMYTH_EVENT_DONE_RECORDING;
	} else if (strncmp(tmp, "QUIT_LIVETV", 11) == 0) {
		event = CMYTH_EVENT_QUIT_LIVETV;
	} else if (strncmp(tmp, "LIVETV_WATCH", 12) == 0) {
		event = CMYTH_EVENT_WATCH_LIVETV;
		strncpy(data, tmp + 13, len);
	/* Sergio: Added to support the new live tv protocol */
	} else if (strncmp(tmp, "LIVETV_CHAIN UPDATE", 19) == 0) {
		event = CMYTH_EVENT_LIVETV_CHAIN_UPDATE;
		strncpy(data, tmp + 20, len);
	} else if (strncmp(tmp, "SIGNAL", 6) == 0) { 
		event = CMYTH_EVENT_SIGNAL; 
		/* get slock, signal, seen_pat, matching_pat */ 
		while (count > 0) { 
			/* get signalmonitorvalue name */ 
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count); 
			count -= consumed; 

			/* get signalmonitorvalue status */ 
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count); 
			count -= consumed; 
		}
	} else if (strncmp(tmp, "ASK_RECORDING", 13) == 0) {
		event = CMYTH_EVENT_ASK_RECORDING;
		if (cmyth_conn_get_protocol_version(conn) < 37) {
			/* receive 4 string - do nothing with them */
			for (i = 0; i < 4; i++) {
				consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) -1, count);
				count -= consumed;
			}
		} else {
			/* receive a proginfo structure - do nothing with it (yet?)*/
			proginfo = cmyth_proginfo_create();
			if (!proginfo) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					"%s: cmyth_proginfo_create() failed\n",
					__FUNCTION__);
				goto fail;
			}
			consumed = cmyth_rcv_proginfo(conn, &err, proginfo, count);
			ref_release(proginfo);
			proginfo = NULL;
			count -= consumed;
		}
	} else if (strncmp(tmp, "CLEAR_SETTINGS_CACHE", 20) == 0) {
		event = CMYTH_EVENT_CLEAR_SETTINGS_CACHE;
	} else if (strncmp(tmp, "GENERATED_PIXMAP", 16) == 0) {
		/* capture the file which a pixmap has been generated for */
		event = CMYTH_EVENT_GENERATED_PIXMAP;
		consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
		if (strncmp(tmp, "OK", 2) == 0) {
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
			strncpy(data, tmp, len);
		} else {
			data[0] = 0;
		}
	} else if (strncmp(tmp, "SYSTEM_EVENT", 12) == 0) {
		event = CMYTH_EVENT_SYSTEM_EVENT;
		strncpy(data, tmp + 13, len);
	} else if (strncmp(tmp, "UPDATE_FILE_SIZE", 16) == 0) {
		event = CMYTH_EVENT_UPDATE_FILE_SIZE;
		strncpy(data, tmp + 17, len);
	} else {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: unknown mythtv BACKEND_MESSAGE '%s'\n", __FUNCTION__, tmp);
		event = CMYTH_EVENT_UNKNOWN;
	}

	while(count > 0) {
		consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
		count -= consumed;
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: leftover data %s\n", __FUNCTION__, tmp);
	}

	return event;

 fail:
	return CMYTH_EVENT_UNKNOWN;
}

int
cmyth_event_select(cmyth_conn_t conn, struct timeval *timeout)
{
	fd_set fds;
	int ret;
	cmyth_socket_t fd;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n", __FUNCTION__,
				__FILE__, __LINE__);

	if (conn == NULL)
		return -EINVAL;

	fd = conn->conn_fd;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select((int)fd+1, &fds, NULL, NULL, timeout);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
				__FUNCTION__, __FILE__, __LINE__);

	return ret;
}

