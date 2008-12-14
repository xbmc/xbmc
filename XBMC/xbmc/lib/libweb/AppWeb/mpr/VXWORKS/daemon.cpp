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
	int		pid, status;

	//
	//	Fork twice to get a free child with no parent
	//
	if ((pid = vfork()) < 0) {
		mprError(MPR_L, MPR_LOG, "Fork failed for background operation");
		return MPR_ERR_GENERAL;

	} else if (pid == 0) {
		if ((pid = vfork()) < 0) {
			mprError(MPR_L, MPR_LOG, "Second fork failed");
			exit(127);

		} else if (pid > 0) {
			//	Parent of second child -- must exit
			exit(0);
		}
		setsid();
		mprLog(2, "Switching to background operation\n");
		return 0;
	}

	//
	//	Original process waits for first child here. Must get child death
	//	notification with a successful exit status
	//
	if (waitpid(pid, &status, 0) != pid || WEXITSTATUS(status) != 0) {
		return MPR_ERR_CANT_ACCESS;
	}
	if (parentExit) {
		exit(0);
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
#endif	// BLD_FEATURE_RUN_AS_SERVICE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
