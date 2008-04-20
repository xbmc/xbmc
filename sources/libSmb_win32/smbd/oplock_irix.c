/*
   Unix SMB/CIFS implementation.
   IRIX kernel oplock processing
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

#define DBGC_CLASS DBGC_LOCKING
#include "includes.h"

#if HAVE_KERNEL_OPLOCKS_IRIX

static int oplock_pipe_write = -1;
static int oplock_pipe_read = -1;

/****************************************************************************
 Test to see if IRIX kernel oplocks work.
****************************************************************************/

static BOOL irix_oplocks_available(void)
{
	int fd;
	int pfd[2];
	pstring tmpname;

	set_effective_capability(KERNEL_OPLOCK_CAPABILITY);

	slprintf(tmpname,sizeof(tmpname)-1, "%s/koplock.%d", lp_lockdir(), (int)sys_getpid());

	if(pipe(pfd) != 0) {
		DEBUG(0,("check_kernel_oplocks: Unable to create pipe. Error was %s\n",
			 strerror(errno) ));
		return False;
	}

	if((fd = sys_open(tmpname, O_RDWR|O_CREAT|O_EXCL|O_TRUNC, 0600)) < 0) {
		DEBUG(0,("check_kernel_oplocks: Unable to open temp test file %s. Error was %s\n",
			 tmpname, strerror(errno) ));
		unlink( tmpname );
		close(pfd[0]);
		close(pfd[1]);
		return False;
	}

	unlink(tmpname);

	if(sys_fcntl_long(fd, F_OPLKREG, pfd[1]) == -1) {
		DEBUG(0,("check_kernel_oplocks: Kernel oplocks are not available on this machine. \
Disabling kernel oplock support.\n" ));
		close(pfd[0]);
		close(pfd[1]);
		close(fd);
		return False;
	}

	if(sys_fcntl_long(fd, F_OPLKACK, OP_REVOKE) < 0 ) {
		DEBUG(0,("check_kernel_oplocks: Error when removing kernel oplock. Error was %s. \
Disabling kernel oplock support.\n", strerror(errno) ));
		close(pfd[0]);
		close(pfd[1]);
		close(fd);
		return False;
	}

	close(pfd[0]);
	close(pfd[1]);
	close(fd);

	return True;
}

/****************************************************************************
 * Deal with the IRIX kernel <--> smbd
 * oplock break protocol.
****************************************************************************/

static files_struct *irix_oplock_receive_message(fd_set *fds)
{
	extern int smb_read_error;
	oplock_stat_t os;
	char dummy;
	files_struct *fsp;

	/* Ensure we only get one call per select fd set. */
	FD_CLR(oplock_pipe_read, fds);

	/*
	 * Read one byte of zero to clear the
	 * kernel break notify message.
	 */

	if(read(oplock_pipe_read, &dummy, 1) != 1) {
		DEBUG(0,("irix_oplock_receive_message: read of kernel "
			 "notification failed. Error was %s.\n",
			 strerror(errno) ));
		smb_read_error = READ_ERROR;
		return NULL;
	}

	/*
	 * Do a query to get the
	 * device and inode of the file that has the break
	 * request outstanding.
	 */

	if(sys_fcntl_ptr(oplock_pipe_read, F_OPLKSTAT, &os) < 0) {
		DEBUG(0,("irix_oplock_receive_message: fcntl of kernel "
			 "notification failed. Error was %s.\n",
			 strerror(errno) ));
		if(errno == EAGAIN) {
			/*
			 * Duplicate kernel break message - ignore.
			 */
			return NULL;
		}
		smb_read_error = READ_ERROR;
		return NULL;
	}

	/*
	 * We only have device and inode info here - we have to guess that this
	 * is the first fsp open with this dev,ino pair.
	 */

	if ((fsp = file_find_di_first((SMB_DEV_T)os.os_dev,
				      (SMB_INO_T)os.os_ino)) == NULL) {
		DEBUG(0,("irix_oplock_receive_message: unable to find open "
			 "file with dev = %x, inode = %.0f\n",
			 (unsigned int)os.os_dev, (double)os.os_ino ));
		return NULL;
	}
     
	DEBUG(5,("irix_oplock_receive_message: kernel oplock break request "
		 "received for dev = %x, inode = %.0f\n, file_id = %ul",
		 (unsigned int)fsp->dev, (double)fsp->inode, fsp->fh->file_id ));

	return fsp;
}

/****************************************************************************
 Attempt to set an kernel oplock on a file.
****************************************************************************/

static BOOL irix_set_kernel_oplock(files_struct *fsp, int oplock_type)
{
	if (sys_fcntl_long(fsp->fh->fd, F_OPLKREG, oplock_pipe_write) == -1) {
		if(errno != EAGAIN) {
			DEBUG(0,("irix_set_kernel_oplock: Unable to get kernel oplock on file %s, dev = %x, \
inode = %.0f, file_id = %ul. Error was %s\n", 
				 fsp->fsp_name, (unsigned int)fsp->dev, (double)fsp->inode, fsp->fh->file_id,
				 strerror(errno) ));
		} else {
			DEBUG(5,("irix_set_kernel_oplock: Refused oplock on file %s, fd = %d, dev = %x, \
inode = %.0f, file_id = %ul. Another process had the file open.\n",
				 fsp->fsp_name, fsp->fh->fd, (unsigned int)fsp->dev, (double)fsp->inode, fsp->fh->file_id ));
		}
		return False;
	}
	
	DEBUG(10,("irix_set_kernel_oplock: got kernel oplock on file %s, dev = %x, inode = %.0f, file_id = %ul\n",
		  fsp->fsp_name, (unsigned int)fsp->dev, (double)fsp->inode, fsp->fh->file_id));

	return True;
}

/****************************************************************************
 Release a kernel oplock on a file.
****************************************************************************/

static void irix_release_kernel_oplock(files_struct *fsp)
{
	if (DEBUGLVL(10)) {
		/*
		 * Check and print out the current kernel
		 * oplock state of this file.
		 */
		int state = sys_fcntl_long(fsp->fh->fd, F_OPLKACK, -1);
		dbgtext("irix_release_kernel_oplock: file %s, dev = %x, inode = %.0f file_id = %ul, has kernel \
oplock state of %x.\n", fsp->fsp_name, (unsigned int)fsp->dev,
                        (double)fsp->inode, fsp->fh->file_id, state );
	}

	/*
	 * Remove the kernel oplock on this file.
	 */
	if(sys_fcntl_long(fsp->fh->fd, F_OPLKACK, OP_REVOKE) < 0) {
		if( DEBUGLVL( 0 )) {
			dbgtext("irix_release_kernel_oplock: Error when removing kernel oplock on file " );
			dbgtext("%s, dev = %x, inode = %.0f, file_id = %ul. Error was %s\n",
				fsp->fsp_name, (unsigned int)fsp->dev, 
				(double)fsp->inode, fsp->fh->file_id, strerror(errno) );
		}
	}
}

/****************************************************************************
 See if there is a message waiting in this fd set.
 Note that fds MAY BE NULL ! If so we must do our own select.
****************************************************************************/

static BOOL irix_oplock_msg_waiting(fd_set *fds)
{
	int selrtn;
	fd_set myfds;
	struct timeval to;

	if (oplock_pipe_read == -1)
		return False;

	if (fds) {
		return FD_ISSET(oplock_pipe_read, fds);
	}

	/* Do a zero-time select. We just need to find out if there
	 * are any outstanding messages. We use sys_select_intr as
	 * we need to ignore any signals. */

	FD_ZERO(&myfds);
	FD_SET(oplock_pipe_read, &myfds);

	to = timeval_set(0, 0);
	selrtn = sys_select_intr(oplock_pipe_read+1,&myfds,NULL,NULL,&to);
	return (selrtn == 1) ? True : False;
}

/****************************************************************************
 Setup kernel oplocks.
****************************************************************************/

struct kernel_oplocks *irix_init_kernel_oplocks(void) 
{
	int pfd[2];
	static struct kernel_oplocks koplocks;

	if (!irix_oplocks_available())
		return NULL;

	if(pipe(pfd) != 0) {
		DEBUG(0,("setup_kernel_oplock_pipe: Unable to create pipe. Error was %s\n",
			 strerror(errno) ));
		return False;
	}

	oplock_pipe_read = pfd[0];
	oplock_pipe_write = pfd[1];

	koplocks.receive_message = irix_oplock_receive_message;
	koplocks.set_oplock = irix_set_kernel_oplock;
	koplocks.release_oplock = irix_release_kernel_oplock;
	koplocks.msg_waiting = irix_oplock_msg_waiting;
	koplocks.notification_fd = oplock_pipe_read;

	return &koplocks;
}
#else
 void oplock_irix_dummy(void) {}
#endif /* HAVE_KERNEL_OPLOCKS_IRIX */
