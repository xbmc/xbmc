/*
   Unix SMB/CIFS implementation.
   Copyright (C) Jeremy Allison 1998.
   rewritten for version 2.0.6 by Tridge

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

#ifndef AUTOCONF_TEST
#include "includes.h"
#else
/* we are running this code in autoconf test mode to see which type of setuid
   function works */
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#ifdef HAVE_SYS_PRIV_H
#include <sys/priv.h>
#endif
#ifdef HAVE_SYS_ID_H
#include <sys/id.h>
#endif

#define DEBUG(x, y) printf y
#define smb_panic(x) exit(1)
#define BOOL int
#endif

/* are we running as non-root? This is used by the regresison test code,
   and potentially also for sites that want non-root smbd */
static uid_t initial_uid;
static gid_t initial_gid;

/****************************************************************************
remember what uid we got started as - this allows us to run correctly
as non-root while catching trapdoor systems
****************************************************************************/
void sec_init(void)
{
	static int initialized;

	if (!initialized) {
		initial_uid = geteuid();
		initial_gid = getegid();
		initialized = 1;
	}
}

/****************************************************************************
some code (eg. winbindd) needs to know what uid we started as
****************************************************************************/
uid_t sec_initial_uid(void)
{
	return initial_uid;
}

/****************************************************************************
some code (eg. winbindd, profiling shm) needs to know what gid we started as
****************************************************************************/
gid_t sec_initial_gid(void)
{
	return initial_gid;
}

/****************************************************************************
are we running in non-root mode?
****************************************************************************/
BOOL non_root_mode(void)
{
	return (initial_uid != (uid_t)0);
}

/****************************************************************************
abort if we haven't set the uid correctly
****************************************************************************/
static void assert_uid(uid_t ruid, uid_t euid)
{
	if ((euid != (uid_t)-1 && geteuid() != euid) ||
	    (ruid != (uid_t)-1 && getuid() != ruid)) {
		if (!non_root_mode()) {
			DEBUG(0,("Failed to set uid privileges to (%d,%d) now set to (%d,%d)\n",
				 (int)ruid, (int)euid,
				 (int)getuid(), (int)geteuid()));
			smb_panic("failed to set uid\n");
			exit(1);
		}
	}
}

/****************************************************************************
abort if we haven't set the gid correctly
****************************************************************************/
static void assert_gid(gid_t rgid, gid_t egid)
{
	if ((egid != (gid_t)-1 && getegid() != egid) ||
	    (rgid != (gid_t)-1 && getgid() != rgid)) {
		if (!non_root_mode()) {
			DEBUG(0,("Failed to set gid privileges to (%d,%d) now set to (%d,%d) uid=(%d,%d)\n",
				 (int)rgid, (int)egid,
				 (int)getgid(), (int)getegid(),
				 (int)getuid(), (int)geteuid()));
			smb_panic("failed to set gid\n");
			exit(1);
		}
	}
}

/****************************************************************************
 Gain root privilege before doing something. 
 We want to end up with ruid==euid==0
****************************************************************************/
void gain_root_privilege(void)
{	
#if USE_SETRESUID
	setresuid(0,0,0);
#endif
    
#if USE_SETEUID
	seteuid(0);
#endif

#if USE_SETREUID
	setreuid(0, 0);
#endif

#if USE_SETUIDX
	setuidx(ID_EFFECTIVE, 0);
	setuidx(ID_REAL, 0);
#endif

	/* this is needed on some systems */
	setuid(0);

	assert_uid(0, 0);
}


/****************************************************************************
 Ensure our real and effective groups are zero.
 we want to end up with rgid==egid==0
****************************************************************************/
void gain_root_group_privilege(void)
{
#if USE_SETRESUID
	setresgid(0,0,0);
#endif

#if USE_SETREUID
	setregid(0,0);
#endif

#if USE_SETEUID
	setegid(0);
#endif

#if USE_SETUIDX
	setgidx(ID_EFFECTIVE, 0);
	setgidx(ID_REAL, 0);
#endif

	setgid(0);

	assert_gid(0, 0);
}


/****************************************************************************
 Set effective uid, and possibly the real uid too.
 We want to end up with either:
  
   ruid==uid and euid==uid

 or

   ruid==0 and euid==uid

 depending on what the local OS will allow us to regain root from.
****************************************************************************/
void set_effective_uid(uid_t uid)
{
#if USE_SETRESUID
        /* Set the effective as well as the real uid. */
	setresuid(uid,uid,-1);
#endif

#if USE_SETREUID
	setreuid(-1,uid);
#endif

#if USE_SETEUID
	seteuid(uid);
#endif

#if USE_SETUIDX
	setuidx(ID_EFFECTIVE, uid);
#endif

	assert_uid(-1, uid);
}

/****************************************************************************
 Set *only* the effective gid.
 we want to end up with rgid==0 and egid==gid
****************************************************************************/
void set_effective_gid(gid_t gid)
{
#if USE_SETRESUID
	setresgid(-1,gid,-1);
#endif

#if USE_SETREUID
	setregid(-1,gid);
#endif

#if USE_SETEUID
	setegid(gid);
#endif

#if USE_SETUIDX
	setgidx(ID_EFFECTIVE, gid);
#endif

	assert_gid(-1, gid);
}

static uid_t saved_euid, saved_ruid;
static gid_t saved_egid, saved_rgid;

/****************************************************************************
 save the real and effective uid for later restoration. Used by the quotas
 code
****************************************************************************/
void save_re_uid(void)
{
	saved_ruid = getuid();
	saved_euid = geteuid();
}


/****************************************************************************
 and restore them!
****************************************************************************/

static void restore_re_uid_fromroot(void)
{
#if USE_SETRESUID
	setresuid(saved_ruid, saved_euid, -1);
#elif USE_SETREUID
	setreuid(saved_ruid, -1);
	setreuid(-1,saved_euid);
#elif USE_SETUIDX
	setuidx(ID_REAL, saved_ruid);
	setuidx(ID_EFFECTIVE, saved_euid);
#else
	set_effective_uid(saved_euid);
	if (getuid() != saved_ruid)
		setuid(saved_ruid);
	set_effective_uid(saved_euid);
#endif

	assert_uid(saved_ruid, saved_euid);
}

void restore_re_uid(void)
{
	set_effective_uid(0);
	restore_re_uid_fromroot();
}

/****************************************************************************
 Lightweight become root - no group change.
****************************************************************************/

void become_root_uid_only(void)
{
	save_re_uid();
	set_effective_uid(0);
}

/****************************************************************************
 Lightweight unbecome root - no group change. Expects we are root already,
 saves errno across call boundary.
****************************************************************************/

void unbecome_root_uid_only(void)
{
	int saved_errno = errno;
	restore_re_uid_fromroot();
	errno = saved_errno;
}

/****************************************************************************
 save the real and effective gid for later restoration. Used by the 
 getgroups code
****************************************************************************/
void save_re_gid(void)
{
	saved_rgid = getgid();
	saved_egid = getegid();
}

/****************************************************************************
 and restore them!
****************************************************************************/
void restore_re_gid(void)
{
#if USE_SETRESUID
	setresgid(saved_rgid, saved_egid, -1);
#elif USE_SETREUID
	setregid(saved_rgid, -1);
	setregid(-1,saved_egid);
#elif USE_SETUIDX
	setgidx(ID_REAL, saved_rgid);
	setgidx(ID_EFFECTIVE, saved_egid);
#else
	set_effective_gid(saved_egid);
	if (getgid() != saved_rgid)
		setgid(saved_rgid);
	set_effective_gid(saved_egid);
#endif

	assert_gid(saved_rgid, saved_egid);
}


/****************************************************************************
 set the real AND effective uid to the current effective uid in a way that
 allows root to be regained.
 This is only possible on some platforms.
****************************************************************************/
int set_re_uid(void)
{
	uid_t uid = geteuid();

#if USE_SETRESUID
	setresuid(geteuid(), -1, -1);
#endif

#if USE_SETREUID
	setreuid(0, 0);
	setreuid(uid, -1);
	setreuid(-1, uid);
#endif

#if USE_SETEUID
	/* can't be done */
	return -1;
#endif

#if USE_SETUIDX
	/* can't be done */
	return -1;
#endif

	assert_uid(uid, uid);
	return 0;
}


/****************************************************************************
 Become the specified uid and gid - permanently !
 there should be no way back if possible
****************************************************************************/
void become_user_permanently(uid_t uid, gid_t gid)
{
	/*
	 * First - gain root privilege. We do this to ensure
	 * we can lose it again.
	 */

	gain_root_privilege();
	gain_root_group_privilege();

#if USE_SETRESUID
	setresgid(gid,gid,gid);
	setgid(gid);
	setresuid(uid,uid,uid);
	setuid(uid);
#endif

#if USE_SETREUID
	setregid(gid,gid);
	setgid(gid);
	setreuid(uid,uid);
	setuid(uid);
#endif

#if USE_SETEUID
	setegid(gid);
	setgid(gid);
	setuid(uid);
	seteuid(uid);
	setuid(uid);
#endif

#if USE_SETUIDX
	setgidx(ID_REAL, gid);
	setgidx(ID_EFFECTIVE, gid);
	setgid(gid);
	setuidx(ID_REAL, uid);
	setuidx(ID_EFFECTIVE, uid);
	setuid(uid);
#endif
	
	assert_uid(uid, uid);
	assert_gid(gid, gid);
}

#ifdef AUTOCONF_TEST

/****************************************************************************
this function just checks that we don't get ENOSYS back
****************************************************************************/
static int have_syscall(void)
{
	errno = 0;

#if USE_SETRESUID
	setresuid(-1,-1,-1);
#endif

#if USE_SETREUID
	setreuid(-1,-1);
#endif

#if USE_SETEUID
	seteuid(-1);
#endif

#if USE_SETUIDX
	setuidx(ID_EFFECTIVE, -1);
#endif

	if (errno == ENOSYS) return -1;
	
	return 0;
}

main()
{
        if (getuid() != 0) {
#if (defined(AIX) && defined(USE_SETREUID))
		/* setreuid is badly broken on AIX 4.1, we avoid it completely */
                fprintf(stderr,"avoiding possibly broken setreuid\n");
		exit(1);
#endif

		/* if not running as root then at least check to see if we get ENOSYS - this 
		   handles Linux 2.0.x with glibc 2.1 */
                fprintf(stderr,"not running as root: checking for ENOSYS\n");
		exit(have_syscall());
	}

	gain_root_privilege();
	gain_root_group_privilege();
	set_effective_gid(1);
	set_effective_uid(1);
	save_re_uid();
	restore_re_uid();
	gain_root_privilege();
	gain_root_group_privilege();
	become_user_permanently(1, 1);
	setuid(0);
	if (getuid() == 0) {
		fprintf(stderr,"uid not set permanently\n");
		exit(1);
	}

	printf("OK\n");

	exit(0);
}
#endif

/****************************************************************************
Check if we are setuid root.  Used in libsmb and smbpasswd paranoia checks.
****************************************************************************/
BOOL is_setuid_root(void) 
{
	return (geteuid() == (uid_t)0) && (getuid() != (uid_t)0);
}
