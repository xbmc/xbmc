///
///	@file 	WIN/os.cpp
/// @brief 	Windows support for the Mbedthis Portable Runtime
/// @overview This file contains most of the Windows specific 
///		implementation required to host the MPR.
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

///////////////////////////// Forward Declarations /////////////////////////////

static char	*getHive(char *key, HKEY *root);

//////////////////////////////////// Code //////////////////////////////////////
//
//	Initialize the platform layer
// 

int Mpr::platformInitialize()
{
	WSADATA		wsaData;

	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		return -1;
	}

	//
	//	This crashes the WIN CRT
	//
	//	putenv("IFS=\t ");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Terminate the platform layer
// 

int Mpr::platformTerminate()
{
	WSACleanup();
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Start any required platform services
// 

int Mpr::platformStart(int startFlags)
{
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
 *	Add a search path to the set of directories search for when loading a module.
 *	The set of directories can be quoted and have spaces. For example:
 *
 *		"/dir/some dir" "/next" "andAnother"
 */

void mprSetModuleSearchPath(char *dirs)
{
	char	full[MPR_MAX_FNAME], native[MPR_MAX_FNAME];
	char	*oldPath, sep, *np, *tok, *newPath, *searchPath, *cp;
	int		len, modified;

	mprAssert(dirs && *dirs);

	oldPath = getenv("PATH");
	mprAssert(oldPath);

	if (oldPath == 0 || dirs == 0) {
		return;
	}

	/*
	 *	Prepare to tokenize the search path
	 */
	searchPath = mprStrdup(dirs);
	sep = strchr(searchPath, '"') ? '"' : ' ';
	for (cp = searchPath; *cp; cp++) {
		if (*cp == sep) {
			*cp = '\001';
		}
	}

	len = mprAllocSprintf(&newPath, -1, "PATH=%s", oldPath);
	mprAssert(len > 0);

	np = mprStrTok(searchPath, "\001\n", &tok);
	for (modified = 0; np; ) {
		mprGetFullPathName(full, sizeof(full), np);
		mprGetNativePathName(native, sizeof(native), full);

		//	TODO - this should really do a case insensitive scan
		if ((cp = strstr(oldPath, native)) != 0) {
			cp = &oldPath[strlen(native)];
			if (*cp == ';' || *cp == '\0') {
				np = mprStrTok(0, "\001\n", &tok);
				continue;
			}
		}

		len = mprReallocStrcat(&newPath, -1, len, ";", native, 0);
		mprAssert(len >= 0);
		np = mprStrTok(0, "\001\n", &tok);
		modified = 1;
	}
	if (modified) {
		mprLog(7, "Set %s\n", newPath);

		if (putenv(newPath) < 0) {
			mprAssert(0);
		}
	}

	mprFree(searchPath);
	mprFree(newPath);
}


////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DLL

int Mpr::loadDll(char *path, char *fnName, void *arg, void **handlePtr)
{
	MprDllEntryProc	fn;
	char			localPath[MPR_MAX_FNAME], dir[MPR_MAX_FNAME];
    void			*handle;
	char			*cp;
	int				rc;

	mprAssert(path && *path);
	mprAssert(fnName && *fnName);

	mprGetDirName(dir, sizeof(dir), path);
	mprSetModuleSearchPath(dir);

	mprStrcpy(localPath, sizeof(localPath), path);
	//	TODO - good to have a x-platform method for this.
	for (cp = localPath; *cp; cp++) {
		if (*cp == '/') {
			*cp = '\\';
		}
	}

    if ((handle = GetModuleHandle(mprGetBaseName(localPath))) == 0) {
		if ((handle = LoadLibraryEx(localPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH)) == 0) {
			char cwd[1024], *env;
			getcwd(cwd, sizeof(cwd) - 1);
			env = getenv("PATH");
			mprLog(0, "ERROR %d\n", GetLastError());
			mprLog(0, "Can't load %s\nReason: \"%d\"\n", path, mprGetOsError());
			mprLog(0, "CWD %s\n PATH %s\n", cwd, env);
			return MPR_ERR_CANT_OPEN;
		}
    }

	fn = (MprDllEntryProc) GetProcAddress((HINSTANCE) handle, fnName);
	if (fn == 0) {
		FreeLibrary((HINSTANCE) handle);
		mprLog(0, "Can't load %s\nReason: can't find function \"%s\"\n", 
			localPath, fnName);
		return MPR_ERR_NOT_FOUND;
	}
	mprLog(MPR_INFO, "Loading DLL %s\n", path);

	if ((rc = (fn)(arg)) < 0) {
		FreeLibrary((HINSTANCE) handle);
		mprError(MPR_L, MPR_LOG, "Initialization for %s failed.", path);
		return MPR_ERR_CANT_INITIALIZE;
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
	FreeLibrary((HINSTANCE) handle);
}

#endif	// BLD_FEATURE_DLL
////////////////////////////////////////////////////////////////////////////////
//
//	Write the given message to the native O/S log. We change the message.
// 

void Mpr::writeToOsLog(char *message, int flags)
{
	HKEY		hkey;
	void		*event;
	long		errorType;
	char		buf[MPR_MAX_STRING], msg[MPR_MAX_STRING];
	char		logName[MPR_MAX_STRING];
	char		*lines[9];
	char		*cp, *value;
	int			type;
	ulong		exists;
	static int	once = 0;

	mprStrcpy(buf, sizeof(buf), message);
	cp = &buf[strlen(buf) - 1];
	while (*cp == '\n' && cp > buf) {
		*cp-- = '\0';
	}

	if (flags & MPR_INFO) {
		type = EVENTLOG_INFORMATION_TYPE;
		mprSprintf(msg, MPR_MAX_STRING, "%s information: ", 
			Mpr::getAppName());

	} else if (flags == MPR_WARN) {
		type = EVENTLOG_WARNING_TYPE;
		mprSprintf(msg, MPR_MAX_STRING, "%s warning: ", Mpr::getAppName());

	} else {
		type = EVENTLOG_ERROR_TYPE;
		mprSprintf(msg, MPR_MAX_STRING, "%s error: %d", Mpr::getAppName(), 
			GetLastError());
	}

	lines[0] = msg;
	lines[1] = buf;
	lines[2] = lines[3] = lines[4] = lines[5] = 0;
	lines[6] = lines[7] = lines[8] = 0;

	if (once == 0) {
		//	Initialize the registry
		once = 1;
		mprSprintf(logName, sizeof(logName), 
			"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
			Mpr::getAppName());
		hkey = 0;

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, logName, 0, NULL, 0, 
				KEY_ALL_ACCESS, NULL, &hkey, &exists) == ERROR_SUCCESS) {

			value = "%SystemRoot%\\System32\\netmsg.dll";
			if (RegSetValueEx(hkey, "EventMessageFile", 0, REG_EXPAND_SZ, 
					(uchar*) value, strlen(value) + 1) != ERROR_SUCCESS) {
				RegCloseKey(hkey);
				return;
			}

			errorType = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
				EVENTLOG_INFORMATION_TYPE;
			if (RegSetValueEx(hkey, "TypesSupported", 0, REG_DWORD, 
					(uchar*) &errorType, sizeof(DWORD)) != ERROR_SUCCESS) {
				RegCloseKey(hkey);
				return;
			}
			RegCloseKey(hkey);
		}
	}

	event = RegisterEventSource(0, Mpr::getAppName());
	if (event) {
		//	
		//	3299 is the event number for the generic message in netmsg.dll.
		//	"%1 %2 %3 %4 %5 %6 %7 %8 %9" -- thanks Apache for the tip
		//
		ReportEvent(event, EVENTLOG_ERROR_TYPE, 0, 3299, NULL, 
			sizeof(lines) / sizeof(char*), 0, (LPCSTR*) lines, 0);
		DeregisterEventSource(event);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Kill another running MR instance
//

int Mpr::killMpr()
{
	HWND	hwnd;
	int		i;

	hwnd = FindWindow(getAppName(), getAppTitle());
	if (hwnd) {
		PostMessage(hwnd, WM_QUIT, 0, 0L);

		//
		//	Wait for up to ten seconds while winAppweb exits
		//
		for (i = 0; hwnd && i < 100; i++) {
			mprSleep(100);
			hwnd = FindWindow(getAppName(), getAppTitle());
		}
		if (hwnd = 0) {
			return 0;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprCmdService ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CGI_MODULE
//
//	MUST NOT BE invoked locked
//
void MprCmdService::cmdWatcher()
{
	MprCmd	*cp, *nextp;

	mprLog(6, "cmdWatcher:\n");

	//
	//	Reap status for all completed commands
	//
	lock();
again:
	cp = (MprCmd*) cmdList.getFirst();
	while (cp) {
		nextp = (MprCmd*) cmdList.getNext(cp);
		if (cp->getProcess() != 0) {
			if (cp->waitForChild(0) == 0) {
				//
				//	invokeCallback will temporarily release our lock and will
				//	re-acquire it before returning. It needs to release the
				//	lock while invoking the callback to prevent deadly
				//	embraces. WARNING: cp may be deleted on return.
				//
				cp->invokeCallback(mutex);
				goto again;
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
//	code to enable. Specifically the cgiHandler.cpp
//
int MprCmd::makeStdio()
{
	SECURITY_ATTRIBUTES	clientAtt, serverAtt, *att;
	HANDLE				fileHandle;
	char				pipeBuf[MPR_MAX_FNAME];
	int					openMode, pipeMode, i, pipeFd, fileFd;
	static int			tempSeed = 0;

	resetFiles();

	//
	//	The difference is server fds are not inherited by the child
	//
	memset(&clientAtt, 0, sizeof(clientAtt));
	clientAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	clientAtt.bInheritHandle = TRUE;

	memset(&serverAtt, 0, sizeof(serverAtt));
	serverAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	serverAtt.bInheritHandle = FALSE;

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {

		mprSprintf(pipeBuf, sizeof(pipeBuf), "\\\\.\\pipe\\mpr_%d_%d", 
			getpid(), tempSeed++);

		if (i == MPR_CMD_IN) {
			openMode = PIPE_ACCESS_OUTBOUND;
				/* | FILE_FLAG_OVERLAPPED; */
			pipeMode = 0;
		} else {
			openMode = PIPE_ACCESS_INBOUND;
			pipeMode = 0;
		}
		att = &clientAtt;

		//
		//	Create named pipe and read handle
		//
		fileHandle = CreateNamedPipe(pipeBuf, openMode, pipeMode, 1, 0, 
			65536, 1, att);
		pipeFd = (int) _open_osfhandle((long) fileHandle, 0);

		//
		//	Create write handle
		//
		att = &serverAtt;
		if (i == MPR_CMD_IN) {
			fileHandle = CreateFile(pipeBuf, GENERIC_READ, 0, att,
				OPEN_EXISTING, 0, 0);
		} else {
			fileHandle = CreateFile(pipeBuf, GENERIC_WRITE, 0, att,
				OPEN_EXISTING, 0, 0);
		}
		fileFd = (int) _open_osfhandle((long) fileHandle, 0);

		if (pipeFd < 0 || fileFd < 0) {
			mprError(MPR_L, MPR_LOG, "Can't create stdio pipes. Err %d",
				mprGetOsError());
			mprAssert(0);
			return -1;
		}
		files.fd[i] = fileFd;
		files.clientFd[i] = pipeFd;
	}
	flags |= MPR_CMD_STDIO_MADE;
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

int MprCmd::makeStdio()
{
	SECURITY_ATTRIBUTES	clientAtt, serverAtt, *att;
	HANDLE				fileHandle;
	char				path[MPR_MAX_FNAME];
	int					i, fdRead, fdWrite;

	memset(&clientAtt, 0, sizeof(clientAtt));
	clientAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	clientAtt.bInheritHandle = TRUE;

	memset(&serverAtt, 0, sizeof(serverAtt));
	serverAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	serverAtt.bInheritHandle = FALSE;

	for (i = 0; i < MPR_CMD_MAX_FD; i++) {

		mprMakeTempFileName(path, sizeof(path), 0);
		files.name[i] = mprStrdup(path);

		att = (i == MPR_CMD_IN) ? &clientAtt : &serverAtt;
		fileHandle = CreateFile(path, GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, att, 
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		mprAssert(fileHandle);
		fdWrite = (int) _open_osfhandle((long) fileHandle, 0);

		att = (i == MPR_CMD_OUT) ? &clientAtt : &serverAtt;
		fileHandle = CreateFile(path, GENERIC_READ, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, att, 
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		mprAssert(fileHandle);
		fdRead = (int) _open_osfhandle((long) fileHandle, 0);

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
	}
	flags |= MPR_CMD_STDIO_MADE;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the command to run (stdIn and stdOut are named from the client's 
//	perspective)
//

int MprCmd::start(char *program, char **argv, char **envp, 
	MprCmdProc completionFn, void *fnData, int userFlags)
{
	PROCESS_INFORMATION	procInfo;
	STARTUPINFO			startInfo;
	char				dirBuf[MPR_MAX_FNAME], *systemRoot;
	char				*envBuf, **ep, *cmdBuf, **ap, *destp, *cp, *dir, *key;
	char				progBuf[MPR_MAX_STRING], *localArgv[2], *saveArg0;
	int					argc, len, inheritFiles;

	mprAssert(program);
	mprAssert(argv);

	reset();

	flags |= (userFlags & MPR_CMD_USER_FLAGS);
	exitStatus = -1;

	mprStrcpy(progBuf, sizeof(progBuf), program);
	progBuf[sizeof(progBuf) - 1] = '\0';
	program = progBuf;

	//
	//	Sanitize the command line (program name only)
	//
	for (cp = program; *cp; cp++) {
		if (*cp == '/') {
			*cp = '\\';
		} else if (*cp == '\r' || *cp == '\n') {
			*cp = ' ';
		}
	}
	if (*program == '"') {
		if ((cp = strrchr(++program, '"')) != 0) {
			*cp = '\0';
		}
	}

	saveArg0 = argv[0];
	if (argv == 0) {
		argv = localArgv;
		argv[1] = 0;
	}
	argv[0] = program;

	//
	//	Determine the command line length and arg count
	//
	argc = 0;
	for (len = 0, ap = argv; *ap; ap++) {
		len += strlen(*ap) + 1 + 2;			// Space and possible quotes
		argc++;
	}
	cmdBuf = (char*) mprMalloc(len + 1);
	cmdBuf[len] = '\0';
	
	//
	//	Add quotes to all args that have spaces in them including "program"
	//
	destp = cmdBuf;
	for (ap = &argv[0]; *ap; ) {
		cp = *ap;
		if ((strchr(cp, ' ') != 0) && cp[0] != '\"') {
			*destp++ = '\"';
			strcpy(destp, cp);
			destp += strlen(cp);
			*destp++ = '\"';
		} else {
			strcpy(destp, cp);
			destp += strlen(cp);
		}
		if (*++ap) {
			*destp++ = ' ';
		}
	}
	*destp = '\0';
	mprAssert((int) strlen(destp) < (len - 1));
	mprAssert(cmdBuf[len] == '\0');
	argv[0] = saveArg0;

	envBuf = 0;
	if (envp) {
		for (len = 0, ep = envp; *ep; ep++) {
			len += strlen(*ep) + 1;
		}

		key = "SYSTEMROOT";
		systemRoot = getenv(key);
		if (systemRoot) {
			len += strlen(key) + 1 + strlen(systemRoot) + 1;
		}

		envBuf = (char*) mprMalloc(len + 2);		// Win requires two nulls
		destp = envBuf;
		for (ep = envp; *ep; ep++) {
			strcpy(destp, *ep);
			mprLog(6, log, "Set CGI variable: %s\n", destp);
			destp += strlen(*ep) + 1;
		}

		strcpy(destp, key);
		destp += strlen(key);
		*destp++ = '=';
		strcpy(destp, systemRoot);
		destp += strlen(systemRoot) + 1;

		*destp++ = '\0';
		*destp++ = '\0';						// WIN requires two nulls
	}
	
	memset(&startInfo, 0, sizeof(startInfo));
	startInfo.cb = sizeof(startInfo);

    startInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	if (flags & MPR_CMD_SHOW) {
		startInfo.wShowWindow = SW_SHOW;
	} else {
		startInfo.wShowWindow = SW_HIDE;
	}

	//
	//	CMD_OUT is stdin for the client. CMD_IN is stdout for the client
	//
	if (files.clientFd[MPR_CMD_OUT] > 0) {
		startInfo.hStdInput = 
			(HANDLE) _get_osfhandle(files.clientFd[MPR_CMD_OUT]);
	}
	if (files.clientFd[MPR_CMD_IN] > 0) {
		startInfo.hStdOutput = 
			(HANDLE)_get_osfhandle(files.clientFd[MPR_CMD_IN]);
	}
#if UNUSED
	if (files.clientFd[MPR_CMD_ERR] > 0) {
		startInfo.hStdError = 
			(HANDLE) _get_osfhandle(files.clientFd[MPR_CMD_ERR]);
	}
#endif

#if UNUSED
	SECURITY_ATTRIBUTES	secAtt;
	memset(&secAtt, 0, sizeof(secAtt));
	secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAtt.bInheritHandle = TRUE;
#endif

	if (userFlags & MPR_CMD_CHDIR) {
		if (cwd) {
			dir = cwd;
		} else {
			mprGetDirName(dirBuf, sizeof(dirBuf), argv[0]);
			dir = dirBuf;
		}
	} else {
		dir = 0;
	}

	inheritFiles = (flags & MPR_CMD_STDIO_MADE) ? 1 : 0;
	flags &= ~(MPR_CMD_STDIO_MADE);

	mprLog(5, log, "Running: %s\n", cmdBuf); 

	if (! CreateProcess(0, cmdBuf, 0, 0, inheritFiles, CREATE_NEW_CONSOLE,
			envBuf, dir, &startInfo, &procInfo)) {
		mprError(MPR_L, MPR_LOG, "Can't create process: %s, %d", 
			cmdBuf, mprGetOsError());
		return MPR_ERR_CANT_CREATE;
	}
	process = (int) procInfo.hProcess;

	//
	//	Wait for the child to initialize
	//
	WaitForInputIdle((HANDLE) process, 1000);

	if (procInfo.hThread != 0)  {
		CloseHandle(procInfo.hThread);
	}

	mprFree(cmdBuf);
	mprFree(envBuf);

	cmdDoneProc = completionFn;
	data = fnData;

	if (flags & MPR_CMD_DETACHED) {
		CloseHandle((HANDLE) process);
		process = 0;

	} else if (userFlags & MPR_CMD_WAIT) {
		waitForChild(INT_MAX);
		if (getExitCode() < 0) {
			return MPR_ERR_BAD_STATE;
		}
		return exitStatus;

	} else {
		mprGetMpr()->cmdService->startWatcher();
	}


	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Windows blocks a thread and waits for the child to exit. Return zero if
//	successfully waited on.
//

int MprCmd::waitForChild(int timeout)
{
	int		status;

	lock();
	if (process == 0) {
		unlock();
		return 0;
	}
	if (timeout) {
		//	
		//	FUTURE we could loop and call serviceEvents here. Must then
		//	increment inUse and call unlock() before calling serviceEvents
		//
		if (WaitForSingleObject((HANDLE) process, timeout) != WAIT_OBJECT_0) {
			mprLog(7, "waitForChild: WaitForSingleObject timeout\n");
			unlock();
			return MPR_ERR_TIMEOUT;
		}
	}
	if (GetExitCodeProcess((HANDLE) process, (ulong*) &status) == 0) {
		mprLog(7, "waitForChild: GetExitProcess error\n");
		unlock();
		return MPR_ERR_BUSY;
	}

	if (status == STILL_ACTIVE) {
		mprLog(7, "waitForChild: GetExitProcess status is still active\n");
		unlock();
		return MPR_ERR_BUSY;
	}

	//
	//	
	//
	CloseHandle((HANDLE) process);
	setExitStatus(status);
	process = 0;

	mprLog(7, log, "waitForChild: status %d\n", status);

	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the command.
//

void MprCmd::stop()
{
	mprLog(7, log, "stop\n");

	lock();
	//
	//	Ensure there are no callbacks from here on
	//
	cmdDoneProc = 0;

	if (process > 0) {
		TerminateProcess((HANDLE) process, 2);
		process = 0;
	}
	unlock();
}

#endif // BLD_FEATURE_CGI_MODULE
////////////////////////////////////////////////////////////////////////////////
extern "C" {

int mprGetRandomBytes(uchar *buf, int length, int block)
{
    HCRYPTPROV 		prov;
	int				rc;

	rc = 0;

    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, 
			CRYPT_VERIFYCONTEXT | 0x40)) {
		return -mprGetOsError();
    }
    if (!CryptGenRandom(prov, length, buf)) {
		mprError(MPR_L, MPR_USER, "Can't generate random bytes");
    	rc = -mprGetOsError();
    }
    CryptReleaseContext(prov, 0);
    return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read a registry value
// 

int mprReadRegistry(char *key, char *name, char **buf, int max)
{
	HKEY		top, h;
	char		*value;
	ulong		type, size;

	mprAssert(key && *key);
	mprAssert(buf);

	//
	//	Get the registry hive
	// 
	if ((key = getHive(key, &top)) == 0) {
		return MPR_ERR_CANT_ACCESS;
	}

	if (RegOpenKeyEx(top, key, 0, KEY_READ, &h) != ERROR_SUCCESS) {
		return MPR_ERR_CANT_ACCESS;
	}

	//
	//	Get the type
	// 
	if (RegQueryValueEx(h, name, 0, &type, 0, &size) != ERROR_SUCCESS) {
		RegCloseKey(h);
		return MPR_ERR_CANT_READ;
	}
	if (type != REG_SZ && type != REG_EXPAND_SZ) {
		RegCloseKey(h);
		return MPR_ERR_BAD_TYPE;
	}

	value = (char*) mprMalloc(size);
	if ((int) size > max) {
		RegCloseKey(h);
		return MPR_ERR_WONT_FIT;
	}
	if (RegQueryValueEx(h, name, 0, &type, (uchar*) value, &size) != 
			ERROR_SUCCESS) {
		delete value;
		RegCloseKey(h);
		return MPR_ERR_CANT_READ;
	}

    RegCloseKey(h);
	*buf = value;
	return 0;
}

} // extern "C"
////////////////////////////////////////////////////////////////////////////////
//
//	Determine the registry hive by the first portion of the path. Return 
//	a pointer to the rest of key path after the hive portion.
// 

static char *getHive(char *keyPath, HKEY *hive)
{
	char	key[MPR_MAX_STRING], *cp;
	int		len;

	mprAssert(keyPath && *keyPath);

	*hive = 0;

	mprStrcpy(key, sizeof(key), keyPath);
	key[sizeof(key) - 1] = '\0';

	if (cp = strchr(key, '\\')) {
		*cp++ = '\0';
	}
	if (cp == 0 || *cp == '\0') {
		return 0;
	}

	if (!mprStrCmpAnyCase(key, "HKEY_LOCAL_MACHINE")) {
		*hive = HKEY_LOCAL_MACHINE;
	} else if (!mprStrCmpAnyCase(key, "HKEY_CURRENT_USER")) {
		*hive = HKEY_CURRENT_USER;
	} else if (!mprStrCmpAnyCase(key, "HKEY_USERS")) {
		*hive = HKEY_USERS;
	} else if (!mprStrCmpAnyCase(key, "HKEY_CLASSES_ROOT")) {
		*hive = HKEY_CLASSES_ROOT;
	} else {
		mprError(MPR_L, MPR_LOG, "Bad hive key: %s", key);
	}

	if (*hive == 0) {
		return 0;
	}
	len = strlen(key) + 1;
	return keyPath + len;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
