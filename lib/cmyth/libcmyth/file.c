/*
 *  Copyright (C) 2004-2012, Eric Lund
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
#include <inttypes.h>
#include <sys/types.h>
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
	int err;
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

		if ((err=cmyth_rcv_okay(file->file_control, "ok")) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_okay() failed (%d)\n",
				  __FUNCTION__, err);
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
	ret->file_req = 0;
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
	int ret;

	if (file == NULL || file->file_data == NULL)
		return -EINVAL;

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(file->file_data->conn_fd, &fds);
	if (select((int)(file->file_data->conn_fd)+1, NULL, &fds, NULL, &tv) == 0) {
		file->file_data->conn_hang = 1;
		return 0;
	} else {
		file->file_data->conn_hang = 0;
	}

	ret = recv(file->file_data->conn_fd, buf, len, 0);

	if(ret < 0)
		return ret;

	file->file_pos += ret;

	if(file->file_pos > file->file_length)
		file->file_length = file->file_pos;

	return ret;
}

int
cmyth_file_select(cmyth_file_t file, struct timeval *timeout)
{
	fd_set fds;
	int ret;
	cmyth_socket_t fd;

	if (file == NULL || file->file_data == NULL)
		return -EINVAL;

	fd = file->file_data->conn_fd;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select((int)fd+1, &fds, NULL, NULL, timeout);

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

	if(len > (unsigned int)file->file_control->conn_tcp_rcvbuf)
		len = (unsigned int)file->file_control->conn_tcp_rcvbuf;

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

	file->file_req += c;
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
	int64_t c;
	long r;
	long long ret;

	if (file == NULL)
		return -EINVAL;

	if ((offset == 0) && (whence == SEEK_CUR))
		return file->file_pos;

	if ((offset == file->file_pos) && (whence == SEEK_SET))
		return file->file_pos;

	while(file->file_pos < file->file_req) {
		c = file->file_req - file->file_pos;
		if(c > sizeof(msg))
			c = sizeof(msg);

		if (cmyth_file_get_block(file, msg, (unsigned long)c) < 0)
			return -1;
	}

	pthread_mutex_lock(&mutex);

	if (file->file_control->conn_version >= 66) {
		/*
		 * Since protocol 66 mythbackend expects to receive a single 64 bit integer rather than
		 * two 32 bit hi and lo integers.
		 */
		snprintf(msg, sizeof(msg),
			 "QUERY_FILETRANSFER %ld[]:[]SEEK[]:[]%"PRIu64"[]:[]%d[]:[]%"PRIu64,
			 file->file_id,
			 (int64_t)offset,
			 whence,
			 (int64_t)file->file_pos);
	}
	else {
		snprintf(msg, sizeof(msg),
			 "QUERY_FILETRANSFER %ld[]:[]SEEK[]:[]%d[]:[]%d[]:[]%d[]:[]%d[]:[]%d",
			 file->file_id,
			 (int32_t)(offset >> 32),
			 (int32_t)(offset & 0xffffffff),
			 whence,
			 (int32_t)(file->file_pos >> 32),
			 (int32_t)(file->file_pos & 0xffffffff));
	}

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
	if ((r=cmyth_rcv_int64(file->file_control, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_int64() failed (%d)\n",
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

	file->file_req = file->file_pos;
	if(file->file_pos > file->file_length)
		file->file_length = file->file_pos;

	ret = file->file_pos;

    out:
	pthread_mutex_unlock(&mutex);
	
	return ret;
}

/*
 * cmyth_file_read(cmyth_recorder_t rec, char *buf, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Request and read a block of data from backend
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int cmyth_file_read(cmyth_file_t file, char *buf, unsigned long len)
{
	int err, count;
	int ret, req, nfds, rec;
	char *end, *cur;
	char msg[256];
	int64_t len64;
	struct timeval tv;
	fd_set fds;

	if (!file || !file->file_data) {
		cmyth_dbg (CMYTH_DBG_ERROR, "%s: no connection\n",
		           __FUNCTION__);
		return -EINVAL;
	}
	if (len == 0)
		return 0;

	pthread_mutex_lock (&mutex);

	/* make sure we have outstanding requests that fill the buffer that was called with */
	/* this way we should be able to saturate the network connection better */
	if (file->file_req < file->file_pos + len) {

		snprintf (msg, sizeof (msg),
	            "QUERY_FILETRANSFER %ld[]:[]REQUEST_BLOCK[]:[]%ld",
	            file->file_id, (unsigned long)(file->file_pos + len - file->file_req));

		if ( (err = cmyth_send_message (file->file_control, msg) ) < 0) {
			cmyth_dbg (CMYTH_DBG_ERROR,
			           "%s: cmyth_send_message() failed (%d)\n",
			           __FUNCTION__, err);
			ret = err;
			goto out;
		}
		req = 1;
	} else {
		req = 0;
	}

	rec = 0;
	cur = buf;
	end = buf+len;

	while (cur == buf || req || rec) {
		if(rec) {
			tv.tv_sec =  0;
			tv.tv_usec = 0;
		} else {
			tv.tv_sec = 20;
			tv.tv_usec = 0;
		}
		nfds = 0;

		FD_ZERO (&fds);
		if (req) {
			if ((int)file->file_control->conn_fd > nfds)
				nfds = (int)file->file_control->conn_fd;
			FD_SET (file->file_control->conn_fd, &fds);
		}
		if ((int)file->file_data->conn_fd > nfds)
			nfds = (int)file->file_data->conn_fd;
		FD_SET (file->file_data->conn_fd, &fds);

		if ((ret = select (nfds+1, &fds, NULL, NULL,&tv)) < 0) {
			cmyth_dbg (CMYTH_DBG_ERROR,
			           "%s: select(() failed (%d)\n",
			           __FUNCTION__, ret);
			goto out;
		}

		if (ret == 0 && !rec) {
			file->file_control->conn_hang = 1;
			file->file_data->conn_hang = 1;
			ret = -ETIMEDOUT;
			goto out;
		}

		/* check control connection */
		if (FD_ISSET(file->file_control->conn_fd, &fds)) {

			if ((count=cmyth_rcv_length (file->file_control)) < 0) {
				cmyth_dbg (CMYTH_DBG_ERROR,
				           "%s: cmyth_rcv_length() failed (%d)\n",
				           __FUNCTION__, count);
				ret = count;
				goto out;
			}

			/*
			 * MythTV originally sent back a signed 32bit value but was changed to a
			 * signed 64bit value in http://svn.mythtv.org/trac/changeset/18011 (1-Aug-2008).
			 *
			 * libcmyth now retrieves the 64-bit signed value, does error-checking,
			 * and then converts to a 32bit unsigned.
			 *
			 * This rcv_ method needs to be forced to use new_int64 to pull back a
			 * single 64bit number otherwise the handling in rcv_int64 will revert to
			 * the old two 32bit hi and lo long values.
			 */
			if ((ret=cmyth_rcv_new_int64 (file->file_control, &err, &len64, count, 1))< 0) {
				cmyth_dbg (CMYTH_DBG_ERROR,
				           "%s: cmyth_rcv_new_int64() failed (%d)\n",
				           __FUNCTION__, ret);
				ret = err;
				goto out;
			}
			if (len64 >= 0x100000000LL || len64 < 0) {
				/* -1 seems to be a common result, but isn't valid so use 0 instead. */
				cmyth_dbg (CMYTH_DBG_WARN,
				           "%s: cmyth_rcv_new_int64() returned out of bound value (%"PRId64"). Using 0 instead.\n",
				           __FUNCTION__, len64);
				len64 = 0;
			}
			len = (unsigned long)len64;
			req = 0;
			file->file_req += len;

			if (file->file_req < file->file_pos) {
				cmyth_dbg (CMYTH_DBG_ERROR,
				           "%s: received invalid invalid length, read position is ahead of request (req: %"PRIu64", pos: %"PRIu64", len: %"PRId64")\n",
				           __FUNCTION__, file->file_req, file->file_pos, len64);
				ret = -1;
				goto out;
			}


			/* check if we are already done */
			if (file->file_pos == file->file_req)
				break;
		}

		/* restore direct request fleg */
		rec = 0;

		/* check data connection */
		if (FD_ISSET(file->file_data->conn_fd, &fds)) {
			if (end < cur) {
				cmyth_dbg (CMYTH_DBG_ERROR,
				           "%s: positions invalid on read, bailing out (cur: %x, end: %x)\n",
				           __FUNCTION__, cur, end);
				ret = -1;
				goto out;
			}
			if ((ret = recv (file->file_data->conn_fd, cur, (int)(end-cur), 0)) < 0) {
				cmyth_dbg (CMYTH_DBG_ERROR,
				           "%s: recv() failed (%d)\n",
				           __FUNCTION__, ret);
				goto out;
			}
			cur += ret;
			file->file_pos += ret;
			if(ret)
				rec = 1; /* attempt to read directly again to get all queued packets */
		}
	}

	/* make sure file grows, as we move past length */
	if (file->file_pos > file->file_length)
		file->file_length = file->file_pos;

	ret = (int)(cur - buf);
out:
	pthread_mutex_unlock (&mutex);
	return ret;
}

#ifdef _MSC_VER

struct errentry {
    unsigned long oscode;           /* OS return value */
    int errnocode;  /* System V error code */
};

static struct errentry errtable[] = {
  {  ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
  {  ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
  {  ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
  {  ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
  {  ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
  {  ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
  {  ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
  {  ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
  {  ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
  {  ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
  {  ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
  {  ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
  {  ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
  {  ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
  {  ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
  {  ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
  {  ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
  {  ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
  {  ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
  {  ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
  {  ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
  {  ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
  {  ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
  {  ERROR_FAIL_I24,               EACCES    },  /* 83 */
  {  ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
  {  ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
  {  ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
  {  ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
  {  ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
  {  ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
  {  ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
  {  ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
  {  ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
  {  ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
  {  ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
  {  ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
  {  ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
  {  ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
  {  ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
  {  ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
  {  ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
  {  ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
  {  ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
  {  ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
  {  ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }    /* 1816 */
};

/* size of the table */
#define ERRTABLESIZE (sizeof(errtable)/sizeof(errtable[0]))

/* The following two constants must be the minimum and maximum
   values in the (contiguous) range of Exec Failure errors. */
#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN

/* These are the low and high value in the range of errors that are
   access violations */
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED


/***
 *void _dosmaperr(oserrno) - Map function number
 *
 *Purpose:
 *       This function takes an OS error number, and maps it to the
 *       corresponding errno value (based on UNIX System V values). The
 *       OS error number is stored in _doserrno (and the mapped value is
 *       stored in errno)
 *
 *Entry:
 *       ULONG oserrno = OS error value
 *
 *Exit:
 *       sets _doserrno and errno.
 *
 *Exceptions:
 *
 *******************************************************************************/

void __cdecl _dosmaperr (
  unsigned long oserrno
  )
{
  int i;
  
  _doserrno = oserrno;        /* set _doserrno */
  
  /* check the table for the OS error code */
  for (i = 0; i < ERRTABLESIZE; ++i) {
    if (oserrno == errtable[i].oscode) {
      errno = errtable[i].errnocode;
      return;
    }
  }
  
  /* The error code wasn't in the table.  We check for a range of */
  /* EACCES errors or exec failure errors (ENOEXEC).  Otherwise   */
  /* EINVAL is returned.                                          */
  
  if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
    errno = EACCES;
  else if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
    errno = ENOEXEC;
  else
    errno = EINVAL;
}

#endif

