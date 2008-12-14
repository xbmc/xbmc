///
///	@file 	UNIX/os.cpp
/// @brief 	Linux support for the Mbedthis Portable Runtime
///	@overview This file contains most of the UNIX specific implementation 
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

////////////////////////////// Forward Declarations ////////////////////////////

static pid_t	readPid();
static void		writePid();

#if BLD_FEATURE_CGI_MODULE
static void 	(*chainFunc)(int signo, siginfo_t *info, void *arg);
#endif

#if BLD_FEATURE_MULTITHREAD
static MprSpinLock cmdLock;
#endif

//////////////////////////////////// Code //////////////////////////////////////
//
//	Initialize the platform layer
//

int Mpr::platformInitialize()
{
#if UNUSED
	//
	//	Changing the runas user is currently done in http
	//	FUTURE: move into the MPR
	//
	if (geteuid() != 0) {
		mprError(MPR_L, MPR_USER, "Insufficient privilege");
		return -1;
	}
#endif
	
	umask(022);
	putenv("IFS=\t ");

#if FUTURE
	// 
	//	Open a syslog connection
	//
	openlog(mprGetMpr()->getAppName(), LOG_CONS || LOG_PERROR, LOG_LOCAL0);
#endif

#if BLD_FEATURE_MULTITHREAD
	mprSpinInit(&cmdLock);
#endif

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Terminate the platform layer
//

int Mpr::platformTerminate()
{
#if BLD_FEATURE_MULTITHREAD
	mprSpinDestroy(&cmdLock);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start any required platform services
//

int Mpr::platformStart(int startFlags)
{
	if (startFlags & MPR_KILLABLE) {
		writePid();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the platform services
//

int Mpr::platformStop()
{
	char	pidPath[MPR_MAX_FNAME];

	mprSprintf(pidPath, MPR_MAX_FNAME, "%s/.%s_pid.log", 
		getInstallDir(), getAppName());
	unlink(pidPath);

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

int Mpr::loadDll(char *path, char *fnName, void *arg, void **handlePtr)
{
	MprDllEntryProc	fn;
	void			*handle;
	int				rc;

	mprAssert(path && *path);
	mprAssert(fnName && *fnName);

	if ((handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL)) == 0) {
		mprError(MPR_L, MPR_LOG, "Can't load %s\nReason: \"%s\"", 
			path, dlerror());
		return MPR_ERR_CANT_OPEN;
	}

	if ((fn = (MprDllEntryProc) dlsym(handle, fnName)) == 0) {
		mprError(MPR_L, MPR_LOG, 
			"Can't load %s\nReason: can't find function \"%s\"", 
			path, fnName);
		dlclose(handle);
		return MPR_ERR_NOT_FOUND;
	}

	mprLog(MPR_INFO, "Loading DLL %s\n", path);

	if ((rc = (fn)(arg)) < 0) {
		dlclose(handle);
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
	mprAssert(handle);
	dlclose(handle);
}

#endif	// BLD_FEATURE_DLL
////////////////////////////////////////////////////////////////////////////////
//	
//	Write a message in the O/S native log (syslog in the case of LINUX)
//

void Mpr::writeToOsLog(char *message, int flags)
{
#if FUTURE
	Mpr		*mpr;

	mpr = mprGetMpr();

	//
	//	This bloats memory a lot
	//
	char	msg[MPR_MAX_FNAME];

	if (flags & MPR_INFO) {
		mprSprintf(msg, sizeof(msg), "%s information: ", mpr->getAppName());

	} else if (flags & MPR_WARN) {
		mprSprintf(msg, sizeof(msg), "%s warning: ", mpr->getAppName());

	} else {
		mprSprintf(msg, sizeof(msg), "%s error: ", mpr->getAppName());
	}
	syslog(flags, "%s: %s\n", msg, message);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Kill another running MR instance
//

int Mpr::killMpr()
{
	pid_t	pid;

	pid = readPid();
	if (pid < 0) {
		return MPR_ERR_NOT_FOUND;
	}

	mprLog(MPR_INFO, "Sending signal %d to process %d\n", SIGTERM, pid);

	if (kill(pid, SIGTERM) < 0) {
		if (errno == ESRCH) {
			mprLog(MPR_INFO, "Pid %d is not valid\n", pid);
		} else {
			mprLog(MPR_INFO, "Call to kill(%d) failed, %d\n", pid, errno);
		}
		return MPR_ERR_CANT_COMPLETE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get the pid for the current MR process
//

static pid_t readPid()
{
	Mpr		*mpr;
	char	pidPath[MPR_MAX_FNAME];
	pid_t	pid;
	int		fd;

	mpr = mprGetMpr();
	mprSprintf(pidPath, MPR_MAX_FNAME, "%s/.%s_pid.log", 
		mpr->getInstallDir(), mpr->getAppName());

	if ((fd = open(pidPath, O_RDONLY, 0666)) < 0) {
		mprLog(MPR_DEBUG, "Could not read a pid from %s\n", pidPath);
		return -1;
	}
	if (read(fd, &pid, sizeof(pid)) != sizeof(pid)) {
		mprLog(MPR_DEBUG, "Read from file %s failed\n", pidPath);
		close(fd);
		return -1;
	}
	close(fd);
	return pid;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write the pid for the current MR
// 

static void writePid()
{
	char	pidPath[MPR_MAX_FNAME];
	Mpr		*mpr;
	pid_t	pid;
	int		fd;

	mpr = mprGetMpr();
	mprSprintf(pidPath, MPR_MAX_FNAME, "%s/.%s_pid.log", 
		mpr->getInstallDir(), mpr->getAppName());

	if ((fd = open(pidPath, O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
		mprLog(MPR_INFO, "Could not create pid file %s\n", pidPath);
		return;
	}
	pid = getpid();
	if (write(fd, &pid, sizeof(pid)) != sizeof(pid)) {
		mprLog(MPR_WARN, "Write to file %s failed\n", pidPath);
	}
	close(fd);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprCmdService /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CGI_MODULE
//
//	Catch child death signals and chain to any previous signal handler.
//	We reap the command process remenants here. WARNING: this routine may be
//	called reentrantly from different threads. We use a spinlock to serialize.
//	Must be quick!! Must not use any routines that are not async-safe, ie.
//	must not call mprAlloc, malloc, mprLog or other such routines. Lock is
//	no protection against a signal coming recursively on a thread. Think about it.
//

static void cmdCompleted(int signo, siginfo_t *info, void *arg)
{
	int		pid, rc, status, code, saveErrno;

#if BLD_FEATURE_MULTITHREAD
	int once = 0;

	/*
	 *	Must prevent other threads from entering here. Note: we can be reentrant
	 *	on our own thread.
	 */
	mprSpinLock(&cmdLock);

	if (once++ > 0) {
		char *msg = "WARNING: Reentrant signal";
		write(0, msg, (int) strlen(msg));
	}
#endif

	saveErrno = errno;
	mprAssert(signo == SIGCHLD && info);

	/*
	 * 	Yet another warning. DONT USE non-async-thread safe routines here.
	 */
	if (info) {
		/*
		 *	Required for PPC on some Linux versions.
		 */
		code = info->si_code & 0xFFFF;
		if (code == CLD_EXITED || code == CLD_KILLED) {
			pid = info->si_pid;

			/*
			 * 	Loop waiting for any reaped children. NOTE: we may reap the child
			 * 	before the parent has awakened from the fork()
			 */
			do {
				errno = 0;
				rc = waitpid(pid, &status, 0);

				if (rc > 0) {
					status = WEXITSTATUS(status);
					mprGetMpr()->cmdService->processStatus(rc, status);
					pid = -1;
					/* Keep going to reap any other children */

				} else if (errno == EINTR) {
					rc = 1;
				}

			} while (rc > 0);
		}
	}

	if (chainFunc) {
		(*chainFunc)(signo, info, arg);
	}

	errno = saveErrno;

#if BLD_FEATURE_MULTITHREAD
	once--;
	mprSpinUnlock(&cmdLock);
#endif
}


////////////////////////////////////////////////////////////////////////////////
/*
 *	Called by the signal handler. Must be async-thread safe.
 */

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
//
//	Linuxthreads (uClibc) will create threads with a different thread group. This
//	means the new thread cannot call waitpid(). NPTL in glibc does not have this problem. 
//	The signal will be delivered to the thread that created the child.
//

void MprCmdService::initSignals()
{
	struct sigaction	act, old;

	memset(&act, 0, sizeof(act));

	act.sa_sigaction = cmdCompleted;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;

	if (sigaction(SIGCHLD, &act, &old) < 0) {
		mprError(MPR_L, MPR_USER, "Can't initialize signals");
	}
	chainFunc = old.sa_sigaction;
}

////////////////////////////////////////////////////////////////////////////////

void MprCmdService::cmdWatcher()
{
	MprCmd	*cp, *nextp;
	int		i, index, pid;

	mprLog(6, "cmdWatcher:\n");

	//
	//	NOTE: this lock is not for cmdCompleted in UNIX/os.cpp. Locks are 
	//	useless for signal handlers. The locks are for the child list and for
	//	re-entrant calls to this routine from startWatcher.
	//
	//
rescan:
	lock();

	cp = (MprCmd*) cmdList.getFirst();
	while (cp) {
		nextp = (MprCmd*) cmdList.getNext(cp);

		pid = cp->getProcess();

		//
		//	Pid will be set when the vfork() system call returns
		//
		if (pid) {
			/*
			 * 	Set the reap index if the child has been reaped
			 */
			i = 0;
			if (cp->reapIndex < 0) {
				for (i = 0; i < MPR_CMD_REAP_MAX && children[i].pid != 0; i++) {
					if (children[i].pid == cp->process) {
						cp->reapIndex = i;
					}
				}
			}

			index = cp->reapIndex;
			if (index >= 0) {
				mprAssert(pid == (int) cp->getProcess());
				if (pid == (int) children[index].pid) {
					cp->setExitStatus(children[index].exitStatus);

					//
					//	Set to one so we dont disturb the zero end of list marker
					//	PID == 1 is never valid for user programs. NOTE: this is racing
					//	with the cmdCompleted signal handler, but because pid was non-zero
					//	above, we can safely set to 1.
					//
					children[index].pid = 1;

					//
					//	invokeCallback will temporarily release our lock and will
					//	re-acquire it before returning. It needs to release the
					//	lock while invoking the callback to prevent deadly
					//	embraces. WARNING: cp may be deleted on return.
					//
					cp->invokeCallback(mutex);
					
					//
					//	Must start the scan incase another thread changed the list
					//
					unlock();
					goto rescan;
				}
			}
		}
		cp = nextp;
	}

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MprCmd ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if MPR_USE_PIPES
//
//	This code has been debugged but requires changes to other portions of the
//	code to enable. Specifically, the cgiHandler.cpp.
//
int MprCmd::makeStdio()
{
	int		fds[2], i;

	resetFiles();

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		if (pipe(fds) < 0) {
			mprError(MPR_L, MPR_LOG, "Can't create stdio pipes. Err %d",
				mprGetOsError());
			mprAssert(0);
			return -1;
		}
		if (i == MPR_CMD_OUT) {
			files.clientFd[i] = fds[0];			// read fd
			files.fd[i] = fds[1];				// write fd
		} else {
			files.clientFd[i] = fds[1];			// write fd
			files.fd[i] = fds[0];				// read fd
		}
		mprLog(7, log, "makeStdio: pipe handles[%d] read %d, write %d\n",
			i, fds[0], fds[1]);
	}

	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

int MprCmd::makeStdio()
{
	char	path[MPR_MAX_FNAME];
	int		i, fdRead, fdWrite;

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {
		mprMakeTempFileName(path, sizeof(path), 0);
		files.name[i] = mprStrdup(path);

		fdWrite = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0600);
		fdRead = open(path, O_RDONLY);
		if (fdRead < 0 || fdWrite < 0) {
			mprError(MPR_L, MPR_LOG, "Can't create stdio files\n");
			return -1;
		}
		if (i == MPR_CMD_IN) {
			files.fd[i] = fdRead;
			files.clientFd[i] = fdWrite;
		} else {
			files.fd[i] = fdWrite;
			files.clientFd[i] = fdRead;
		}
		mprLog(7, log, "makeStdio: file %s handles[%d] read %d, write %d\n", files.name[i], i, fdRead, fdWrite);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the command to run (stdIn and stdOut are named from the client's perspective)
//

int MprCmd::start(char *program, char **argv, char **envp, MprCmdProc fn, 
	void *fnData, int userFlags)
{
	char		dir[MPR_MAX_FNAME];
	int			pid, i, err;

	mprAssert(program != 0);
	mprAssert(argv != 0);
	mprLog(4, log, "start: %s\n", program);

	reset();

	flags |= (userFlags & MPR_CMD_USER_FLAGS);

	for (i = 0; argv[i]; i++) {
		mprLog(6, log, "    arg[%d]: %s\n", i, argv[i]);
	}
	if (envp) {
		for (i = 0; envp[i]; i++) {
			mprLog(6, log, "    envp[%d]: %s\n", i, envp[i]);
		}
	}

	if (access(program, X_OK) < 0) {
		mprLog(5, log, "start: can't access %s, errno %d\n", 
			program, mprGetOsError());
		return MPR_ERR_CANT_ACCESS;
	}

	data = fnData;
	cmdDoneProc = fn;
	reapIndex = -1;

	//
	//	Create the child
	//
	pid = vfork();

	if (pid < 0) {
		mprLog(0, log, "Can't fork a new process to run %s\n", program);
		return MPR_ERR_CANT_INITIALIZE;

	} else if (pid == 0) {
		//
		//	Child
		//
		umask(022);
		if (flags & MPR_CMD_NEW_SESSION) {
			setsid();
		}
		if (flags & MPR_CMD_CHDIR) {
			if (cwd) {
				chdir(cwd);
			} else {
				mprGetDirName(dir, sizeof(dir), program);
				chdir(dir);
			}
		}

		//	
		//	FUTURE -- could chroot as a security feature (perhaps cgi-bin)
		//
		if (files.clientFd[MPR_CMD_OUT] >= 0) {
			dup2(files.clientFd[MPR_CMD_OUT], 0);	// Client stdin
		} else {
			close(0);
		}
		if (files.clientFd[MPR_CMD_IN] >= 0) {
			dup2(files.clientFd[MPR_CMD_IN], 1);	// Client stdout
			dup2(files.clientFd[MPR_CMD_IN], 2);	// Client stderr
		} else {
			close(1);
			close(2);
		}

		//
		//	FUTURE -- need to get a better max file limit than this
		//
		for (i = 3; i < 128; i++) {
			close(i);
		}

		if (envp) {
			execve(program, argv, envp);
		} else {
			//
			//	Do this rather than user execv to avoid errors in valgrind
			//
			char	*env[2];
	
			env[0] = "_appweb=1";
			env[1] = 0;
			execve(program, argv, (char**) &env);
		}

		err = errno;
		getcwd(dir, sizeof(dir));
		mprStaticPrintf("Can't exec %s, err %d, cwd %s\n", program, err, dir);
		mprAssert(0);
		exit(-(MPR_ERR_CANT_INITIALIZE));

	} else {

		//
		//	Close the client handles
		//
		for (i = 0; i < MPR_CMD_MAX_FD; i++) {
			if (files.clientFd[i] >= 0) {
				close(files.clientFd[i]);
				files.clientFd[i] = -1;
			}
		}

		data = fnData;
		cmdDoneProc = fn;
		process = (ulong) pid;

		if (flags & MPR_CMD_DETACHED) {
			process = 0;
			return 0;

		} else if (flags & MPR_CMD_WAIT) {
			waitForChild(INT_MAX);
			if (getExitCode() < 0) {
				return MPR_ERR_BAD_STATE;
			}
			return exitStatus;

		} else {
			mprGetMpr()->cmdService->startWatcher();
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the command
//

void MprCmd::stop()
{
	int		rc;

	mprLog(7, log, "stop: process %d\n", process);

	lock();
	cmdDoneProc = 0;
	if (process > 0) {
		rc = kill(process, SIGTERM);
		mprLog(5, "MprCmd::stop kill %d returns %d errno %d\n", process, rc, errno);
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
	int		fd, sofar, rc;

	fd = open((block) ? "/dev/random" : "/dev/urandom", O_RDONLY, 0666);
	if (fd < 0) {
		mprError(MPR_L, MPR_USER, "Can't open /dev/random");
		return MPR_ERR_CANT_OPEN;
	}

	sofar = 0;
	do {
		rc = read(fd, &buf[sofar], length);
		if (rc < 0) {
			mprAssert(0);
			return MPR_ERR_CANT_READ;
		}
		length -= rc;
		sofar += rc;
	} while (length > 0);
	close(fd);
	return 0;
}

} // extern "C"
////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
