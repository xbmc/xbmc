/* 
   Unix SMB/CIFS implementation.
   signal handling functions

   Copyright (C) Andrew Tridgell 1998
   
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
 Catch child exits and reap the child zombie status.
****************************************************************************/

static void sig_cld(int signum)
{
	while (sys_waitpid((pid_t)-1,(int *)NULL, WNOHANG) > 0)
		;

	/*
	 * Turns out it's *really* important not to
	 * restore the signal handler here if we have real POSIX
	 * signal handling. If we do, then we get the signal re-delivered
	 * immediately - hey presto - instant loop ! JRA.
	 */

#if !defined(HAVE_SIGACTION)
	CatchSignal(SIGCLD, sig_cld);
#endif
}

/****************************************************************************
catch child exits - leave status;
****************************************************************************/

static void sig_cld_leave_status(int signum)
{
	/*
	 * Turns out it's *really* important not to
	 * restore the signal handler here if we have real POSIX
	 * signal handling. If we do, then we get the signal re-delivered
	 * immediately - hey presto - instant loop ! JRA.
	 */

#if !defined(HAVE_SIGACTION)
	CatchSignal(SIGCLD, sig_cld_leave_status);
#endif
}

/*******************************************************************
 Block sigs.
********************************************************************/

void BlockSignals(BOOL block,int signum)
{
#ifdef HAVE_SIGPROCMASK
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,signum);
	sigprocmask(block?SIG_BLOCK:SIG_UNBLOCK,&set,NULL);
#elif defined(HAVE_SIGBLOCK)
	if (block) {
		sigblock(sigmask(signum));
	} else {
		sigsetmask(siggetmask() & ~sigmask(signum));
	}
#else
	/* yikes! This platform can't block signals? */
	static int done;
	if (!done) {
		DEBUG(0,("WARNING: No signal blocking available\n"));
		done=1;
	}
#endif
}

/*******************************************************************
 Catch a signal. This should implement the following semantics:

 1) The handler remains installed after being called.
 2) The signal should be blocked during handler execution.
********************************************************************/

void (*CatchSignal(int signum,void (*handler)(int )))(int)
{
#ifdef HAVE_SIGACTION
	struct sigaction act;
	struct sigaction oldact;

	ZERO_STRUCT(act);

	act.sa_handler = handler;
#ifdef SA_RESTART
	/*
	 * We *want* SIGALRM to interrupt a system call.
	 */
	if(signum != SIGALRM)
		act.sa_flags = SA_RESTART;
#endif
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask,signum);
	sigaction(signum,&act,&oldact);
	return oldact.sa_handler;
#else /* !HAVE_SIGACTION */
	/* FIXME: need to handle sigvec and systems with broken signal() */
	return signal(signum, handler);
#endif
}

/*******************************************************************
 Ignore SIGCLD via whatever means is necessary for this OS.
********************************************************************/

void CatchChild(void)
{
	CatchSignal(SIGCLD, sig_cld);
}

/*******************************************************************
 Catch SIGCLD but leave the child around so it's status can be reaped.
********************************************************************/

void CatchChildLeaveStatus(void)
{
	CatchSignal(SIGCLD, sig_cld_leave_status);
}
