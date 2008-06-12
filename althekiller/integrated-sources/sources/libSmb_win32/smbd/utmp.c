/* 
   Unix SMB/CIFS implementation.
   utmp routines
   Copyright (C) T.D.Lee@durham.ac.uk 1999
   Heavily modified by Andrew Bartlett and Tridge, April 2001
   
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

/****************************************************************************
Reflect connection status in utmp/wtmp files.
	T.D.Lee@durham.ac.uk  September 1999

	With grateful thanks since then to many who have helped port it to
	different operating systems.  The variety of OS quirks thereby
	uncovered is amazing...

Hints for porting:
	o  Always attempt to use programmatic interface (pututline() etc.)
	   Indeed, at present only programmatic use is supported.
	o  The only currently supported programmatic interface to "wtmp{,x}"
	   is through "updwtmp*()" routines.
	o  The "x" (utmpx/wtmpx; HAVE_UTMPX_H) seems preferable.
	o  The HAVE_* items should identify supported features.
	o  If at all possible, avoid "if defined(MY-OS)" constructions.

OS observations and status:
	Almost every OS seems to have its own quirks.

	Solaris 2.x:
		Tested on 2.6 and 2.7; should be OK on other flavours.
	AIX:
		Apparently has utmpx.h but doesn't implement.
	OSF:
		Has utmpx.h, but (e.g.) no "getutmpx()".  (Is this like AIX ?)
	Redhat 6:
		utmpx.h seems not to set default filenames.  non-x better.
	IRIX 6.5:
		Not tested.  Appears to have "x".
	HP-UX 9.x:
		Not tested.  Appears to lack "x".
	HP-UX 10.x:
		Not tested.
		"updwtmp*()" routines seem absent, so no current wtmp* support.
		Has "ut_addr": probably trivial to implement (although remember
		that IPv6 is coming...).

	FreeBSD:
		No "putut*()" type of interface.
		No "ut_type" and associated defines. 
		Write files directly.  Alternatively use its login(3)/logout(3).
	SunOS 4:
		Not tested.  Resembles FreeBSD, but no login()/logout().

lastlog:
	Should "lastlog" files, if any, be updated?
	BSD systems (SunOS 4, FreeBSD):
		o  Prominent mention on man pages.
	System-V (e.g. Solaris 2):
		o  No mention on man pages, even under "man -k".
		o  Has a "/var/adm/lastlog" file, but pututxline() etc. seem
		   not to touch it.
		o  Despite downplaying (above), nevertheless has <lastlog.h>.
	So perhaps UN*X "lastlog" facility is intended for tty/terminal only?

Notes:
	Each connection requires a small number (starting at 0, working up)
	to represent the line.  This must be unique within and across all
	smbd processes.  It is the 'id_num' from Samba's session.c code.

	The 4 byte 'ut_id' component is vital to distinguish connections,
	of which there could be several hundred or even thousand.
	Entries seem to be printable characters, with optional NULL pads.

	We need to be distinct from other entries in utmp/wtmp.

	Observed things: therefore avoid them.  Add to this list please.
	From Solaris 2.x (because that's what I have):
		'sN'	: run-levels; N: [0-9]
		'co'	: console
		'CC'	: arbitrary things;  C: [a-z]
		'rXNN'	: rlogin;  N: [0-9]; X: [0-9a-z]
		'tXNN'	: rlogin;  N: [0-9]; X: [0-9a-z]
		'/NNN'	: Solaris CDE
		'ftpZ'	: ftp (Z is the number 255, aka 0377, aka 0xff)
	Mostly a record uses the same 'ut_id' in both "utmp" and "wtmp",
	but differences have been seen.

	Arbitrarily I have chosen to use a distinctive 'SM' for the
	first two bytes.

	The remaining two bytes encode the session 'id_num' (see above).
	Our caller (session.c) should note our 16-bit limitation.

****************************************************************************/

#ifndef WITH_UTMP
/*
 * Not WITH_UTMP?  Simply supply dummy routines.
 */

void sys_utmp_claim(const char *username, const char *hostname, 
		    struct in_addr *ipaddr,
		    const char *id_str, int id_num)
{}

void sys_utmp_yield(const char *username, const char *hostname, 
		    struct in_addr *ipaddr,
		    const char *id_str, int id_num)
{}

#else /* WITH_UTMP */

#include <utmp.h>

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif

/* BSD systems: some may need lastlog.h (SunOS 4), some may not (FreeBSD) */
/* Some System-V systems (e.g. Solaris 2) declare this too. */
#ifdef HAVE_LASTLOG_H
#include <lastlog.h>
#endif

/****************************************************************************
 Default paths to various {u,w}tmp{,x} files.
****************************************************************************/

#ifdef	HAVE_UTMPX_H

static const char *ux_pathname =
# if defined (UTMPX_FILE)
	UTMPX_FILE ;
# elif defined (_UTMPX_FILE)
	_UTMPX_FILE ;
# elif defined (_PATH_UTMPX)
	_PATH_UTMPX ;
# else
	"" ;
# endif

static const char *wx_pathname =
# if defined (WTMPX_FILE)
	WTMPX_FILE ;
# elif defined (_WTMPX_FILE)
	_WTMPX_FILE ;
# elif defined (_PATH_WTMPX)
	_PATH_WTMPX ;
# else
	"" ;
# endif

#endif	/* HAVE_UTMPX_H */

static const char *ut_pathname =
# if defined (UTMP_FILE)
	UTMP_FILE ;
# elif defined (_UTMP_FILE)
	_UTMP_FILE ;
# elif defined (_PATH_UTMP)
	_PATH_UTMP ;
# else
	"" ;
# endif

static const char *wt_pathname =
# if defined (WTMP_FILE)
	WTMP_FILE ;
# elif defined (_WTMP_FILE)
	_WTMP_FILE ;
# elif defined (_PATH_WTMP)
	_PATH_WTMP ;
# else
	"" ;
# endif

/* BSD-like systems might want "lastlog" support. */
/* *** Not yet implemented */
#ifndef HAVE_PUTUTLINE		/* see "pututline_my()" */
static const char *ll_pathname =
# if defined (_PATH_LASTLOG)	/* what other names (if any?) */
	_PATH_LASTLOG ;
# else
	"" ;
# endif	/* _PATH_LASTLOG */
#endif	/* HAVE_PUTUTLINE */

/*
 * Get name of {u,w}tmp{,x} file.
 *	return: fname contains filename
 *		Possibly empty if this code not yet ported to this system.
 *
 * utmp{,x}:  try "utmp dir", then default (a define)
 * wtmp{,x}:  try "wtmp dir", then "utmp dir", then default (a define)
 */
static void uw_pathname(pstring fname, const char *uw_name, const char *uw_default)
{
	pstring dirname;

	pstrcpy(dirname, "");

	/* For w-files, first look for explicit "wtmp dir" */
	if (uw_name[0] == 'w') {
		pstrcpy(dirname,lp_wtmpdir());
		trim_char(dirname,'\0','/');
	}

	/* For u-files and non-explicit w-dir, look for "utmp dir" */
	if (dirname == 0 || strlen(dirname) == 0) {
		pstrcpy(dirname,lp_utmpdir());
		trim_char(dirname,'\0','/');
	}

	/* If explicit directory above, use it */
	if (dirname != 0 && strlen(dirname) != 0) {
		pstrcpy(fname, dirname);
		pstrcat(fname, "/");
		pstrcat(fname, uw_name);
		return;
	}

	/* No explicit directory: attempt to use default paths */
	if (strlen(uw_default) == 0) {
		/* No explicit setting, no known default.
		 * Has it yet been ported to this OS?
		 */
		DEBUG(2,("uw_pathname: unable to determine pathname\n"));
	}
	pstrcpy(fname, uw_default);
}

#ifndef HAVE_PUTUTLINE

/****************************************************************************
 Update utmp file directly.  No subroutine interface: probably a BSD system.
****************************************************************************/

static void pututline_my(pstring uname, struct utmp *u, BOOL claim)
{
	DEBUG(1,("pututline_my: not yet implemented\n"));
	/* BSD implementor: may want to consider (or not) adjusting "lastlog" */
}
#endif /* HAVE_PUTUTLINE */

#ifndef HAVE_UPDWTMP

/****************************************************************************
 Update wtmp file directly.  No subroutine interface: probably a BSD system.
 Credit: Michail Vidiassov <master@iaas.msu.ru>
****************************************************************************/

static void updwtmp_my(pstring wname, struct utmp *u, BOOL claim)
{
	int fd;
	struct stat buf;

	if (! claim) {
		/*
	 	 * BSD-like systems:
		 *	may use empty ut_name to distinguish a logout record.
		 *
		 * May need "if defined(SUNOS4)" etc. around some of these,
		 * but try to avoid if possible.
		 *
		 * SunOS 4:
		 *	man page indicates ut_name and ut_host both NULL
		 * FreeBSD 4.0:
		 *	man page appears not to specify (hints non-NULL)
		 *	A correspondent suggest at least ut_name should be NULL
		 */
#if defined(HAVE_UT_UT_NAME)
		memset((char *)&u->ut_name, '\0', sizeof(u->ut_name));
#endif
#if defined(HAVE_UT_UT_HOST)
		memset((char *)&u->ut_host, '\0', sizeof(u->ut_host));
#endif
	}
	/* Stolen from logwtmp function in libutil.
	 * May be more locking/blocking is needed?
	 */
	if ((fd = open(wname, O_WRONLY|O_APPEND, 0)) < 0)
		return;
	if (fstat(fd, &buf) == 0) {
		if (write(fd, (char *)u, sizeof(struct utmp)) != sizeof(struct utmp))
		(void) ftruncate(fd, buf.st_size);
	}
	(void) close(fd);
}
#endif /* HAVE_UPDWTMP */

/****************************************************************************
 Update via utmp/wtmp (not utmpx/wtmpx).
****************************************************************************/

static void utmp_nox_update(struct utmp *u, BOOL claim)
{
	pstring uname, wname;
#if defined(PUTUTLINE_RETURNS_UTMP)
	struct utmp *urc;
#endif /* PUTUTLINE_RETURNS_UTMP */

	uw_pathname(uname, "utmp", ut_pathname);
	DEBUG(2,("utmp_nox_update: uname:%s\n", uname));

#ifdef HAVE_PUTUTLINE
	if (strlen(uname) != 0) {
		utmpname(uname);
	}

# if defined(PUTUTLINE_RETURNS_UTMP)
	setutent();
	urc = pututline(u);
	endutent();
	if (urc == NULL) {
		DEBUG(2,("utmp_nox_update: pututline() failed\n"));
		return;
	}
# else	/* PUTUTLINE_RETURNS_UTMP */
	setutent();
	pututline(u);
	endutent();
# endif	/* PUTUTLINE_RETURNS_UTMP */

#else	/* HAVE_PUTUTLINE */
	if (strlen(uname) != 0) {
		pututline_my(uname, u, claim);
	}
#endif /* HAVE_PUTUTLINE */

	uw_pathname(wname, "wtmp", wt_pathname);
	DEBUG(2,("utmp_nox_update: wname:%s\n", wname));
	if (strlen(wname) != 0) {
#ifdef HAVE_UPDWTMP
		updwtmp(wname, u);
		/*
		 * updwtmp() and the newer updwtmpx() may be unsymmetrical.
		 * At least one OS, Solaris 2.x declares the former in the
		 * "utmpx" (latter) file and context.
		 * In the Solaris case this is irrelevant: it has both and
		 * we always prefer the "x" case, so doesn't come here.
		 * But are there other systems, with no "x", which lack
		 * updwtmp() perhaps?
		 */
#else
		updwtmp_my(wname, u, claim);
#endif /* HAVE_UPDWTMP */
	}
}

/****************************************************************************
 Copy a string in the utmp structure.
****************************************************************************/

static void utmp_strcpy(char *dest, const char *src, size_t n)
{
	size_t len = 0;

	memset(dest, '\0', n);
	if (src)
		len = strlen(src);
	if (len >= n) {
		memcpy(dest, src, n);
	} else {
		if (len)
			memcpy(dest, src, len);
	}
}

/****************************************************************************
 Update via utmpx/wtmpx (preferred) or via utmp/wtmp.
****************************************************************************/

static void sys_utmp_update(struct utmp *u, const char *hostname, BOOL claim)
{
#if !defined(HAVE_UTMPX_H)
	/* No utmpx stuff.  Drop to non-x stuff */
	utmp_nox_update(u, claim);
#elif !defined(HAVE_PUTUTXLINE)
	/* Odd.  Have utmpx.h but no "pututxline()".  Drop to non-x stuff */
	DEBUG(1,("utmp_update: have utmpx.h but no pututxline() function\n"));
	utmp_nox_update(u, claim);
#elif !defined(HAVE_GETUTMPX)
	/* Odd.  Have utmpx.h but no "getutmpx()".  Drop to non-x stuff */
	DEBUG(1,("utmp_update: have utmpx.h but no getutmpx() function\n"));
	utmp_nox_update(u, claim);
#else
	pstring uname, wname;
	struct utmpx ux, *uxrc;

	getutmpx(u, &ux);

#if defined(HAVE_UX_UT_SYSLEN)
	if (hostname)
		ux.ut_syslen = strlen(hostname) + 1;	/* include end NULL */
	else
		ux.ut_syslen = 0;
#endif
#if defined(HAVE_UT_UT_HOST)
	utmp_strcpy(ux.ut_host, hostname, sizeof(ux.ut_host));
#endif

	uw_pathname(uname, "utmpx", ux_pathname);
	uw_pathname(wname, "wtmpx", wx_pathname);
	DEBUG(2,("utmp_update: uname:%s wname:%s\n", uname, wname));
	/*
	 * Check for either uname or wname being empty.
	 * Some systems, such as Redhat 6, have a "utmpx.h" which doesn't
	 * define default filenames.
	 * Also, our local installation has not provided an override.
	 * Drop to non-x method.  (E.g. RH6 has good defaults in "utmp.h".)
	 */
	if ((strlen(uname) == 0) || (strlen(wname) == 0)) {
		utmp_nox_update(u, claim);
	} else {
		utmpxname(uname);
		setutxent();
		uxrc = pututxline(&ux);
		endutxent();
		if (uxrc == NULL) {
			DEBUG(2,("utmp_update: pututxline() failed\n"));
			return;
		}
		updwtmpx(wname, &ux);
	}
#endif /* HAVE_UTMPX_H */
}

#if defined(HAVE_UT_UT_ID)
/****************************************************************************
 Encode the unique connection number into "ut_id".
****************************************************************************/

static int ut_id_encode(int i, char *fourbyte)
{
	int nbase;
	const char *ut_id_encstr = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	fourbyte[0] = 'S';
	fourbyte[1] = 'M';

/*
 * Encode remaining 2 bytes from 'i'.
 * 'ut_id_encstr' is the character set on which modulo arithmetic is done.
 * Example: digits would produce the base-10 numbers from '001'.
 */
	nbase = strlen(ut_id_encstr);

	fourbyte[3] = ut_id_encstr[i % nbase];
	i /= nbase;
	fourbyte[2] = ut_id_encstr[i % nbase];
	i /= nbase;

	return(i);	/* 0: good; else overflow */
}
#endif /* defined(HAVE_UT_UT_ID) */


/*
  fill a system utmp structure given all the info we can gather 
*/
static BOOL sys_utmp_fill(struct utmp *u,
			  const char *username, const char *hostname,
			  struct in_addr *ipaddr,
			  const char *id_str, int id_num)
{			  
	struct timeval timeval;

	/*
	 * ut_name, ut_user:
	 *	Several (all?) systems seems to define one as the other.
	 *	It is easier and clearer simply to let the following take its course,
	 *	rather than to try to detect and optimise.
	 */
#if defined(HAVE_UT_UT_USER)
	utmp_strcpy(u->ut_user, username, sizeof(u->ut_user));
#elif defined(HAVE_UT_UT_NAME)
	utmp_strcpy(u->ut_name, username, sizeof(u->ut_name));
#endif

	/*
	 * ut_line:
	 *	If size limit proves troublesome, then perhaps use "ut_id_encode()".
	 */
	if (strlen(id_str) > sizeof(u->ut_line)) {
		DEBUG(1,("id_str [%s] is too long for %lu char utmp field\n",
			 id_str, (unsigned long)sizeof(u->ut_line)));
		return False;
	}
	utmp_strcpy(u->ut_line, id_str, sizeof(u->ut_line));

#if defined(HAVE_UT_UT_PID)
	u->ut_pid = sys_getpid();
#endif

/*
 * ut_time, ut_tv: 
 *	Some have one, some the other.  Many have both, but defined (aliased).
 *	It is easier and clearer simply to let the following take its course.
 *	But note that we do the more precise ut_tv as the final assignment.
 */
#if defined(HAVE_UT_UT_TIME)
	GetTimeOfDay(&timeval);
	u->ut_time = timeval.tv_sec;
#elif defined(HAVE_UT_UT_TV)
	GetTimeOfDay(&timeval);
	u->ut_tv = timeval;
#else
#error "with-utmp must have UT_TIME or UT_TV"
#endif

#if defined(HAVE_UT_UT_HOST)
	utmp_strcpy(u->ut_host, hostname, sizeof(u->ut_host));
#endif
#if defined(HAVE_UT_UT_ADDR)
	if (ipaddr)
		u->ut_addr = ipaddr->s_addr;
	/*
	 * "(unsigned long) ut_addr" apparently exists on at least HP-UX 10.20.
	 * Volunteer to implement, please ...
	 */
#endif

#if defined(HAVE_UT_UT_ID)
	if (ut_id_encode(id_num, u->ut_id) != 0) {
		DEBUG(1,("utmp_fill: cannot encode id %d\n", id_num));
		return False;
	}
#endif

	return True;
}

/****************************************************************************
 Close a connection.
****************************************************************************/

void sys_utmp_yield(const char *username, const char *hostname, 
		    struct in_addr *ipaddr,
		    const char *id_str, int id_num)
{
	struct utmp u;

	ZERO_STRUCT(u);

#if defined(HAVE_UT_UT_EXIT)
	u.ut_exit.e_termination = 0;
	u.ut_exit.e_exit = 0;
#endif

#if defined(HAVE_UT_UT_TYPE)
	u.ut_type = DEAD_PROCESS;
#endif

	if (!sys_utmp_fill(&u, username, hostname, ipaddr, id_str, id_num)) return;

	sys_utmp_update(&u, NULL, False);
}

/****************************************************************************
 Claim a entry in whatever utmp system the OS uses.
****************************************************************************/

void sys_utmp_claim(const char *username, const char *hostname, 
		    struct in_addr *ipaddr,
		    const char *id_str, int id_num)
{
	struct utmp u;

	ZERO_STRUCT(u);

#if defined(HAVE_UT_UT_TYPE)
	u.ut_type = USER_PROCESS;
#endif

	if (!sys_utmp_fill(&u, username, hostname, ipaddr, id_str, id_num)) return;

	sys_utmp_update(&u, hostname, True);
}

#endif /* WITH_UTMP */
