///
///	@file 	UNIX/daemon.cpp
/// @brief 	Daemonize the application (run as a service)
///	@overview Run MPR applications in the background as a daemon (service). 
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr/mpr.h"

///////////////////////////////////// Code /////////////////////////////////////
#if BLD_FEATURE_RUN_AS_SERVICE

int Mpr::makeDaemon(int parentExit)
{
	struct sigaction	act, old;
	int					pid, status;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = (void (*)(int, siginfo_t*, void*)) SIG_DFL;
	sigemptyset(&act.sa_mask);

	act.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;
#if MACOSX || FREEBSD
	act.sa_flags |= SA_NODEFER;
#else
	act.sa_flags |= SA_NOMASK;
#endif

	if (sigaction(SIGCHLD, &act, &old) < 0) {
		mprError(MPR_L, MPR_USER, "Can't initialize signals");
		return MPR_ERR_BAD_STATE;
	}

	//
	//	Fork twice to get a free child with no parent
	//
	if ((pid = fork()) < 0) {
		mprError(MPR_L, MPR_LOG, "Fork failed for background operation");
		return MPR_ERR_GENERAL;

	} else if (pid == 0) {
		if ((pid = fork()) < 0) {
			mprError(MPR_L, MPR_LOG, "Second fork failed");
			exit(127);

		} else if (pid > 0) {
			//	Parent of second child -- must exit
			exit(0);
		}

		//
		//	This is the real process that will serve http requests
		//
		setsid();
		if (sigaction(SIGCHLD, &old, 0) < 0) {
			mprError(MPR_L, MPR_USER, "Can't restore signals");
			return MPR_ERR_BAD_STATE;
		}
		mprLog(2, "Switching to background operation\n");
		return 0;
	}

	//
	//	Original process waits for first child here. Must get child death
	//	notification with a successful exit status
	//
	while (waitpid(pid, &status, 0) != pid) {
		if (errno == EINTR) {
			mprSleep(100);
			continue;
		}
		mprError(MPR_L, MPR_LOG, "Can't wait for daemon parent.");
		exit(0);
	}
	if (WEXITSTATUS(status) != 0) {
		mprError(MPR_L, MPR_LOG, "Daemon parent had bad exit status.");
		exit(0);
	}

	if (sigaction(SIGCHLD, &old, 0) < 0) {
		mprError(MPR_L, MPR_USER, "Can't restore signals");
		return MPR_ERR_BAD_STATE;
	}

	if (parentExit) {
		exit(0);
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
#else

void mprMakeDaemonDummy() {}

#endif	// BLD_FEATURE_RUN_AS_SERVICE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
