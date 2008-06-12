/* 
   Unix SMB/CIFS implementation.
   store smbd profiling information in shared memory
   Copyright (C) Andrew Tridgell 1999
   
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

#ifdef WITH_PROFILE
#define IPC_PERMS ((S_IRUSR | S_IWUSR) | S_IRGRP | S_IROTH)
#endif /* WITH_PROFILE */

#ifdef WITH_PROFILE
static int shm_id;
static BOOL read_only;
#if defined(HAVE_CLOCK_GETTIME)
clockid_t __profile_clock;
BOOL have_profiling_clock = False;
#endif
#endif

struct profile_header *profile_h;
struct profile_stats *profile_p;

BOOL do_profile_flag = False;
BOOL do_profile_times = False;

/****************************************************************************
receive a set profile level message
****************************************************************************/
void profile_message(int msg_type, struct process_id src, void *buf, size_t len)
{
        int level;

	memcpy(&level, buf, sizeof(int));
#ifdef WITH_PROFILE
	switch (level) {
	case 0:		/* turn off profiling */
		do_profile_flag = False;
		do_profile_times = False;
		DEBUG(1,("INFO: Profiling turned OFF from pid %d\n",
			 (int)procid_to_pid(&src)));
		break;
	case 1:		/* turn on counter profiling only */
		do_profile_flag = True;
		do_profile_times = False;
		DEBUG(1,("INFO: Profiling counts turned ON from pid %d\n",
			 (int)procid_to_pid(&src)));
		break;
	case 2:		/* turn on complete profiling */

#if defined(HAVE_CLOCK_GETTIME)
		if (!have_profiling_clock) {
			do_profile_flag = True;
			do_profile_times = False;
			DEBUG(1,("INFO: Profiling counts turned ON from "
				"pid %d\n", (int)procid_to_pid(&src)));
			DEBUGADD(1,("INFO: Profiling times disabled "
				"due to lack of a suitable clock\n"));
			break;
		}
#endif

		do_profile_flag = True;
		do_profile_times = True;
		DEBUG(1,("INFO: Full profiling turned ON from pid %d\n",
			 (int)procid_to_pid(&src)));
		break;
	case 3:		/* reset profile values */
		memset((char *)profile_p, 0, sizeof(*profile_p));
		DEBUG(1,("INFO: Profiling values cleared from pid %d\n",
			 (int)procid_to_pid(&src)));
		break;
	}
#else /* WITH_PROFILE */
	DEBUG(1,("INFO: Profiling support unavailable in this build.\n"));
#endif /* WITH_PROFILE */
}

/****************************************************************************
receive a request profile level message
****************************************************************************/
void reqprofile_message(int msg_type, struct process_id src,
			void *buf, size_t len)
{
        int level;

#ifdef WITH_PROFILE
	level = 1 + (do_profile_flag?2:0) + (do_profile_times?4:0);
#else
	level = 0;
#endif
	DEBUG(1,("INFO: Received REQ_PROFILELEVEL message from PID %u\n",
		 (unsigned int)procid_to_pid(&src)));
	message_send_pid(src, MSG_PROFILELEVEL, &level, sizeof(int), True);
}

/*******************************************************************
  open the profiling shared memory area
  ******************************************************************/
#ifdef WITH_PROFILE

#ifdef HAVE_CLOCK_GETTIME

/* Find a clock. Just because the definition for a particular clock ID is
 * present doesn't mean the system actually supports it.
 */
static void init_clock_gettime(void)
{
	struct timespec ts;

	have_profiling_clock = False;

#ifdef HAVE_CLOCK_PROCESS_CPUTIME_ID
	/* CLOCK_PROCESS_CPUTIME_ID is sufficiently fast that the
	 * always profiling times is plausible. Unfortunately on Linux
	 * it is only accurate if we can guarantee we will not be scheduled
	 * scheduled onto a different CPU between samples. Until there is
	 * some way to set processor affinity, we can only use this on
	 * uniprocessors.
	 */
	if (!this_is_smp()) {
	    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0) {
		    DEBUG(10, ("Using CLOCK_PROCESS_CPUTIME_ID "
				"for profile_clock\n"));
		    __profile_clock = CLOCK_PROCESS_CPUTIME_ID;
		    have_profiling_clock = True;
	    }
	}
#endif

#ifdef HAVE_CLOCK_MONOTONIC
	if (!have_profiling_clock &&
	    clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		DEBUG(10, ("Using CLOCK_MONOTONIC for profile_clock\n"));
		__profile_clock = CLOCK_MONOTONIC;
		have_profiling_clock = True;
	}
#endif

#ifdef HAVE_CLOCK_REALTIME
	/* POSIX says that CLOCK_REALTIME should be defined everywhere
	 * where we have clock_gettime...
	 */
	if (!have_profiling_clock &&
	    clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		__profile_clock = CLOCK_REALTIME;
		have_profiling_clock = True;

		SMB_WARN(__profile_clock != CLOCK_REALTIME,
			("forced to use a slow profiling clock"));
	}

#endif

	SMB_WARN(have_profiling_clock == True,
		("could not find a working clock for profiling"));
	return;
}
#endif

BOOL profile_setup(BOOL rdonly)
{
	struct shmid_ds shm_ds;

	read_only = rdonly;

#ifdef HAVE_CLOCK_GETTIME
	init_clock_gettime();
#endif

 again:
	/* try to use an existing key */
	shm_id = shmget(PROF_SHMEM_KEY, 0, 0);
	
	/* if that failed then create one. There is a race condition here
	   if we are running from inetd. Bad luck. */
	if (shm_id == -1) {
		if (read_only) return False;
		shm_id = shmget(PROF_SHMEM_KEY, sizeof(*profile_h), 
				IPC_CREAT | IPC_EXCL | IPC_PERMS);
	}
	
	if (shm_id == -1) {
		DEBUG(0,("Can't create or use IPC area. Error was %s\n", 
			 strerror(errno)));
		return False;
	}   
	
	
	profile_h = (struct profile_header *)shmat(shm_id, 0, 
						   read_only?SHM_RDONLY:0);
	if ((long)profile_p == -1) {
		DEBUG(0,("Can't attach to IPC area. Error was %s\n", 
			 strerror(errno)));
		return False;
	}

	/* find out who created this memory area */
	if (shmctl(shm_id, IPC_STAT, &shm_ds) != 0) {
		DEBUG(0,("ERROR shmctl : can't IPC_STAT. Error was %s\n", 
			 strerror(errno)));
		return False;
	}

	if (shm_ds.shm_perm.cuid != sec_initial_uid() ||
	    shm_ds.shm_perm.cgid != sec_initial_gid()) {
		DEBUG(0,("ERROR: we did not create the shmem "
			 "(owned by another user, uid %u, gid %u)\n",
			 shm_ds.shm_perm.cuid,
			 shm_ds.shm_perm.cgid));
		return False;
	}

	if (shm_ds.shm_segsz != sizeof(*profile_h)) {
		DEBUG(0,("WARNING: profile size is %d (expected %d). Deleting\n",
			 (int)shm_ds.shm_segsz, sizeof(*profile_h)));
		if (shmctl(shm_id, IPC_RMID, &shm_ds) == 0) {
			goto again;
		} else {
			return False;
		}
	}

	if (!read_only && (shm_ds.shm_nattch == 1)) {
		memset((char *)profile_h, 0, sizeof(*profile_h));
		profile_h->prof_shm_magic = PROF_SHM_MAGIC;
		profile_h->prof_shm_version = PROF_SHM_VERSION;
		DEBUG(3,("Initialised profile area\n"));
	}

	profile_p = &profile_h->stats;
	message_register(MSG_PROFILE, profile_message);
	message_register(MSG_REQ_PROFILELEVEL, reqprofile_message);
	return True;
}
#endif /* WITH_PROFILE */
