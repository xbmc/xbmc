///
///	@file 	WIN/daemon.cpp
/// @brief 	Daemonize the MPR (run as a service)
/// @overview Run MPR applications in the background as a daemon (service). 
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

/////////////////////////////// Forward Declarations ///////////////////////////
#if BLD_FEATURE_RUN_AS_SERVICE

static void		WINAPI svcCback(ulong code);
static int		tellScm(long state, long exitCode, long wait);

//////////////////////////////////// Locals ////////////////////////////////////

static HANDLE					threadHandle;
static HANDLE					waitEvent;
static SERVICE_STATUS			svcStatus;
static SERVICE_STATUS_HANDLE	svcHandle;
static int						svcStopped;
static SERVICE_TABLE_ENTRY		svcTable[] = {
	{ "default",	0	},
	{ 0,			0	}
};
static MprWinService	*winService;		// Global MprWinService object

///////////////////////////////////// Code /////////////////////////////////////
//
//	Create a MprWinService control object
// 

MprWinService::MprWinService(char *name)
{
	svcName = mprStrdup(name);
	winService = this;
	svcTable[0].lpServiceProc = 0;
}

////////////////////////////////////////////////////////////////////////////////

MprWinService::~MprWinService()
{
	mprFree(svcName);
	winService = 0;
}

////////////////////////////////////////////////////////////////////////////////

int MprWinService::startDispatcher(LPSERVICE_MAIN_FUNCTION svcMain)
{
	SC_HANDLE		mgr;
	char			name[80];
	ulong			len;

	if (!(mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
		mprError(MPR_L, MPR_LOG, "Can't open service manager");
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Is the service installed?
	// 
	len = sizeof(name);
	if (GetServiceDisplayName(mgr, svcName, name, &len) == 0) {
		CloseServiceHandle(mgr);
		return MPR_ERR_CANT_READ;
	}

	//
	//	Register this service with the SCM. This call will block and consume
	//	the main thread if the service is installed and the app was started by
	//	the SCM. If started manually, this routine will return 0.
	// 
	svcTable[0].lpServiceProc = svcMain;
	if (StartServiceCtrlDispatcher(svcTable) == 0) {
		int rc = GetLastError();
		//	Failure
		mprLog(MPR_CONFIG, "Could not start the service control dispatcher\n");
		return MPR_ERR_CANT_INITIALIZE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Should be called first thing after the service entry point is called by
//	the service manager
//

int MprWinService::registerService(HANDLE thread, HANDLE event)
{
	threadHandle = thread;
	waitEvent = event;

	svcHandle = RegisterServiceCtrlHandler(svcName, svcCback);
	if (svcHandle == 0) {
		mprError(MPR_L, MPR_LOG, "Can't register handler: %x", GetLastError());
		return MPR_ERR_CANT_INITIALIZE;
	}

	//
	//	Report the svcStatus to the service control manager.
	// 
	svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	svcStatus.dwServiceSpecificExitCode = 0;
	if (!tellScm(SERVICE_START_PENDING, NO_ERROR, 1000)) {
		tellScm(SERVICE_STOPPED, 0, 0);
		return MPR_ERR_CANT_WRITE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprWinService::updateStatus(int status, int exitCode)
{
	tellScm(status, exitCode, 10000);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Service callback. Invoked by the SCM.
// 

static void WINAPI svcCback(ulong cmd)
{
	mprLog(MPR_CONFIG, "svcCback cmd 0x%x %d.\n", cmd, cmd);

	switch(cmd) {
	case SERVICE_CONTROL_INTERROGATE:
		break;

	case SERVICE_CONTROL_PAUSE:
		SuspendThread(threadHandle);
		svcStatus.dwCurrentState = SERVICE_PAUSED;
		break;

	case SERVICE_CONTROL_STOP:
		winService->stop(SERVICE_CONTROL_STOP);
		break;

	case SERVICE_CONTROL_CONTINUE:
		ResumeThread(threadHandle);
		svcStatus.dwCurrentState = SERVICE_RUNNING;
		break;

	case SERVICE_CONTROL_SHUTDOWN:
		winService->stop(SERVICE_CONTROL_SHUTDOWN);
		return;

	default:
		break;
	}
	tellScm(svcStatus.dwCurrentState, NO_ERROR, 0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Install the window's service
// 

int MprWinService::install(char *displayName, char *cmd)
{
	SC_HANDLE	svc, mgr;

	mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (! mgr) {
		mprError(MPR_L, MPR_LOG, "Can't open service manager");
		return MPR_ERR_CANT_ACCESS;
	}

	//
	//	Install this app as a service
	// 
	svc = OpenService(mgr, svcName, SERVICE_ALL_ACCESS);
	if (svc == NULL) {
		svc = CreateService(mgr, svcName, displayName, SERVICE_ALL_ACCESS,
			SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, cmd, NULL, NULL, 
			"", NULL, NULL);
		if (! svc) {
			mprError(MPR_L, MPR_LOG, "Can't create service: 0x%x == %d", 
				GetLastError(), GetLastError());
			CloseServiceHandle(mgr);
			return MPR_ERR_CANT_CREATE;
		}
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(mgr);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove the window's service
// 

int MprWinService::remove(int removeFromScmDb)
{
	SC_HANDLE	svc, mgr;

	mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (! mgr) {
		mprError(MPR_L, MPR_LOG, "Can't open service manager");
		return MPR_ERR_CANT_ACCESS;
	}
	svc = OpenService(mgr, svcName, SERVICE_ALL_ACCESS);
	if (! svc) {
		CloseServiceHandle(mgr);
		mprError(MPR_L, MPR_LOG, "Can't open service");
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Stop the application
	//
	mprGetMpr()->killMpr();

	if (ControlService(svc, SERVICE_CONTROL_STOP, &svcStatus)) {
		mprSleep(500);

		while (QueryServiceStatus(svc, &svcStatus)) {
			if (svcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
				mprSleep(250);
			} else {
				break;
			}
		}
		if (svcStatus.dwCurrentState != SERVICE_STOPPED) {
			mprError(MPR_L, MPR_LOG, "Can't stop service: %x", GetLastError());
		}
	}

	if (removeFromScmDb && !DeleteService(svc)) {
		mprError(MPR_L, MPR_LOG, "Can't delete service: %x", GetLastError());
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(mgr);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the window's service
// 

int MprWinService::start()
{
	SC_HANDLE	svc, mgr;

	mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (! mgr) {
		mprError(MPR_L, MPR_LOG, "Can't open service manager");
		return MPR_ERR_CANT_ACCESS;
	}

	svc = OpenService(mgr, svcName, SERVICE_ALL_ACCESS);
	if (! svc) {
		mprError(MPR_L, MPR_LOG, "Can't open service");
		CloseServiceHandle(mgr);
		return MPR_ERR_CANT_OPEN;
	}

	if (! StartService(svc, 0, NULL)) {
		mprError(MPR_L, MPR_LOG, "Can't start service: %x", GetLastError());
		return MPR_ERR_CANT_INITIALIZE;
	}

	CloseServiceHandle(svc);
	CloseServiceHandle(mgr);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the service in the current process. This is a bit confusing.
//	remove(0) will stop the service in another process.
// 

int MprWinService::stop(int cmd)
{
	int		exitCode;

	svcStopped++;
	mprGetMpr()->terminate(1);

	if (cmd == SERVICE_CONTROL_SHUTDOWN) {
		return 0;
	}

	SetEvent(waitEvent);
	svcStatus.dwCurrentState = SERVICE_STOP_PENDING;
	tellScm(svcStatus.dwCurrentState, NO_ERROR, 1000);

	exitCode = 0;
	GetExitCodeThread(threadHandle, (ulong*) &exitCode);
	while (exitCode == STILL_ACTIVE) {
		GetExitCodeThread(threadHandle, (ulong*) &exitCode);
		mprSleep(100);
		tellScm(svcStatus.dwCurrentState, NO_ERROR, 125);
	}
	svcStatus.dwCurrentState = SERVICE_STOPPED;
	tellScm(svcStatus.dwCurrentState, exitCode, 0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Tell SCM our current status
// 

static int tellScm(long state, long exitCode, long wait)
{
	static ulong generation = 1;

	svcStatus.dwWaitHint = wait;
	svcStatus.dwCurrentState = state;
	svcStatus.dwWin32ExitCode = exitCode;

	if (state == SERVICE_START_PENDING) {
		svcStatus.dwControlsAccepted = 0;
	} else {
		svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
			SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
	}

	if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED)) {
		svcStatus.dwCheckPoint = 0;
	} else {
		svcStatus.dwCheckPoint = generation++;
	}

	//
	//	Report the svcStatus of the service to the service control manager
	// 
	return SetServiceStatus(svcHandle, &svcStatus);
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
