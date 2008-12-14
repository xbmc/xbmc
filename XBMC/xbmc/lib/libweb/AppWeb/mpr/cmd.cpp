///
///	@file 	cmd.cpp
/// @brief 	Run external commands
/// @overview This modules provides a cross-platform command execution
///		abstraction. Per O/S specifics are contained in */os.cpp.
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

#include	"mpr.h"

#if BLD_FEATURE_CGI_MODULE
//
//	FUTURE -- make this its own define so it can be used without CGI
//
static void cmdWatcherWrapper(void *data, MprTimer *tp);

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprCmdService /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create the run service
//

MprCmdService::MprCmdService()
{
	int		i;

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
	timer = 0;
	for (i = 0; i < MPR_CMD_REAP_MAX; i++) {
		children[i].pid = 0;
		children[i].exitStatus = 0;
	}
#if CYGWIN || LINUX || SOLARIS || VXWORKS || MACOSX
	initSignals();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Terminate the run service
//

MprCmdService::~MprCmdService()
{
	lock();

	//
	//	Timers will delete themselves in cmdWatcherTimer. Must not displose
	//	of timer here.
	//

#if BLD_DEBUG
	if (cmdList.getNumItems() > 0) {
		mprError(MPR_L, MPR_LOG, "Exiting with %d run commands unfreed",
			cmdList.getNumItems());
	}
#endif
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#else
	mutex = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////

int MprCmdService::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop all processes
//

int MprCmdService::stop()
{
	MprCmd		*cp, *nextp;

	lock();
	cp = (MprCmd*) cmdList.getFirst();
	while (cp) {
		nextp = (MprCmd*) cmdList.getNext(cp);
		if (cp->isRunning()) {
			cp->stop();
		}
		cp = nextp;
	}
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Insert a new run command
//

void MprCmdService::insertCmd(MprCmd *cp)
{
	lock();
	cmdList.insert(cp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove a run command
//

void MprCmdService::removeCmd(MprCmd *cp)
{
	lock();
	cmdList.remove(cp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprCmdService::startWatcher()
{

	lock();
	mprLog(7, "startWatcher timer is %x\n", timer);
	if (timer == 0) {
		//	FUTURE -- need idle timer
		timer = new MprTimer(25, cmdWatcherWrapper, (void*) this);
		mprLog(7, "startWatcher creates new timer %x\n", timer);
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Must be very careful here. The childDeath signal handler will set the
//	children[] data fields. There is a race here so we must ONLY do atomic 
//	operations on these data arrays. Locking won't protect against signal handlers.
//

static void cmdWatcherWrapper(void *data, MprTimer *tp)
{
	mprGetMpr()->cmdService->cmdWatcherTimer(tp);
}

////////////////////////////////////////////////////////////////////////////////

void MprCmdService::cmdWatcherTimer(MprTimer *tp)
{
	MprCmd	*cp;
	int		count;

	mprLog(7, "cmdWatcherTimer %x\n", tp);

	//
	//	Call the real command watcher. This routine is supplied per O/S.
	//	MUST NOT call the watcher when locked as it will invoke callbacks.
	//
	cmdWatcher();

	//
	//	Get count of commands still running
	//
	lock();
	count = 0;
	cp = (MprCmd*) cmdList.getFirst();
	while (cp) {
		if (cp->getProcess() && !(cp->getFlags() & MPR_CMD_COMPLETE)) {
			mprLog(7, "cmdWatcherTimer pid %d still in cmd queue\n", cp->getProcess());
			count++;
		}
		cp = (MprCmd*) cmdList.getNext(cp);
	}

	mprLog(7, "cmdWatcher cmdList has %d, count %d\n", cmdList.getNumItems(), count);

	if (count > 0 && !mprGetMpr()->isExiting()) {
		mprLog(7, "cmdWatcher %x, returns %d, calling RESCHEDULE\n", tp, count);
		//
		//	Appears to be more commands to wait for completion.
		//
		tp->reschedule();

	} else {
		mprLog(7, "cmdWatcher disposing timer %x\n", tp);
		tp->dispose();
		timer = 0;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprCmdFiles //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprCmdFiles::MprCmdFiles()
{
	int		i;

	//
	//	Easier to see when file opens if we initialize with -2 rather than -1
	//
	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		fd[i] = -2;					// Server fd
		clientFd[i] = -2;
		name[i] = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

MprCmdFiles::~MprCmdFiles()
{
	//	Now done in MprCmd::resetFiles
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MprCmd ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Create a run object to run a command
// 

MprCmd::MprCmd()
{
	int		i;

	cwd = 0;
	data = 0;
	exitStatus = -1;
	reapIndex = -1;
	flags = 0;
	inUse = 1;

#if BLD_FEATURE_LOG
	log = new MprLogModule("cmd");
#endif
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

	cmdDoneProc = 0;
	process = 0;

#if VXWORKS
	handler = 0;
#if BLD_FEATURE_MULTITHREAD
	startCond = new MprCond();
	exitCond = new MprCond();
#endif
#endif

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		files.clientFd[i] = -1;
		files.fd[i] = -1;
	}
	
	mprGetMpr()->cmdService->insertCmd(this);
}

////////////////////////////////////////////////////////////////////////////////

MprCmd::~MprCmd()
{
	mprLog(8, log, "~MprCmd: process %d\n", process);

	//
	//	Must do remove first
	//
	mprGetMpr()->cmdService->removeCmd(this);

	reset();
	resetFiles();

#if BLD_FEATURE_LOG
	delete log;
#endif
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#if VXWORKS
	delete startCond;
	delete exitCond;
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////

int MprCmd::dispose()
{
	lock();
	mprAssert(inUse > 0);
	mprAssert(!(flags & MPR_CMD_DISPOSED));

	mprLog(8, log, "dispose: process %d\n", process);
	flags |= MPR_CMD_DISPOSED;

	if (--inUse == 0) {
		delete this;
		return 1;
	} else {
		unlock();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	The caller must be prepared that no callbacks will be called during the
//	command termination.
//

void MprCmd::reset()
{
	lock();

	if (process > 0) {
		stop();
	}

	mprFree(cwd);
	cwd = 0;
	data = 0;
	cmdDoneProc = 0;
	exitStatus = -1;

	if (process) {
#if WIN
		CloseHandle((HANDLE) process);
#endif
		process = 0;
	}

	//
	//	Must preserve the made flag if makeStdio has been called
	//
	flags &= MPR_CMD_STDIO_MADE;

	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprCmd::resetFiles()
{
	int		i, j, rc;

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		if (files.clientFd[i] >= 0) {
			close(files.clientFd[i]);
			files.clientFd[i] = -1;
		}
		if (files.fd[i] >= 0) {
			close(files.fd[i]);
			files.fd[i] = -1;
		}
		if (files.name[i]) {
			//
			//	Windows seems to be very slow in cleaning up the child's
			//	hold on the standard I/O file descriptors. Despite having
			//	waited for the child to exit and having received exit status,
			//	this unlink sometimes still gets a sharing violation. Ugh !!!
			//	We need to retry here (for up to 60 seconds). Under extreme 
			//	load -- this may fail to unlink the file.
			//
			for (j = 0; j < 1000; j++) {
				rc = unlink(files.name[i]);
				if (rc == 0) {
					break;
				}
				mprSleep(60);
			}
			if (j == 1000) {
				mprLog(0, "File busy, failed to unlink %s\n", files.name[i]);
			}
			mprFree(files.name[i]);
			files.name[i] = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	It would be nice if you could issue multiple start() commands on a single
//	object (serially), however it is very doubtful if this currently works.
//

int MprCmd::start(char *cmd, int userFlags)
{
	char	**argv;
	int		rc;

	mprMakeArgv(0, cmd, &argv, 0);
	rc = start(argv[0], argv, 0, 0, 0, userFlags);
	mprFree(argv);
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

void MprCmd::setExitStatus(int status)
{ 
	mprAssert(process > 0);

	mprLog(6, "setExitStatus: process %d, status %d\n", process, status);

	lock();
	exitStatus = status; 
	flags |= MPR_CMD_COMPLETE;

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Invoke the callback. CallerMutex is a mutex that is locked on entry.
//	Typically this is the service/factory list lock. We must not call the
//	callback while the caller's mutex is locked, but we must not release the
//	lock until we have incremented our inUse ref count to ensure this cmd is
//	not deleted prematurely.
//

void MprCmd::invokeCallback(MprMutex *callerMutex)
{
	lock();
	if (cmdDoneProc == 0 || (flags & MPR_CMD_DISPOSED)) {
		unlock();
		return;
	}

	//
	//	Prevent this object from being deleted by another thread while we are using it.
	//
	inUse++;
	unlock();

#if BLD_FEATURE_MULTITHREAD
	if (callerMutex) {
		callerMutex->unlock();
	}
#endif
	if (cmdDoneProc) {
		(cmdDoneProc)(this, data);
	}

	lock();
	//
	//	If the object was deleted while we were using it, then delete now.
	//
	if (--inUse == 0 && flags & MPR_CMD_DISPOSED) {
		delete this;
	} else {
		unlock();
	}
#if BLD_FEATURE_MULTITHREAD
	if (callerMutex) {
		callerMutex->lock();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

int MprCmd::getExitCode(int *status)
{
	lock();
	if (! (flags & MPR_CMD_COMPLETE)) {
		mprLog(5, log, "getExitCode: process %d\n", process);
		unlock();
		return MPR_ERR_NOT_READY;
	}
	if (status) {
		*status = exitStatus;
		mprLog(7, log, "getExitCode: process %d, code %d\n", process, *status);
	}
	unlock();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MprCmd::isRunning()
{
	return process > 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprCmd::setCwd(char *dir)
{
	mprFree(cwd);
	cwd = mprStrdup(dir);
}

////////////////////////////////////////////////////////////////////////////////

int MprCmd::getWriteFd() 
{ 
	return files.fd[MPR_CMD_OUT]; 
}

////////////////////////////////////////////////////////////////////////////////

int MprCmd::getReadFd() 
{
	return files.fd[MPR_CMD_IN]; 
}

////////////////////////////////////////////////////////////////////////////////

void MprCmd::closeWriteFd()
{
	lock();
	if (files.fd[MPR_CMD_OUT] >= 0) {
		close(files.fd[MPR_CMD_OUT]);
		files.fd[MPR_CMD_OUT] = -1;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

#else

void mprCmdDummy() {}

#endif // BLD_FEATURE_CGI_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
