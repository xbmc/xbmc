/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   Samba select/poll implementation
   Copyright (C) Andrew Tridgell 1992-1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

/* This is here because it allows us to avoid a nasty race in signal handling. 
   We need to guarantee that when we get a signal we get out of a select immediately
   but doing that involves a race condition. We can avoid the race by getting the 
   signal handler to write to a pipe that is in the select/poll list 

   This means all Samba signal handlers should call sys_select_signal().
*/

static pid_t initialised;
static int select_pipe[2];
static VOLATILE unsigned pipe_written, pipe_read;

/*******************************************************************
 Call this from all Samba signal handlers if you want to avoid a 
 nasty signal race condition.
********************************************************************/

void sys_select_signal(char c)
{
	if (!initialised) return;

	if (pipe_written > pipe_read+256) return;

	if (write(select_pipe[1], &c, 1) == 1) pipe_written++;
}

/*******************************************************************
 Like select() but avoids the signal race using a pipe
 it also guuarantees that fds on return only ever contains bits set
 for file descriptors that were readable.
********************************************************************/

int sys_select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *tval)
{
	int ret, saved_errno;
	fd_set *readfds2, readfds_buf;

	if (initialised != sys_getpid()) {
#ifdef _XBOX
		OutputDebugString("SMB -> Todo: sys_select\n");
#elif //_XBOX
		pipe(select_pipe);

		/*
		 * These next two lines seem to fix a bug with the Linux
		 * 2.0.x kernel (and probably other UNIXes as well) where
		 * the one byte read below can block even though the
		 * select returned that there is data in the pipe and
		 * the pipe_written variable was incremented. Thanks to
		 * HP for finding this one. JRA.
		 */

		if(set_blocking(select_pipe[0],0)==-1)
			smb_panic("select_pipe[0]: O_NONBLOCK failed.\n");
		if(set_blocking(select_pipe[1],0)==-1)
			smb_panic("select_pipe[1]: O_NONBLOCK failed.\n");
#endif //_XBOX
		initialised = sys_getpid();
	}

	maxfd = MAX(select_pipe[0]+1, maxfd);

	/* If readfds is NULL we need to provide our own set. */
	if (readfds) {
		readfds2 = readfds;
	} else {
		readfds2 = &readfds_buf;
		FD_ZERO(readfds2);
	}
	FD_SET(select_pipe[0], readfds2);

	errno = 0;
	ret = select(maxfd,readfds2,writefds,errorfds,tval);

	if (ret <= 0) {
		FD_ZERO(readfds2);
		if (writefds)
			FD_ZERO(writefds);
		if (errorfds)
			FD_ZERO(errorfds);
	} else if (FD_ISSET(select_pipe[0], readfds2)) {
		char c;
		saved_errno = errno;
		if (read(select_pipe[0], &c, 1) == 1) {
			pipe_read++;
			/* Mark Weaver <mark-clist@npsl.co.uk> pointed out a critical
			   fix to ensure we don't lose signals. We must always
			   return -1 when the select pipe is set, otherwise if another
			   fd is also ready (so ret == 2) then we used to eat the
			   byte in the pipe and lose the signal. JRA.
			*/
			ret = -1;
#if 0
			/* JRA - we can use this to debug the signal messaging... */
			DEBUG(0,("select got %u signal\n", (unsigned int)c));
#endif
			errno = EINTR;
		} else {
			FD_CLR(select_pipe[0], readfds2);
			ret--;
			errno = saved_errno;
		}
	}

	return ret;
}

/*******************************************************************
 Similar to sys_select() but catch EINTR and continue.
 This is what sys_select() used to do in Samba.
********************************************************************/

int sys_select_intr(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *tval)
{
	int ret;
	fd_set *readfds2, readfds_buf, *writefds2, writefds_buf, *errorfds2, errorfds_buf;
	struct timeval tval2, *ptval, end_time;

	readfds2 = (readfds ? &readfds_buf : NULL);
	writefds2 = (writefds ? &writefds_buf : NULL);
	errorfds2 = (errorfds ? &errorfds_buf : NULL);
	if (tval) {
		GetTimeOfDay(&end_time);
		end_time.tv_sec += tval->tv_sec;
		end_time.tv_usec += tval->tv_usec;
		end_time.tv_sec += end_time.tv_usec / 1000000;
		end_time.tv_usec %= 1000000;
		errno = 0;
		tval2 = *tval;
		ptval = &tval2;
	} else {
		ptval = NULL;
	}

	do {
		if (readfds)
			readfds_buf = *readfds;
		if (writefds)
			writefds_buf = *writefds;
		if (errorfds)
			errorfds_buf = *errorfds;
		if (ptval && (errno == EINTR)) {
			struct timeval now_time;
			SMB_BIG_INT tdif;

			GetTimeOfDay(&now_time);
			tdif = usec_time_diff(&end_time, &now_time);
			if (tdif <= 0) {
				ret = 0; /* time expired. */
				break;
			}
			ptval->tv_sec = tdif / 1000000;
			ptval->tv_usec = tdif % 1000000;
		}

		/* We must use select and not sys_select here. If we use
		   sys_select we'd lose the fact a signal occurred when sys_select
		   read a byte from the pipe. Fix from Mark Weaver
		   <mark-clist@npsl.co.uk>
		*/
		ret = select(maxfd, readfds2, writefds2, errorfds2, ptval);
	} while (ret == -1 && errno == EINTR);

	if (readfds)
		*readfds = readfds_buf;
	if (writefds)
		*writefds = writefds_buf;
	if (errorfds)
		*errorfds = errorfds_buf;

	return ret;
}
