/*
   Unix SMB/Netbios implementation.
   Version 3.0
   change notify handling - linux kernel based implementation
   Copyright (C) Andrew Tridgell 2000

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

#if HAVE_KERNEL_CHANGE_NOTIFY

#define FD_PENDING_SIZE 20
static SIG_ATOMIC_T fd_pending_array[FD_PENDING_SIZE];
static SIG_ATOMIC_T signals_received;

#ifndef DN_ACCESS
#define DN_ACCESS       0x00000001      /* File accessed in directory */
#define DN_MODIFY       0x00000002      /* File modified in directory */
#define DN_CREATE       0x00000004      /* File created in directory */
#define DN_DELETE       0x00000008      /* File removed from directory */
#define DN_RENAME       0x00000010      /* File renamed in directory */
#define DN_ATTRIB       0x00000020      /* File changed attribute */
#define DN_MULTISHOT    0x80000000      /* Don't remove notifier */
#endif


#ifndef RT_SIGNAL_NOTIFY
#define RT_SIGNAL_NOTIFY (SIGRTMIN+2)
#endif

#ifndef F_SETSIG
#define F_SETSIG 10
#endif

#ifndef F_NOTIFY
#define F_NOTIFY 1026
#endif

/****************************************************************************
 This is the structure to keep the information needed to
 determine if a directory has changed.
*****************************************************************************/

struct change_data {
	int directory_handle;
};

/****************************************************************************
 The signal handler for change notify.
 The Linux kernel has a bug in that we should be able to block any
 further delivery of RT signals until the kernel_check_notify() function
 unblocks them, but it seems that any signal mask we're setting here is
 being overwritten on exit from this handler. I should create a standalone
 test case for the kernel hackers. JRA.
*****************************************************************************/

static void signal_handler(int sig, siginfo_t *info, void *unused)
{
	if (signals_received < FD_PENDING_SIZE - 1) {
		fd_pending_array[signals_received] = (SIG_ATOMIC_T)info->si_fd;
		signals_received++;
	} /* Else signal is lost. */
	sys_select_signal(RT_SIGNAL_NOTIFY);
}

/****************************************************************************
 Check if a change notify should be issued.
 time non-zero means timeout check (used for hash). Ignore this (async method
 where time is zero will be used instead).
*****************************************************************************/

static BOOL kernel_check_notify(connection_struct *conn, uint16 vuid, char *path, uint32 flags, void *datap, time_t t)
{
	struct change_data *data = (struct change_data *)datap;
	int i;
	BOOL ret = False;

	if (t)
		return False;

	BlockSignals(True, RT_SIGNAL_NOTIFY);
	for (i = 0; i < signals_received; i++) {
		if (data->directory_handle == (int)fd_pending_array[i]) {
			DEBUG(3,("kernel_check_notify: kernel change notify on %s fd[%d]=%d (signals_received=%d)\n",
						path, i, (int)fd_pending_array[i], (int)signals_received ));

			close((int)fd_pending_array[i]);
			fd_pending_array[i] = (SIG_ATOMIC_T)-1;
			if (signals_received - i - 1) {
				memmove((void *)&fd_pending_array[i], (void *)&fd_pending_array[i+1],
						sizeof(SIG_ATOMIC_T)*(signals_received-i-1));
			}
			data->directory_handle = -1;
			signals_received--;
			ret = True;
			break;
		}
	}
	BlockSignals(False, RT_SIGNAL_NOTIFY);
	return ret;
}

/****************************************************************************
 Remove a change notify data structure.
*****************************************************************************/

static void kernel_remove_notify(void *datap)
{
	struct change_data *data = (struct change_data *)datap;
	int fd = data->directory_handle;
	if (fd != -1) {
		int i;
		BlockSignals(True, RT_SIGNAL_NOTIFY);
		for (i = 0; i < signals_received; i++) {
			if (fd == (int)fd_pending_array[i]) {
				fd_pending_array[i] = (SIG_ATOMIC_T)-1;
				if (signals_received - i - 1) {
					memmove((void *)&fd_pending_array[i], (void *)&fd_pending_array[i+1],
							sizeof(SIG_ATOMIC_T)*(signals_received-i-1));
				}
				data->directory_handle = -1;
				signals_received--;
				break;
			}
		}
		close(fd);
		BlockSignals(False, RT_SIGNAL_NOTIFY);
	}
	SAFE_FREE(data);
	DEBUG(3,("kernel_remove_notify: fd=%d\n", fd));
}

/****************************************************************************
 Register a change notify request.
*****************************************************************************/

static void *kernel_register_notify(connection_struct *conn, char *path, uint32 flags)
{
	struct change_data data;
	int fd;
	unsigned long kernel_flags;
	
	fd = sys_open(path,O_RDONLY, 0);

	if (fd == -1) {
		DEBUG(3,("Failed to open directory %s for change notify\n", path));
		return NULL;
	}

	if (sys_fcntl_long(fd, F_SETSIG, RT_SIGNAL_NOTIFY) == -1) {
		DEBUG(3,("Failed to set signal handler for change notify\n"));
		return NULL;
	}

	kernel_flags = DN_CREATE|DN_DELETE|DN_RENAME; /* creation/deletion changes everything! */
	if (flags & FILE_NOTIFY_CHANGE_FILE)        kernel_flags |= DN_MODIFY;
	if (flags & FILE_NOTIFY_CHANGE_DIR_NAME)    kernel_flags |= DN_RENAME|DN_DELETE;
	if (flags & FILE_NOTIFY_CHANGE_ATTRIBUTES)  kernel_flags |= DN_ATTRIB;
	if (flags & FILE_NOTIFY_CHANGE_SIZE)        kernel_flags |= DN_MODIFY;
	if (flags & FILE_NOTIFY_CHANGE_LAST_WRITE)  kernel_flags |= DN_MODIFY;
	if (flags & FILE_NOTIFY_CHANGE_LAST_ACCESS) kernel_flags |= DN_ACCESS;
	if (flags & FILE_NOTIFY_CHANGE_CREATION)    kernel_flags |= DN_CREATE;
	if (flags & FILE_NOTIFY_CHANGE_SECURITY)    kernel_flags |= DN_ATTRIB;
	if (flags & FILE_NOTIFY_CHANGE_EA)          kernel_flags |= DN_ATTRIB;
	if (flags & FILE_NOTIFY_CHANGE_FILE_NAME)   kernel_flags |= DN_RENAME|DN_DELETE;

	if (sys_fcntl_long(fd, F_NOTIFY, kernel_flags) == -1) {
		DEBUG(3,("Failed to set async flag for change notify\n"));
		return NULL;
	}

	data.directory_handle = fd;

	DEBUG(3,("kernel change notify on %s (ntflags=0x%x flags=0x%x) fd=%d\n", 
		 path, (int)flags, (int)kernel_flags, fd));

	return (void *)memdup(&data, sizeof(data));
}

/****************************************************************************
 See if the kernel supports change notify.
****************************************************************************/

static BOOL kernel_notify_available(void) 
{
	int fd, ret;
	fd = open("/tmp", O_RDONLY);
	if (fd == -1)
		return False; /* uggh! */
	ret = sys_fcntl_long(fd, F_NOTIFY, 0);
	close(fd);
	return ret == 0;
}

/****************************************************************************
 Setup kernel based change notify.
****************************************************************************/

struct cnotify_fns *kernel_notify_init(void) 
{
	static struct cnotify_fns cnotify;
        struct sigaction act;

	ZERO_STRUCT(act);

	act.sa_sigaction = signal_handler;
	act.sa_flags = SA_SIGINFO;
	sigemptyset( &act.sa_mask );
	if (sigaction(RT_SIGNAL_NOTIFY, &act, NULL) != 0) {
		DEBUG(0,("Failed to setup RT_SIGNAL_NOTIFY handler\n"));
		return NULL;
	}

	if (!kernel_notify_available())
		return NULL;

	cnotify.register_notify = kernel_register_notify;
	cnotify.check_notify = kernel_check_notify;
	cnotify.remove_notify = kernel_remove_notify;
	cnotify.select_time = -1;
	cnotify.notification_fd = -1;

	/* the signal can start off blocked due to a bug in bash */
	BlockSignals(False, RT_SIGNAL_NOTIFY);

	return &cnotify;
}

#else
 void notify_kernel_dummy(void);

 void notify_kernel_dummy(void) {}
#endif /* HAVE_KERNEL_CHANGE_NOTIFY */
