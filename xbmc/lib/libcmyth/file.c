/*
 *  Copyright (C) 2004-2006, Eric Lund
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
#include <errno.h>
#include <mvp_refmem.h>
#include <cmyth.h>
#include <cmyth_local.h>

/*
 * cmyth_file_destroy(cmyth_file_t file)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Tear down and release storage associated with a file connection.
 * This should only be called by ref_release().  All others
 * should call ref_release() to release a file connection.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_file_destroy(cmyth_file_t file)
{
	int err, count;
	int r;
	long c;
	char msg[256];

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!file) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return;
	}
	if (file->file_control) {
		pthread_mutex_lock(&mutex);

		/*
		 * Try to shut down the file transfer.  Can't do much
		 * if it fails other than log it.
		 */
		snprintf(msg, sizeof(msg),
			 "QUERY_FILETRANSFER %ld[]:[]DONE", file->file_id);

		if ((err = cmyth_send_message(file->file_control, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, err);
			goto fail;
		}

		if ((count = cmyth_rcv_length(file->file_control)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_length() failed (%d)\n",
				  __FUNCTION__, count);
			err = count;
			goto fail;
		}
		if ((r = cmyth_rcv_long(file->file_control,
					&err, &c, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_long() failed (%d)\n",
				  __FUNCTION__, r);
			goto fail;
		}
	    fail:
		ref_release(file->file_control);
		pthread_mutex_unlock(&mutex);
	}
	if (file->closed_callback) {
	    (file->closed_callback)(file);
	}
	if (file->file_data) {
		ref_release(file->file_data);
	}

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * cmyth_file_set_closed_callback(cmyth_file_t file, void (*callback)(cmyth_file_t))
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Sets a callback which will be called when a file connection has been
 * signalled as done. Passing in NULL means no callback.
 */

void cmyth_file_set_closed_callback(cmyth_file_t file, void (*callback)(cmyth_file_t))
{
    if(!file)
	return;
    file->closed_callback = callback;
}

/*
 * cmyth_file_create(cmyth_conn_t control)
 * 
 * Scope: PRIVATE (mapped to __cmyth_file_create)
 *
 * Description
 *
 * Allocate and initialize a cmyth_file_t structure.  This should only
 * be called by cmyth_connect_file(), which establishes a file
 * transfer connection.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_file_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_file_t
 */
cmyth_file_t
cmyth_file_create(cmyth_conn_t control)
{
	cmyth_file_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
 	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
 		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_file_destroy);

	ret->file_control = ref_hold(control);
	ret->file_data = NULL;
	ret->file_id = -1;
	ret->file_start = 0;
	ret->file_length = 0;
	ret->file_pos = 0;
	ret->closed_callback = NULL;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_file_data(cmyth_file_t p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain a held reference to the data connection inside of a file
 * connection.  This cmyth_conn_t can be used to read data from the
 * MythTV backend server during a file transfer.
 *
 * Return Value:
 *
 * Sucess: A non-null cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL
 */
cmyth_conn_t
cmyth_file_data(cmyth_file_t file)
{
	if (!file) {
		return NULL;
	}
	if (!file->file_data) {
		return NULL;
	}
	return ref_hold(file->file_data);
}

/*
 * cmyth_file_control(cmyth_file_t p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain a held reference to the control connection inside of a file
 * connection.  This cmyth_conn_t can be used to control the
 * MythTV backend server during a file transfer.
 *
 * Return Value:
 *
 * Sucess: A non-null cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL
 */
cmyth_conn_t
cmyth_file_control(cmyth_file_t file)
{
	if (!file) {
		return NULL;
	}
	if (!file->file_control) {
		return NULL;
	}
	return ref_hold(file->file_control);
}

/*
 * cmyth_file_start(cmyth_file_t p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the start offset in a file for the beginning of the data.
 *
 * Return Value:
 *
 * Sucess: a long long value >= 0
 *
 * Failure: a long long containing -errno
 */
unsigned long long
cmyth_file_start(cmyth_file_t file)
{
	if (!file) {
		return -EINVAL;
	}
	return file->file_start;
}

/*
 * cmyth_file_length(cmyth_file_t p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the length of the data in a file.
 *
 * Return Value:
 *
 * Sucess: a long long value >= 0
 *
 * Failure: a long long containing -errno
 */
unsigned long long
cmyth_file_length(cmyth_file_t file)
{
	if (!file) {
		return -EINVAL;
	}
	return file->file_length;
}

/*
 * cmyth_file_get_block(cmyth_file_t file, char *buf, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 *
 * Sucess: number of bytes read into buf
 *
 * Failure: -1
 */
int
cmyth_file_get_block(cmyth_file_t file, char *buf, unsigned long len)
{
	struct timeval tv;
	fd_set fds;

	if (file == NULL || file->file_data == NULL)
		return -EINVAL;

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(file->file_data->conn_fd, &fds);
	if (select(file->file_data->conn_fd+1, NULL, &fds, NULL, &tv) == 0) {
		file->file_data->conn_hang = 1;
		return 0;
	} else {
		file->file_data->conn_hang = 0;
	}
	return read(file->file_data->conn_fd, buf, len);
}

int
cmyth_file_select(cmyth_file_t file, struct timeval *timeout)
{
	fd_set fds;
	int fd, ret;

	if (file == NULL || file->file_data == NULL)
		return -EINVAL;

	fd = file->file_data->conn_fd;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select(fd+1, &fds, NULL, NULL, timeout);

	if (ret == 0)
		file->file_data->conn_hang = 1;
	else
		file->file_data->conn_hang = 0;

	return ret;
}

/*
 * cmyth_file_request_block(cmyth_file_t file, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int
cmyth_file_request_block(cmyth_file_t file, unsigned long len)
{
	int err, count;
	int r;
	long c, ret;
	char msg[256];

	if (!file) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_FILETRANSFER %ld[]:[]REQUEST_BLOCK[]:[]%ld",
		 file->file_id, len);

	if ((err = cmyth_send_message(file->file_control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	if ((count=cmyth_rcv_length(file->file_control)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}
	if ((r=cmyth_rcv_long(file->file_control, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	file->file_pos += c;
	ret = c;

    out:
	pthread_mutex_unlock(&mutex);

	return ret;
}

/*
 * cmyth_file_seek(cmyth_file_t file, long long offset, int whence)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	SEEK_SET
 *		The offset is set to offset bytes.
 *	SEEK_CUR
 *		The offset is set to the current position plus offset bytes.
 *	SEEK_END
 *		The offset is set to the size of the file minus offset bytes.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
long long
cmyth_file_seek(cmyth_file_t file, long long offset, int whence)
{
	char msg[128];
	int err;
	int count;
	long long c;
	long r;
	long long ret;

	if (file == NULL)
		return -EINVAL;

	if ((offset == 0) && (whence == SEEK_CUR))
		return file->file_pos;

	pthread_mutex_lock(&mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_FILETRANSFER %ld[]:[]SEEK[]:[]%ld[]:[]%ld[]:[]%d[]:[]%ld[]:[]%ld",
		 file->file_id,
		 (long)(offset >> 32),
		 (long)(offset & 0xffffffff),
		 whence,
		 (long)(file->file_pos >> 32),
		 (long)(file->file_pos & 0xffffffff));

	if ((err = cmyth_send_message(file->file_control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	if ((count=cmyth_rcv_length(file->file_control)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}
	if ((r=cmyth_rcv_long_long(file->file_control, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long_long() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	switch (whence) {
	case SEEK_SET:
		file->file_pos = offset;
		break;
	case SEEK_CUR:
		file->file_pos += offset;
		break;
	case SEEK_END:
		file->file_pos = file->file_length - offset;
		break;
	}

	ret = file->file_pos;

    out:
	pthread_mutex_unlock(&mutex);
	
	return ret;
}
