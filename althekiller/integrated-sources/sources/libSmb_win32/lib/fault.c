/* 
   Unix SMB/CIFS implementation.
   Critical Fault handling
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

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

static void (*cont_fn)(void *);
static pstring corepath;

/*******************************************************************
report a fault
********************************************************************/
static void fault_report(int sig)
{
	static int counter;

	if (counter) _exit(1);

	counter++;

	DEBUGSEP(0);
	DEBUG(0,("INTERNAL ERROR: Signal %d in pid %d (%s)",sig,(int)sys_getpid(),SAMBA_VERSION_STRING));
	DEBUG(0,("\nPlease read the Trouble-Shooting section of the Samba3-HOWTO\n"));
	DEBUG(0,("\nFrom: http://www.samba.org/samba/docs/Samba3-HOWTO.pdf\n"));
	DEBUGSEP(0);
  
	smb_panic("internal error");

	if (cont_fn) {
		cont_fn(NULL);
#ifdef SIGSEGV
		CatchSignal(SIGSEGV,SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGBUS
		CatchSignal(SIGBUS,SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGABRT
		CatchSignal(SIGABRT,SIGNAL_CAST SIG_DFL);
#endif
		return; /* this should cause a core dump */
	}
	exit(1);
}

/****************************************************************************
catch serious errors
****************************************************************************/
static void sig_fault(int sig)
{
	fault_report(sig);
}

/*******************************************************************
setup our fault handlers
********************************************************************/
void fault_setup(void (*fn)(void *))
{
	cont_fn = fn;

#ifdef SIGSEGV
	CatchSignal(SIGSEGV,SIGNAL_CAST sig_fault);
#endif
#ifdef SIGBUS
	CatchSignal(SIGBUS,SIGNAL_CAST sig_fault);
#endif
#ifdef SIGABRT
	CatchSignal(SIGABRT,SIGNAL_CAST sig_fault);
#endif
}

/*******************************************************************
make all the preparations to safely dump a core file
********************************************************************/

void dump_core_setup(const char *progname)
{
	pstring logbase;
	char * end;

	if (lp_logfile() && *lp_logfile()) {
		snprintf(logbase, sizeof(logbase), "%s", lp_logfile());
		if ((end = strrchr_m(logbase, '/'))) {
			*end = '\0';
		}
	} else {
		/* We will end up here is the log file is given on the command
		 * line by the -l option but the "log file" option is not set
		 * in smb.conf.
		 */
		snprintf(logbase, sizeof(logbase), "%s", dyn_LOGFILEBASE);
	}

	SMB_ASSERT(progname != NULL);

	snprintf(corepath, sizeof(corepath), "%s/cores", logbase);
	mkdir(corepath,0700);

	snprintf(corepath, sizeof(corepath), "%s/cores/%s",
		logbase, progname);
	mkdir(corepath,0700);

	sys_chown(corepath,getuid(),getgid());
	chmod(corepath,0700);

#ifdef HAVE_GETRLIMIT
#ifdef RLIMIT_CORE
	{
		struct rlimit rlp;
		getrlimit(RLIMIT_CORE, &rlp);
		rlp.rlim_cur = MAX(16*1024*1024,rlp.rlim_cur);
		setrlimit(RLIMIT_CORE, &rlp);
		getrlimit(RLIMIT_CORE, &rlp);
		DEBUG(3,("Maximum core file size limits now %d(soft) %d(hard)\n",
			 (int)rlp.rlim_cur,(int)rlp.rlim_max));
	}
#endif
#endif

#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
	/* On Linux we lose the ability to dump core when we change our user
	 * ID. We know how to dump core safely, so let's make sure we have our
	 * dumpable flag set.
	 */
	prctl(PR_SET_DUMPABLE, 1);
#endif

	/* FIXME: if we have a core-plus-pid facility, configurably set
	 * this up here.
	 */
}

 void dump_core(void)
{
	/* Note that even if core dumping has been disabled, we still set up
	 * the core path. This is to handle the case where core dumping is
	 * turned on in smb.conf and the relevant daemon is not restarted.
	 */
	if (!lp_enable_core_files()) {
		DEBUG(0, ("Exiting on internal error (core file administratively disabled\n"));
		exit(1);
	}

	if (*corepath != '\0') {
		/* The chdir might fail if we dump core before we finish
		 * processing the config file.
		 */
		if (chdir(corepath) != 0) {
			DEBUG(0, ("unable to change to %s", corepath));
			DEBUGADD(0, ("refusing to dump core\n"));
			exit(1);
		}

		DEBUG(0,("dumping core in %s\n", corepath));
	}

	umask(~(0700));
	dbgflush();

	/* Ensure we don't have a signal handler for abort. */
#ifdef SIGABRT
	CatchSignal(SIGABRT,SIGNAL_CAST SIG_DFL);
#endif

	abort();
}

