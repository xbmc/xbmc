///
///	@file 	mpr.cpp
/// @brief 	Library and application initialization
/// @overview The user instantiates the MR class in their application, 
///		typically in main(). Users can initialize the library by 
///		creating an instance of the Mpr class, or they can start the 
///		Mpr services such as the thread-task, timer and select modules. 
///	@remarks This module is thread-safe.
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
/////////////////////////////////// Includes ///////////////////////////////////

#include	"mpr.h"

/////////////////////////////////// Locals /////////////////////////////////////

Mpr				*mpr;							// Global default Mpr instance
static bool		debugMode;						// Debugging or not

static char	copyright[] = 
	"Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.";


#if UNUSED
char *errMessages[] = {
	"Success", 
	"General error", 
	"Aborted", 
	"Already exists", 
	"Bad args", 
	"Bad format", 
	"Bad handle", 
	"Bad state", 
	"Bad syntax", 
	"Bad type", 
	"Bad value", 
	"Busy", 
	"Can't access", 
	"Can't complete", 
	"Can't create", 
	"Can't initialize", 
	"Can't open", 
	"Can't read", 
	"Can't write", 
	"Already deleted", 
	"Network error", 
	"Not found", 
	"Not initialized", 
	"Not ready", 
	"Read only", 
	"Timeout", 
	"Too many", 
	"Won't fit", 
	"Would block", 
};

#if WIN
char *windowsErrList[] =
{
    /*  0              */  "No error",
    /*  1 EPERM        */  "Operation not permitted",
    /*  2 ENOENT       */  "No such file or directory",
    /*  3 ESRCH        */  "No such process",
    /*  4 EINTR        */  "Interrupted function call",
    /*  5 EIO          */  "I/O error",
    /*  6 ENXIO        */  "No such device or address",
    /*  7 E2BIG        */  "Arg list too long",
    /*  8 ENOEXEC      */  "Exec format error",
    /*  9 EBADF        */  "Bad file number",
    /* 10 ECHILD       */  "No child processes",
    /* 11 EAGAIN       */  "Try again",
    /* 12 ENOMEM       */  "Out of memory",
    /* 13 EACCES       */  "Permission denied",
    /* 14 EFAULT       */  "Bad address",
    /* 15 ENOTBLK      */  "Unknown error",
    /* 16 EBUSY        */  "Resource busy",
    /* 17 EEXIST       */  "File exists",
    /* 18 EXDEV        */  "Improper link",
    /* 19 ENODEV       */  "No such device",
    /* 20 ENOTDIR      */  "Not a directory",
    /* 21 EISDIR       */  "Is a directory",
    /* 22 EINVAL       */  "Invalid argument",
    /* 23 ENFILE       */  "Too many open files in system",
    /* 24 EMFILE       */  "Too many open files",
    /* 25 ENOTTY       */  "Inappropriate I/O control operation",
    /* 26 ETXTBSY      */  "Unknown error",
    /* 27 EFBIG        */  "File too large",
    /* 28 ENOSPC       */  "No space left on device",
    /* 29 ESPIPE       */  "Invalid seek",
    /* 30 EROFS        */  "Read-only file system",
    /* 31 EMLINK       */  "Too many links",
    /* 32 EPIPE        */  "Broken pipe",
    /* 33 EDOM         */  "Domain error",
    /* 34 ERANGE       */  "Result too large",
    /* 35 EUCLEAN      */  "Unknown error",
    /* 36 EDEADLK      */  "Resource deadlock would occur",
    /* 37 UNKNOWN      */  "Unknown error",
    /* 38 ENAMETOOLONG */  "Filename too long",
    /* 39 ENOLCK       */  "No locks available",
    /* 40 ENOSYS       */  "Function not implemented",
    /* 41 ENOTEMPTY    */  "Directory not empty",
    /* 42 EILSEQ       */  "Illegal byte sequence",
    /* 43 ENETDOWN     */  "Network is down",
    /* 44 ECONNRESET   */  "Connection reset",
    /* 45 ECONNREFUSED */  "Connection refused",
    /* 46 EADDRINUSE   */  "Address already in use"

};

int windowsNerr = 47;
#endif
#endif // UNUSED

///////////////////////////// Forward Declarations /////////////////////////////

#if BLD_FEATURE_MULTITHREAD
static void serviceEventsWrapper(void *data, MprThread *tp);
#endif

//////////////////////////////////// Code //////////////////////////////////////

//
//	Initialize the MPR library and the appliation control object for the MPR. 
//

Mpr::Mpr(char *name)
{
	//
	//	We expect only one MR class -- store a reference for global use
	//	Prior to return of this method, beware that mpr-> is not fully 
	//	constructed!!!
	//
#if !VXWORKS
	//
	//	VxWorks does not clear variables on a re-load. For ease of debugging,
	//	we remove this assert.
	//
	mprAssert(mpr == 0);						// Should be only one instance
#endif

	mpr = this;

	appName = 0;
	appTitle = 0;
	buildType = 0;
	cpu = 0;
	domainName = 0;
	eventsThread = 0;
	flags = 0;
	headless = 0;
	os = 0;
	runAsService = 0;
	version = 0;

#if WIN
	appInstance = 0;
	hwnd = 0;
#endif

	appName = mprStrdup(name);				// Initial defaults
	appTitle = mprStrdup(name);			
	buildNumber = atoi(BLD_NUMBER);
	buildType = mprStrdup(BLD_TYPE);
	os = mprStrdup(BLD_HOST_OS);
	version = mprStrdup(BLD_VERSION);
	copyright[0] = copyright[0];			// Suppress compiler warning
	hostName = mprStrdup("localhost");
	serverName = mprStrdup("localhost");
	installDir = mprStrdup(".");
	cpu = mprStrdup(BLD_HOST_CPU);

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
	timeMutex = new MprMutex();
	eventsMutex = new MprMutex();
#endif

#if BLD_FEATURE_LOG
	logService = new MprLogService();
	defaultLog = new MprLogModule("default");
#endif
	platformInitialize();
	configSettings = new MprHashTable();

#if BLD_FEATURE_MULTITHREAD
	threadService = new MprThreadService();
#endif
#if BLD_FEATURE_CGI_MODULE
	cmdService = new MprCmdService();
#endif

	timerService = new MprTimerService();
	poolService = new MprPoolService("default");
	selectService = new MprSelectService();
	socketService = new MprSocketService();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Windup the MR
//

Mpr::~Mpr()
{
	mprLog(3, 0, "MPR Exiting\n");

	if (flags & MPR_STARTED && !(flags & MPR_STOPPED)) {
		stop(1);
	}

	delete socketService;
	delete selectService;
	delete poolService;
#if BLD_FEATURE_CGI_MODULE
	delete cmdService;
#endif
	delete timerService;

	//
	//	Log needs to know when the threadService has been deleted
	//
#if BLD_FEATURE_MULTITHREAD
	mutex->lock();
	delete mutex;
	timeMutex->lock();
	delete timeMutex;
	eventsMutex->lock();
	delete eventsMutex;
#endif
	delete configSettings;

	mprFree(appName);
	mprFree(appTitle);
	mprFree(buildType);
	mprFree(cpu);
	mprFree(domainName);
	mprFree(hostName);
	mprFree(installDir);
	mprFree(os);
	mprFree(serverName);
	mprFree(version);

	mprLog(3, 0, "--------- MPR Shutdown ----------\n");

#if BLD_DEBUG
	mprMemStop();
#endif

#if BLD_FEATURE_MULTITHREAD
	delete threadService;
	threadService = 0;
#endif

#if BLD_FEATURE_LOG
	delete defaultLog;
	logService->stop();
	delete logService;
#endif

	platformTerminate();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the run-time services: timer, thread-task, select
//

int Mpr::start(int startFlags)
{
	int		rc = 0;

#if BLD_FEATURE_LOG
	logService->start();
#endif

	//
	//	Note: users can create timers before the timerService is started.
	//	They won't run until we hit the main event loop in any case which 
	//	processes timer and select events
	// 
	rc += platformStart(startFlags);
#if BLD_FEATURE_MULTITHREAD
	rc += threadService->start();
#endif
	rc += poolService->start();
	rc += selectService->start();
	rc += timerService->start();
	rc += socketService->start();
#if BLD_FEATURE_CGI_MODULE
	rc += cmdService->start();
#endif

	if (rc != 0) {
		mprError(MPR_L, MPR_USER, "Can't start MPR services");
		return MPR_ERR_CANT_INITIALIZE;
	}

#if BLD_FEATURE_MULTITHREAD
	if (startFlags & MPR_SERVICE_THREAD) {
		startEventsThread();
	}
#endif

	flags |= MPR_STARTED | (startFlags & MPR_USER_START_FLAGS);
	mprLog(3, "MPR services are ready\n");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the MR service
//

int Mpr::stop(bool immediateStop)
{
	//
	//	Graceful termination
	//
	this->terminate(1);

#if BLD_FEATURE_LOG
	logService->shuttingDown();
#endif

	//
	//	All stop methods are idempotent
	//
	poolService->stop((immediateStop) ? 0 : MPR_TIMEOUT_STOP_TASK);
#if BLD_FEATURE_CGI_MODULE
	cmdService->stop();
#endif
	socketService->stop();
#if BLD_FEATURE_MULTITHREAD
	threadService->stop((immediateStop) ? 0 : MPR_TIMEOUT_STOP_THREAD);
#endif
	selectService->stop();
	timerService->stop();

	//
	//	Don't stop the log service as we want logging & trace to the bitter end
	//		logService->stop();
	//
	platformStop();
	flags |= MPR_STOPPED;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Thread to service timer and socket events. Used only if the user does not
//	have their own main event loop.
//

void Mpr::startEventsThread()
{
	MprThread	*tp;

	mprLog(MPR_CONFIG, "Starting service thread\n");
	tp = new MprThread(serviceEventsWrapper, MPR_NORMAL_PRIORITY, 0, "event");
	tp->start();
	eventsThread = 1;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Thread main for serviceEvents
//

static void serviceEventsWrapper(void *data, MprThread *tp)
{
	mpr->serviceEvents(0, -1);
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Service timer and select/socket events on a service thread. Only used if
//	multi-threaded and the users does not define their own event loop.
//

void Mpr::serviceEvents(bool loopOnce, int maxTimeout)
{
	fd_set			readFds, writeFds, exceptFds;
	fd_set			readInterest, writeInterest, exceptInterest;
	int				maxFd, till, lastGet, readyFds, err;

#if WIN
	if (getAsyncSelectMode() & MPR_ASYNC_SELECT) {
		serviceWinEvents(loopOnce, maxTimeout);
		return;
	}
#endif

#if BLD_FEATURE_MULTITHREAD
	//
	//	If we are polling and someone is already here, simply return.
	//	
	if (loopOnce) {
		if (eventsMutex->tryLock() < 0) {
			/*
			 * 	TODO - this is slow. Should sleep on an event that the other thread could signal on.
			 */
			mprSleep(10);
			return;
		}
	} else {
		eventsMutex->lock();
	}

	mprGetCurrentThread()->setPriority(MPR_SELECT_PRIORITY);

	//
	//	Set here also incase a user calls serviceEvents manually
	//
	eventsThread = 1;
#endif

	lastGet = -1;
	maxFd = 0;
	FD_ZERO(&readInterest);
	FD_ZERO(&writeInterest);
	FD_ZERO(&exceptInterest);

	do {

		if (runTimers() > 0) {
			till = 0;
		} else {
			till = getIdleTime();
		}

		//
		//	This will run tasks if poolThreads == 0 (single threaded). If 
		//	multithreaded, the thread pool will run tasks
		//
		if (runTasks() > 0) {				// Returns > 0 if more work to do
			till = 0;						// So don't block in select
		}

		if (getFds(&readInterest, &writeInterest, &exceptInterest, &maxFd, 
				&lastGet)) {
			//
			//	Masks have been rebuilt, so add user fds here ....
			//
		}

		//
		//	Copy as select will modify readFds etc.
		//
		memcpy((void*) &readFds, (void*) &readInterest, sizeof(readFds));
		memcpy((void*) &writeFds, (void*) &writeInterest, sizeof(readFds));
		memcpy((void*) &exceptFds, (void*) &exceptInterest, sizeof(exceptFds));

		if (maxTimeout > 0) {
			till = min(till, maxTimeout);
		}

#if VXWORKS
	#if BLD_HOST_CPU_ARCH == MPR_CPU_PPC
		//
		//	Work around to solve hang when large simultaneous downloads
		//
		if (till < 32) {
			till = 32;
		}
	#endif
#endif
		readyFds =  mpr->selectService->doSelect(maxFd, &readFds, &writeFds,
			&exceptFds, till);

		if (readyFds < 0) {
			err = mprGetOsError();
			if (err != EINTR) {
				static int warnOnce = 0;
				if (warnOnce++ == 0) {
					mprLog(0, "WARNING: select failed, errno %d\n", errno);
				}
				mpr->selectService->repairFds();
			}

		} else if (readyFds > 0) {
			serviceIO(readyFds, &readFds, &writeFds, &exceptFds);
		}

	} while (!isExiting() && !loopOnce);

#if BLD_FEATURE_MULTITHREAD
	eventsThread = 0;
	eventsMutex->unlock();
#endif
}

////////////////////////////////////////////////////////////////////////////////
#if WIN

void Mpr::serviceWinEvents(bool loopOnce, int maxTimeout)
{
	MSG		msg;
	int		till;

	do {
		if (runTimers() > 0) {
			till = 0;
		} else {
			till = getIdleTime();
		}

		//
		//	This will run tasks if poolThreads == 0 (single threaded). If 
		//	multithreaded, the thread pool will run tasks
		//
		if (runTasks() > 0) {				// Returns > 0 if more work to do
			till = 0;						// So don't block in select
		}
		SetTimer(hwnd, 0, till, NULL);

		//
		//	Socket events will be serviced in the msgProc
		//
		if (GetMessage(&msg, NULL, 0, 0) == 0) {
			//	WM_QUIT received
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	} while (!isExiting() && !loopOnce);
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

int Mpr::runTimers()
{ 
	return timerService->runTimers(); 
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

int Mpr::runTasks()
{ 
	return poolService->runTasks();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

int Mpr::getIdleTime()
{ 
	return timerService->getIdleTime();
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

MprThread *Mpr::getCurrentThread()
{
	return threadService->getCurrentThread(); 
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

void Mpr::setPriority(int pri)
{
	threadService->getCurrentThread()->setPriority(pri);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience functions
//

MprThread *mprGetCurrentThread()
{
	return mpr->threadService->getCurrentThread();
}

////////////////////////////////////////////////////////////////////////////////

int mprGetCurrentThreadId()
{
	return mpr->threadService->getCurrentThreadId();
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setMinPoolThreads(int n)
{
	poolService->setMinPoolThreads(n);
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setMaxPoolThreads(int n)
{
	poolService->setMaxPoolThreads(n);
}

////////////////////////////////////////////////////////////////////////////////

int Mpr::getMinPoolThreads()
{
	return poolService->getMinPoolThreads();
}

////////////////////////////////////////////////////////////////////////////////

int Mpr::getMaxPoolThreads()
{
	return poolService->getMaxPoolThreads();
}

////////////////////////////////////////////////////////////////////////////////

int mprGetMaxPoolThreads()
{
	return mpr->poolService->getMaxPoolThreads();
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
//
//	Exit the mpr gracefully. Instruct the event loop to exit.
//

void Mpr::terminate(bool graceful)
{
	mprLog(MPR_CONFIG, 0, "MPR: instructed to terminate\n");

	if (! graceful) {
		exit(0);
	}
	lock();
	flags |= MPR_EXITING;
	unlock();
	selectService->awaken();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return true if Mpr is exiting
//

bool Mpr::isExiting()
{
	return flags & MPR_EXITING;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

int Mpr::getFds(fd_set *readInterest, fd_set *writeInterest, 
	fd_set *exceptInterest, int *maxFd, int *lastGet)
{
	return mpr->selectService->getFds(readInterest, writeInterest, 
		exceptInterest, maxFd, lastGet);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convenience function
//

void Mpr::serviceIO(int readyFds, fd_set *readFds, fd_set *writeFds, 
	fd_set *exceptFds)
{
	mpr->selectService->serviceIO(readyFds, readFds, writeFds, exceptFds);
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
//
//	Convenience function
//

void Mpr::serviceIO(int sock, int winMask)
{
	mpr->selectService->serviceIO(sock, winMask);
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Set the applications name (one word by convention). Used mostly in creating
//	pathnames for application components.
//

void Mpr::setAppName(char *s)
{
	lock();
	if (appName) {
		mprFree(appName);
	}
	appName = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getAppName()
{
	return appName;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Set the description of the application. Short, but can be multiword.
//	Used where ever we need to visibly refer to the application.
//
void Mpr::setAppTitle(char *s)
{
	lock();
	if (appTitle) {
		mprFree(appTitle);
	}
	appTitle = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getAppTitle()
{
	return appTitle;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setBuildType(char *s)
{
	lock();
	if (buildType) {
		mprFree(buildType);
	}
	buildType = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getBuildType()
{
	/* FUTURE -- not thread safe */
	return buildType;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setBuildNumber(int num)
{
	/* FUTURE -- waste of time locking here if we don't lock in the get code */
	mprAssert(0 <= num && num < 99999);
	lock();
	buildNumber = num;
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

int Mpr::getBuildNumber()
{
	return buildNumber;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setOs(char *s)
{
	lock();
	if (os) {
		mprFree(os);
	}
	os = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getOs()
{
	return os;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setCpu(char *s)
{
	lock();
	if (cpu) {
		mprFree(cpu);
	}
	cpu = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getCpu()
{
	return cpu;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Version strings are: maj.min.patch.build. E.g. 2.0.1.4
//

void Mpr::setVersion(char *s)
{
	lock();
	if (version) {
		mprFree(version);
	}
	version = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////

char *Mpr::getVersion()
{
	return version;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LOG

int Mpr::setLogSpec(char *file)
{
	if (logService == 0) {
		return MPR_ERR_CANT_INITIALIZE;
	}
	return logService->setLogSpec(file);
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::addListener(MprLogListener *lp)
{
	mprAssert(logService);

	logService->addListener(lp);
}

#endif // BLD_FEATURE_LOG
////////////////////////////////////////////////////////////////////////////////

void Mpr::setInstallDir(char *dir)
{
	lock();
	if (installDir) {
		mprFree(installDir);
	}
	installDir = mprStrdup(dir);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the applications installation directory
//

char *Mpr::getInstallDir()
{
	return installDir;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Full host name with domain. E.g. "server.domain.com"
//

void Mpr::setHostName(char *s)
{
	lock();
	if (hostName) {
		mprFree(hostName);
	}
	hostName = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the fully qualified host name
//

char *Mpr::getHostName()
{
	return hostName;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Server name portion (no domain name)
//

void Mpr::setServerName(char *s)
{
	lock();
	if (serverName) {
		mprFree(serverName);
	}
	serverName = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the server name
//

char *Mpr::getServerName()
{
	return serverName;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Set the domain name 
//

void Mpr::setDomainName(char *s)
{
	lock();
	if (domainName) {
		mprFree(domainName);
	}
	domainName = mprStrdup(s);
	unlock();
	return;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the domain name
//

char *Mpr::getDomainName()
{
	return domainName;
}

////////////////////////////////////////////////////////////////////////////////
#if WIN

bool Mpr::getAsyncSelectMode() 
{ 
	return selectService->getAsyncSelectMode();
}

////////////////////////////////////////////////////////////////////////////////

long Mpr::getInst()
{
	return (long) appInstance;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setInst(long inst)
{
	appInstance = inst;
}

#endif
////////////////////////////////////////////////////////////////////////////////

bool Mpr::isService()
{
	return runAsService;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setService(bool service)
{
	runAsService = service;
}

////////////////////////////////////////////////////////////////////////////////
#if WIN

HWND Mpr::getHwnd()
{
	return hwnd;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setHwnd(HWND h)
{
	hwnd = h;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setSocketHwnd(HWND h) 
{
	selectService->setHwnd(h);
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setSocketMessage(int m) 
{ 
	selectService->setMessage(m);
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setAsyncSelectMode(bool on) 
{ 
	selectService->setAsyncSelectMode(on);
}

#endif

////////////////////////////////////////////////////////////////////////////////

int Mpr::getHeadless()
{
	return headless;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::setHeadless(int flag)
{
	headless = flag;
}

////////////////////////////////////////////////////////////////////////////////

Mpr *mprGetMpr()
{
	return mpr;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get a configuration string value
//

char *Mpr::getConfigStr(char *key, char *defaultValue)
{
#if BLD_FEATURE_XML_CONFIG
	char	*value;

	if (readXmlStr(configSettings, key, &value) < 0) {
		return defaultValue;
	}
	return value;
#else
	return defaultValue;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get a configuration integer value
//

int Mpr::getConfigInt(char *key, int defaultValue)
{	
#if BLD_FEATURE_XML_CONFIG
	int		value;

	if (readXmlInt(configSettings, key, &value) < 0) {
		return defaultValue;
	}
	return value;
#else
	return defaultValue;
#endif
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_XML_CONFIG
//
//	Configure the MR library by opening the product.xml file.
//

int Mpr::configure(char *configFile)
{
	char	*logSpec;

	//
	//	Open the product.xml file
	//
	if (configFile == 0 || *configFile == '\0') {
		return MPR_ERR_BAD_ARGS;
	}

	if (openXmlFile(configFile) < 0) {
		mprError(MPR_L, MPR_USER, 
			"Can't open product configuration file: %s", configFile);
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Extract the standard settings
	//
	setAppTitle(getConfigStr("appTitle", "mprAppTitle"));
	setAppName(getConfigStr("appName", "mprAppName"));
	setHeadless(getConfigInt("headless", headless));

	//
	//	Redirect error log output if required
	//
	logSpec = getConfigStr("logSpec", 0);
	if (logSpec && *logSpec) {
		if (logService->isLogging()) {
			mprError(MPR_L, MPR_LOG, 
				"Logging already enabled. Ignoring logSpec directive in %s", 
				configFile);
		} else {
			logService->stop();
			logService->setLogSpec(logSpec);
			logService->start();
		}
	}

#if BLD_FEATURE_MULTITHREAD
	//
	//	Define the thread task limits
	//
	poolService->setMaxPoolThreads(getConfigInt("maxMprPoolThreads", 0));
	poolService->setMinPoolThreads(getConfigInt("minMprPoolThreads", 0));
#endif

	//
	//	Select breakout port
	//
	selectService->setPort(getConfigInt("selectPort", -1));
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Basic XML parser to extract name / value parirs and store in a symbol table.
//

int Mpr::openXmlFile(char *path)
{
	struct stat	sbuf;
	char		*buf, *name, *cp, *value;
	int			fd, keywordCount, len;

	mprAssert(path && *path);

	//
	//	Read the XML file entirely into memory
	//
	if ((fd = open(path, O_RDONLY)) < 0) {
		mprError(MPR_L, MPR_USER, "Can't open file: %s", path);
		return -1;
	}

	stat(path, &sbuf);
	buf = new char[sbuf.st_size + 1];

	if ((len = read(fd, buf, sbuf.st_size)) < 0) {
		mprError(MPR_L, MPR_USER, "Can't read file: %s", path);
		delete[] buf;
		return -1;
	}
	buf[len] = '\0';
	close(fd);

	//
	//	Read all key values into a symbol table. This is hard-coded parsing. 
	//	We expect a two-level XML tree with all name tags directly under 
	//	the root tag.
	//
	keywordCount = 0;
	cp = strstr(buf, "<config ");
	while (cp && *cp) {
		cp++;
		if ((name = strchr(cp, '<')) == NULL) {
			break;
		}
		name++;
		if (name[0] == '!' && name[1] == '-' && name[2] == '-') {
			cp = name;
			continue;
		}
		if ((value = strchr(name, '>')) == NULL) {
			break;
		}
		*value++ = '\0';
		if ((cp = strchr(value, '<')) == NULL) {
			break;
		}
		*cp++ = '\0';
		if ((cp = strchr(cp, '>')) == NULL) {
			break;
		}
		configSettings->insert(new StringHashEntry(name, value));
		keywordCount++;
	}

	//
	//	Sanity check on the parsing. We should have found at least 4 keywords
	//
	if (keywordCount < 4) {
		mprError(MPR_L, MPR_USER, "Can't parse file: %s", path);
	}
	
	delete[] buf;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read a string configuration item. MprUniType is set to NULL on an error
//

int Mpr::readXmlStr(MprHashTable *symTab, char *key, char **value)
{
	StringHashEntry		*ep;

	mprAssert(key);
	mprAssert(value);

	if ((ep = (StringHashEntry*) symTab->lookup(key)) == 0) {
		*value = 0;
		return MPR_ERR_NOT_FOUND;
	}
	*value = ep->getValue();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read an integer configuration item. MprUniType is set to -1 on an error.
//

int Mpr::readXmlInt(MprHashTable *symTab, char *key, int *value)
{
	StringHashEntry		*ep;

	mprAssert(key);
	mprAssert(value);

	//
	//	FUTURE -- could create IntHashEntry or use Unitype
	//
	if ((ep = (StringHashEntry*) symTab->lookup(key)) == 0) {
		*value = -1;
		return MPR_ERR_NOT_FOUND;
	}
	*value = atoi(ep->getValue());
	return 0;
}

#endif // BLD_FEATURE_XML_CONFIG
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// C API ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern "C" {

bool mprGetDebugMode()
{
	return debugMode;
}

////////////////////////////////////////////////////////////////////////////////

void mprSetDebugMode(bool on)
{
	debugMode = on;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map the O/S error code to portable error codes.
//

int mprGetOsError()
{
#if WIN
	int		rc;
	rc = GetLastError();

	//
	//	Client has closed the pipe
	//
	if (rc == ERROR_NO_DATA) {
		return EPIPE;
	}
	return rc;
#else
	return errno;
#endif
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED

char *mprGetErrorMsg(int err)
{
	//
	//	MPR_ERR_BASE is -200
	//
	if (err < MPR_ERR_BASE) {
		err = MPR_ERR_BASE - err;
		if (err < 0 || err >= (MPR_ERR_BASE - MPR_ERR_MAX)) {
			return "Bad error code";
		}
		return errMessages[err];
	} else {
		//
		//	Negative O/S error code. Map to a positive standard Posix error.
		//
		err = -err;
#if CYGWIN || LINUX || MACOSX || VXWORKS || FREEBSD
		if (err < 0) {
			return "Bad O/S error code";
		}
		//
		//	FUTURE: we currently only use this inside thread locks, but 
		//	this should be cleaned up.
		//
		static char buf[80];
		return strerror_r(err, buf, sizeof(buf) - 1);
		// return (char*) buf;
#endif
#if SOLARIS
		if (err < 0) {
			return (char*) "Bad O/S error code";
		}
		//
		//	FUTURE: we currently only use this inside thread locks, but 
		//	this should be cleaned up.
		//
		return strerror(err);
#endif
#if WIN
		if (err < 0 || err >= windowsNerr) {
			return "Bad O/S error code";
		}
		return (char*) windowsErrList[err];
#endif
	}
}

#endif
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

char *mprGetCurrentThreadName()
{
	return mpr->threadService->getCurrentThread()->getName();
}

#endif
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LICENSE

static uint64 mprKey[1] = {
	MPR_LIC_KEY,
};

////////////////////////////////////////////////////////////////////////////////

uint mprRotate(uint x, int count) 
{
	uint	y;
	if (count > 0) {
		y = ((x << count) | (x >> ((sizeof(x) * 8) - count)));
	} else {
		y = ((x >> -count) | (x << ((sizeof(x) * 8) + count)));
	}
	return y;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Validate a license
//

int mprValidateLicense(uint64 licenseKey, char **product, 
	int *version, int *serial, int *productBits)
{
	static		int times = 0;
	uint64		bits;
	int			i, seed, major, minor, patch, checkBits, sum;
	int			productId, serialNum;

	mprAssert(licenseKey != 0);

	if (times++ > 6) {
		mprSleep((times / 2) * 1000);
	}

	mprDecrypt((uchar*) &bits, sizeof(bits), (uchar*) &licenseKey, 
		sizeof(licenseKey), mprKey, 1);

	seed = (int) ((bits >> 55) & 0x7F);
	serialNum = (int) ((bits >> 39) & 0xFFFF);

	if (productBits) {
		*productBits = (int) ((bits >> 31) & 0xFF);
	}

	productId = (int) ((bits >> 26) & 0x1F);
	if (product) {
		switch (productId) {
		case MPR_PRODUCT_EVAL:
			*product = "eval";
			break;
		case MPR_PRODUCT_APPWEB:
			*product = "appweb";
			break;
		case MPR_PRODUCT_MYAPPWEB:
			*product = "myAppweb";
			break;
		case MPR_PRODUCT_DEVICEMANAGER:
			*product = "deviceManager";
			break;
		default:
			*product = "unknown product";
			break;
		}
	}
	major =	(int) ((bits >> 22) & 0xF);
	minor =	(int) ((bits >> 19) & 0x7);
	patch =	(int) ((bits >> 16) & 0x7);
	checkBits = (int) (bits & 0xFFFF);

	sum = 0;
	for (i = 1; i < 4; i++) {
		sum ^= (int) ((bits >> (i * 16)) & 0xFFFF);
	}
	if (checkBits != sum) {
		return MPR_ERR_BAD_ARGS;
	}

	if (seed != MPR_LIC_SEED) {
		return MPR_ERR_BAD_ARGS;
	}
	if (serialNum > MPR_LIC_MAX_SERIAL) {
	}
	if (major > 16) {
		return MPR_ERR_BAD_ARGS;
	}
	if (minor > 7) {
		return MPR_ERR_BAD_ARGS;
	}
	if (patch > 7) {
		return MPR_ERR_BAD_ARGS;
	}

	if (version) {
		*version = ((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | 
			(patch & 0xFF);
	}

	if (productId < 0 || productId > MPR_PRODUCT_MAX) {
		mprError(MPR_L, MPR_USER, "Unknown product: %d", productId);
		return MPR_ERR_BAD_ARGS;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Basic crypto routine. Modified Feistel cipher suitable for encrypting 
//	small blocks of input.
//
static int mprCrypto(uchar *output, int outputSize, uchar *input, 
	int inputSize, uint64 *key, int keySize, int cryptDir)
{
	uint	left, right;
	uint	subKey;
	uchar	*op;
	int		j, i, n, size;

	keySize /= sizeof(uchar);

	if (inputSize <= 0) {
		if (input) {
			inputSize = strlen((char*) input) + 1;
		}
	}

	if (inputSize >= 8) {
		if (outputSize < inputSize) {
			return MPR_ERR_WONT_FIT;
			return -1;
		}
		if ((outputSize % 8) != 0) {
			if ((outputSize / 8) <= (inputSize / 8)) {
				return MPR_ERR_BAD_ARGS;
			}
		}
	}

	if (input != 0) { 
		if (output != input) {
			for (i = 0; i < inputSize; i++) {
				output[i] = input[i];
			}
			for (; i < outputSize; i++) {
				output[i] = 0;
			}
		}
		outputSize = inputSize;
	}

	//	FUTURE -- ENDIAN, 64 BIT
	size = (outputSize + 7) & ~0x7;

	for (j = 0; j < size; j += sizeof(uint64)) {
		op = &output[j];
		left = ((uint*) op)[0];
		right = ((uint*) op)[1];

		//
		//	Compute n rounds over each pair.
		//
		for (n = 0; n < MPR_CRYPT_ROUNDS; n++) {

			uint shifted;

			if (cryptDir > 0) {
				//
				//	R = (L ^ Fn(R, key)) <<< L);
				//
				subKey = (uint) (key[(n / 2) % keySize] >> ((n % 2) * 32));

				shifted = left ^ right;
				right = mprRotate(shifted, left & 0x1F);
				right += subKey;

				shifted = left ^ right;
				left = mprRotate(shifted, right & 0x1F);
				left += subKey;

			} else {
				//
				//	R = Fn((R >>> L) ^ L);
				//
				int index = MPR_CRYPT_ROUNDS - n - 1;
				subKey = (uint) (key[(index / 2) % keySize] >> 
					((index % 2) * 32));

				left = left - subKey;
				left = mprRotate(left, - (int) (right & 0x1F));
				left ^= right;

				right = right - subKey;
				right = mprRotate(right, - (int) (left & 0x1F));
				right ^= left;

			}
		}
		((uint*) op)[0] = left;
		((uint*) op)[1] = right;
	}
	return size;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Crypt block.
//

int mprCrypt(uchar *output, int outputSize, uchar *input, int inputSize, 
	uint64 *key, int keySize)
{
	return mprCrypto(output, outputSize, input, inputSize, key, keySize, 1);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Decrypt block. 
//

int mprDecrypt(uchar *output, int outputSize, uchar *input, int inputSize, 
	uint64 *key, int keySize)
{
	return mprCrypto(output, outputSize, input, inputSize, key, keySize, -1);
}

#endif // BLD_FEATURE_LICENSE
////////////////////////////////////////////////////////////////////////////////
} // extern "C"

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
