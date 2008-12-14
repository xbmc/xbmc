///
///	@file 	VXWORKS/os.cpp
/// @brief 	VxWorks support for the Mbedthis Portable Runtime
///	@overview This file contains most of the VxWorks specific implementation 
///		required to host the MPR.
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

/////////////////////////////////// Locals /////////////////////////////////////

typedef int		(*MprCmdTaskFn)(int argc, char **argv);

////////////////////////////// Forward Declarations ////////////////////////////

#if BLD_FEATURE_CGI_MODULE
static void	 	cmdCompleted(WIND_TCB *tcb);

static void 	newTaskWrapper(char *program, MprCmdTaskFn *entry, 
					int argc, char **argv, char **envp, 
					char *inPath, char *outPath, char *pipePath, 
					MprCond *startCond, char *cwd);

#endif

//////////////////////////////////// Code //////////////////////////////////////
//
//	Initialize the platform layer
//

int Mpr::platformInitialize()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Terminate the platform layer
//

int Mpr::platformTerminate()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start any required platform services
//

int Mpr::platformStart(int startFlags)
{
#if BLD_FEATURE_CGI_MODULE
	//
	//	Initialize the pipe driver	
	//
	pipeDrv();
	taskDeleteHookAdd((FUNCPTR) cmdCompleted);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the platform services
//

int Mpr::platformStop()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
 * 	Not required
 */

void mprSetModuleSearchPath(char *dirs)
{
}


////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DLL
//
//	Load a shared object specified by path. fnName is the entryPoint.
//
int Mpr::loadDll(char *path, char *fnName, void *arg, void **handlePtr)
{
	MprDllEntryProc	fn;
	SYM_TYPE		symType;
	void			*handle;
	char			entryPoint[MPR_MAX_FNAME];
	int				rc, fd, flags;

	mprAssert(path && *path);
	mprAssert(fnName && *fnName);

	if (moduleFindByName(path) != 0) {
		//	Already loaded
		return 0;
	}

#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86 || BLD_HOST_CPU_ARCH == MPR_CPU_IX64
	mprSprintf(entryPoint, sizeof(entryPoint), "_%s", fnName);
#else
	mprStrcpy(entryPoint, sizeof(entryPoint), fnName);
#endif

	if ((fd = open(path, O_RDONLY, 0664)) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't open %s", path);
		return MPR_ERR_CANT_OPEN;
	}

	flags = LOAD_GLOBAL_SYMBOLS;
#if BLD_DEBUG
	flags |= LOAD_LOCAL_SYMBOLS;
#endif
	if ((handle = loadModule(fd, flags)) == 0) {
		mprError(MPR_L, MPR_LOG, "Can't load %s", path);
		close(fd);
		return MPR_ERR_CANT_INITIALIZE;
	}
	close(fd);

	fn = 0;
	if (symFindByName(sysSymTbl, entryPoint, (char**) &fn, &symType) == -1) {
		mprError(MPR_L, MPR_LOG, "Can't find symbol %s when loading %s",
			fnName, path);
		return MPR_ERR_NOT_FOUND;
	}
	mprLog(MPR_INFO, "Loading DLL %s\n", path);

	if ((rc = (fn)(arg)) < 0) {
		mprError(MPR_L, MPR_LOG, "Initialization for %s failed.", path);
		return rc;
	}

	if (handlePtr) {
		*handlePtr = handle;
	}
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

void Mpr::unloadDll(void *handle)
{
	unldByModuleId((MODULE_ID) handle, 0);
}

#endif	// BLD_FEATURE_DLL
////////////////////////////////////////////////////////////////////////////////
//	
//	Write a message in the O/S native log (Not implemented)
//

void Mpr::writeToOsLog(char *message, int flags)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//	Kill another running MR instance
//

int Mpr::killMpr()
{
	int		taskId;

	if ((taskId = taskNameToId(BLD_PRODUCT)) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't find task %s", BLD_PRODUCT);
		return MPR_ERR_NOT_FOUND;
	}

	if (taskDelete(taskId) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't kill task %s, id %d", BLD_PRODUCT,
			taskId);
		return MPR_ERR_NOT_FOUND;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprCmdService /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CGI_MODULE

static void cmdCompleted(WIND_TCB *tcb)
{
#if MOB
	mprGetMpr()->cmdService->processStatus(tcb->xxx, tcb->exitCode);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MprCmdService::processStatus(ulong pid, int status)
{
	int		i;

	/*
	 * 	Don't use malloc, mprLog or any non-async-thread-safe routine
	 */
	for (i = 0; i < MPR_CMD_REAP_MAX; i++) {
		if (children[i].pid <= 1) {
			children[i].pid = pid;
			children[i].exitStatus = status;
			break;
		}
	}
	mprAssert(i != MPR_CMD_REAP_MAX);
}

////////////////////////////////////////////////////////////////////////////////

void MprCmdService::cmdWatcher()
{
	MprCmd	*cp, *nextp;
	int		i;

	mprLog(6, "cmdWatcher:\n");

	//
	//	NOTE: this lock is not for cmdCompleted in UNIX/os.cpp. Locks are 
	//	useless for signal handlers. The locks are for the child list and for
	//	re-entrant calls to this routine from startWatcher.
	//
	lock();
	for (i = 0; i < MPR_CMD_REAP_MAX; i++) {
		if (completedCmds[i] == 0) {
			break;
		}

		cp = (MprCmd*) cmdList.getFirst();
		while (cp) {
			nextp = (MprCmd*) cmdList.getNext(cp);
			if (cp->getProcess() == completedCmds[i]) {
				cp->setExitStatus(exitStatus[i]);
				completedCmds[i] = 0;

				//
				//	invokeCallback will temporarily release our lock and will
				//	re-acquire it before returning. It needs to release the
				//	lock while invoking the callback to prevent deadly
				//	embraces. WARNING: cp may be deleted on return.
				//
				cp->invokeCallback(mutex);
				break;
			}
			cp = nextp;
		}
		if (cp == 0) {
			completedCmds[i] = 0;
		}
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MprCmd ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Make files for standard I/O. We use files not pipes. Just make the file
//	names here. start() will create the files.
//

int MprCmd::makeStdio()
{
	char	path[MPR_MAX_FNAME];
	int		i;

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		mprMakeTempFileName(path, sizeof(path), 0);
		files.name[i] = mprStrdup(path);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the command to run (stdIn and stdOut are named from the client's 
//	perspective)
//

int MprCmd::start(char *program, char **argv, char **envp, MprCmdProc fn, 
	void *fnData, int userFlags)
{
	MprCmdTaskFn	entryFn;
	SYM_TYPE		symType;
	char			dir[MPR_MAX_FNAME], pipePath[MPR_MAX_FNAME];
	char			*entryPoint, *childCwd;
	int				argc, i, pri;

	mprLog(4, log, "start: %s\n", program);
	mprAssert(program != 0);
	mprAssert(argv != 0);

	reset();
	flags |= (userFlags & MPR_CMD_USER_FLAGS);

	files.fd[MPR_CMD_OUT] = open(files.name[MPR_CMD_OUT], 
		O_CREAT | O_TRUNC | O_WRONLY, 0600);
	if (files.fd[MPR_CMD_OUT] < 0) {
		mprError(MPR_L, MPR_LOG, "Can't create output ...");
		return MPR_ERR_CANT_CREATE;
	}

	//
	//	Make a pipe just so we can signal child death
	//
	mprSprintf(pipePath, MPR_MAX_FNAME, "/pipe/%s%d", BLD_PRODUCT, 
		taskIdSelf());
	if (pipeDevCreate(pipePath, 10, 1) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't create pipes to run %s", program);
		return MPR_ERR_CANT_OPEN;
	}
	waitFd = open(pipePath, O_RDONLY, 0644);

	mprLog(7, log, "start: wait pipe %d\n", waitFd);
	mprAssert(waitFd >= 0);

	for (argc = 0; argv[argc]; argc++) {
		mprLog(6, log, "    arg[%d]: %s\n", argc, argv[argc]);
	}

	entryPoint = 0;
	if (envp) {
		for (i = 0; envp[i]; i++) {
			if (strncmp(envp[i], "entryPoint=", 11) == 0) {
				entryPoint = mprStrdup(envp[i]);
			}
			mprLog(6, log, "    envp[%d]: %s\n", i, envp[i]);
		}
	}
	if (entryPoint == 0) {
		entryPoint = mprStrdup(mprGetBaseName(program));
	}

	if (access(program, X_OK) < 0) {
		mprError(MPR_L, MPR_LOG, "start: can't access %s, errno %d", 
			program, mprGetOsError());
		return MPR_ERR_CANT_ACCESS;
	}

	if (symFindByName(sysSymTbl, entryPoint, (char**) &entryFn, &symType) < 0) {
#if MPR_BLD_FEATURE_DLL
		if (mprGetMpr()->loadDll(program, mprGetBaseName(program), 0, 0) < 0) {
			mprError(MPR_L, MPR_LOG, "start: can't load DLL %s, errno %d",
				program, mprGetOsError());
			return MPR_ERR_CANT_READ;
		}
#endif
		if (symFindByName(sysSymTbl, entryPoint, (char**) &entryFn, 
				&symType) < 0) {
			mprError(MPR_L, MPR_LOG, "start: can't find symbol %s, errno %d", 
				entryPoint, mprGetOsError());
			return MPR_ERR_CANT_ACCESS;
		}
	}

	taskPriorityGet(taskIdSelf(), &pri);

	childCwd = 0;
	if (flags & MPR_CMD_CHDIR) {
		if (cwd) {
			childCwd = mprStrdup(cwd);
		} else {
			mprGetDirName(dir, sizeof(dir), program);
			childCwd = mprStrdup(dir);
		}
	}

	//
	//	Pass the server output file to become the client stdin.
	//
	process = taskSpawn(entryPoint, pri, 0, MPR_DEFAULT_STACK, 
		(FUNCPTR) newTaskWrapper, 
		(int) program, (int) entryFn, (int) argc, (int) argv, (int) envp, 
		(int) files.name[MPR_CMD_OUT], (int) files.name[MPR_CMD_IN], 
		(int) pipePath, (int) startCond, (int) childCwd);

	if (process < 0) {
		mprError(MPR_L, MPR_LOG, "start: can't create task %s, errno %d",
			entryPoint, mprGetOsError());
		mprFree(entryPoint);
		return MPR_ERR_CANT_CREATE;
	}

	mprLog(7, log, "start: child taskId %d\n", process);
	mprFree(entryPoint);
	mprFree(childCwd);

	if (startCond->waitForCond(MPR_CMD_WATCHER_TIMEOUT) < 0) {
		mprError(MPR_L, MPR_LOG, "start: child %s did not initialize, errno %d",
			program, mprGetOsError());
		return MPR_ERR_CANT_CREATE;
	}

	//
	//	Now that the child task is started, we can read from our input file
	//	that is the CGI task's output.
	//
	files.fd[MPR_CMD_IN] = open(files.name[MPR_CMD_IN], O_RDONLY, 0666);
	if (files.fd[MPR_CMD_IN] < 0) {
		mprError(MPR_L, MPR_LOG, "start: can't open CGI output %s, errno %d",
			files.name[MPR_CMD_IN], mprGetOsError());
		return MPR_ERR_CANT_CREATE;
	}

	data = fnData;
	cmdDoneProc = fn;

	if (flags & MPR_CMD_DETACHED) {
		process = 0;
		return 0;

	} else if (flags & MPR_CMD_WAIT) {
		waitForChild(INT_MAX);
		if (getExitCode() < 0) {
			return MPR_ERR_BAD_STATE;
		}
		return exitStatus;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Executed by the child process
//

static void newTaskWrapper(char *program, MprCmdTaskFn *entry, int argc, 
	char **argv, char **envp, char *inPath, char *outPath, char *pipePath, 
	MprCond *startCond, char *cwd)
{
	char	**ep;
	int		inFd, outFd, pipeFd;
	int		id;

	//
	//	Open standard I/O files
	//
	pipeFd = open(pipePath, O_WRONLY, 0666);
	if (pipeFd < 0) {
		exit(255);
	}

	outFd = creat(outPath, O_WRONLY | O_CREAT);
	inFd = open(inPath, O_RDONLY, 0666);

	if (inFd < 0 || outFd < 0) {
		close(pipeFd);
		exit(255);
	}

	id = taskIdSelf();
	ioTaskStdSet(id, 0, inFd);
	ioTaskStdSet(id, 1, outFd);

	//
	//	Now that we have opened the stdin and stdout, wakeup our parent.
	//
	startCond->signalCond();

	//
	//	Create the environment
	//
	if (envPrivateCreate(id, -1) < 0) {
		exit(254);
	}
	for (ep = envp; ep && *ep; ep++) {
		putenv(*ep);
	}

	//
	//	Set current directory if required
	//
	if (cwd) {
		chdir(cwd);
	}

	//
	//	Call the user's CGI entry point
	//
	(*entry)(argc, argv);

	//
	//	Signal our death by closing the wait pipe
	//
	close(pipeFd);

	//
	//	Cleanup
	//
	envPrivateDestroy(id);
	close(inFd);
	close(outFd);

	exit(0);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Stop the command.
//

void MprCmd::stop()
{
	mprLog(7, log, "stop: process %d\n", process);

	lock();
	cmdDoneProc = 0;
	if (process > 0) {
		taskDelete(process);
		process = 0;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	NOTE: we ignore the timeout and always wait forever
//

int MprCmd::waitForChild(int timeout)
{
	lock();
	//
	//	Prevent this object from being deleted by another thread while we 
	//	are using it.
	//
	inUse++;

	if (! (flags & MPR_CMD_COMPLETE)) {
		mprGetMpr()->cmdService->startWatcher();
	}
	unlock();

	while (! (flags & MPR_CMD_COMPLETE)) {
		mprGetMpr()->serviceEvents(1, MPR_CMD_WATCHER_NAP);
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
	return 0;
}

#endif // BLD_FEATURE_CGI_MODULE
////////////////////////////////////////////////////////////////////////////////
#if BLD_DEBUG
//
//	Useful in tracking down file handle leaks
//

void mprNextFds(char *msg)
{
	int i, fds[4];

	mprLog(0, msg);
	for (i = 0; i < 4; i++) {
		fds[i] = open("mob.txt", O_CREAT | O_TRUNC, 0666);
		mprLog("Next Fds %d\n", fds[i]);
	}
	for (i = 0; i < 4; i++) {
		close(fds[i]);
	}
}
#endif
////////////////////////////////////////////////////////////////////////////////
extern "C" {

int mprGetRandomBytes(uchar *buf, int length, int block)
{
	int		i;

	for (i = 0; i < length; i++) {
		buf[i] = (char) (time(0) >> i);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if _WRS_VXWORKS_MAJOR < 6
//
//	access() for VxWorks
//

STATUS access(const char *path, int mode)
{
	struct stat	sbuf;

	return stat((char*) path, &sbuf);
}

#endif
////////////////////////////////////////////////////////////////////////////////

} // extern "C"

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
