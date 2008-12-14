///
///	@file 	log.cpp
/// @brief 	Diagnostic error, trace and logging facility
/// @overview This module provides a flexible trace and error 
///		logging mechanism for development, debugging and run-time 
///		fault-finding.
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
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

#if BLD_FEATURE_LOG
//////////////////////////////////// Locals ////////////////////////////////////

static MprLogService	*defaultLog;		// Default log service

///////////////////////////////////// Code /////////////////////////////////////
extern "C" {
//
//	Universal place to come on an error
//

void mprBreakpoint(char *file, int line, char *msg)
{
	if (! mprGetDebugMode()) {
		return;
	}
#if BLD_DEBUG
#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86 || BLD_HOST_CPU_ARCH == MPR_CPU_IX64
#if WIN
	__asm { int 3 }
#else
	// abort();
	asm("int $03");
	// __asm__ __volatile__ ("int $03"); 
#endif
#endif	// IX86
#endif	// BLD_DEBUG
}

}	// extern "C"
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprLogService ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprLogListener::MprLogListener()
{
}

////////////////////////////////////////////////////////////////////////////////

MprLogListener::~MprLogListener()
{
}

////////////////////////////////////////////////////////////////////////////////

int MprLogListener::setLogSpec(char *path, int size)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogListener::shuttingDown()
{
}

////////////////////////////////////////////////////////////////////////////////

int MprLogListener::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogListener::stop()
{
}

////////////////////////////////////////////////////////////////////////////////

void MprLogListener::logEvent(char *module, int flags, int level, char *thread,
	char *msg)
{
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprLogToFile /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprLogToFile::MprLogToFile()
{
	logFd = -1;
	logFileName = 0;
	timeStamps = 0;
	rotationCount = 0;
	timer = 0;
	maxSize = MPR_MAX_LOG_SIZE;
}

////////////////////////////////////////////////////////////////////////////////

MprLogToFile::~MprLogToFile()
{
	mprFree(logFileName);
	stop();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Wrapper function

static void writeTimeStampWrapper(void *data, MprTimer *tp)
{
	MprLogToFile	*lp = (MprLogToFile*) data;

	lp->writeTimeStamp();
	tp->reschedule();
}

////////////////////////////////////////////////////////////////////////////////

int MprLogToFile::setLogSpec(char *path, int size)
{
	struct stat	sbuf;
	char		bak[MPR_MAX_FNAME];

	if (logFd >= 0) {
		close(logFd);
	}

	maxSize = size * 1024 * 1024;			// Convert to bytes
	logFileName = mprStrdup(path);

	if (strcmp(logFileName, "stdout") == 0) {
		logFd = MPR_STDOUT;

	} else {
		if (stat(logFileName, &sbuf) >= 0) {
			mprSprintf(bak, sizeof(bak), "%s.old", logFileName);
			if (access(bak, R_OK) == 0) {
				unlink(bak);
			}
			rename(logFileName, bak);
#if VXWORKS
			//
			//	VxWorks FTP file system does not succeed with renames
			//
			if (access(logFileName, R_OK) == 0) {
				unlink(logFileName);
			}
			
#endif
		}
		logFd = open(logFileName, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0666);
		if (logFd < 0) {
			return MPR_ERR_CANT_OPEN;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MprLogToFile::start()
{
	logConfig();
	timer = new MprTimer(MPR_TIMEOUT_LOG_STAMP, writeTimeStampWrapper, 
		(void*) this);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToFile::shuttingDown()
{
	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToFile::stop()
{
	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
	if (logFd >= 0 && (logFd != MPR_STDOUT)) {
		mprFprintf(logFd, "\n- End -\n");
		close(logFd);
	}
	logFd = -1;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToFile::logEvent(char *module, int flags, int level, char *thread, 
	char *msg)
{
	char		buf[MPR_MAX_LOG_STRING];

	if (mprGetMpr() == 0) {
		mprStrcpy(buf, sizeof(buf), msg);

	} else if (timeStamps) {
#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD
#if	BLD_FEATURE_HIRES_TIME
		int64 elapsed = mprGetElapsedTime();
		mprSprintf(buf, sizeof(buf), "%,14Ld %10s:%d %s  %s", elapsed, module, 
			level, thread, msg);
#else
		static int last = 0;
		int now = mprGetTime(0);
		int elapsed = now - last;
		mprSprintf(buf, sizeof(buf), "%,14d %10s:%d %s  %s", elapsed, module, 
			level, thread, msg);
		last = now;
#endif
#endif // CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD

	} else if (! (flags & MPR_RAW)) {
		mprSprintf(buf, sizeof(buf), "%10s:%d %4s  %s", module, level, thread, 
			msg);

	} else {
		//	Raw output
		mprStrcpy(buf, sizeof(buf), msg);
	}

	if (logFd < 0) {
		if (level <= 1) {
			//
			//	Always output fatal and error messages
			//
			mprFprintf(MPR_STDERR, buf);
		}
		return;
	}

	//	OPT -- could get length above
	mprAssert(logFd >= 0);
	write(logFd, buf, strlen(buf));

	if (logFd != MPR_STDOUT) {
		struct stat	sbuf;
		if (maxSize <= 0 || fstat(logFd, &sbuf) < 0) {
			return;
		}
		//
		//	Rotate logs when full
		//
		if (sbuf.st_mode & S_IFREG && (unsigned) sbuf.st_size > maxSize) {
			rotate();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToFile::rotate()
{
	char bak[MPR_MAX_FNAME];

	mprSprintf(bak, sizeof(bak), "%s.old", logFileName);
	unlink(bak);

	mprFprintf(logFd, "Log size reached limit. Rotating\n");
	close(logFd);
	if (rename(logFileName, bak) != 0) {
		unlink(logFileName);
	}
	logFd = open(logFileName, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
	rotationCount++;
	logConfig();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Output critical configuration information
//

void MprLogToFile::logConfig()
{
	Mpr			*mpr;
	time_t		now;
	char		timeText[80];

	mpr = mprGetMpr();

	if (mpr == 0) {
		return;
	}
	now = time(0);
	mprCtime(&now, timeText, sizeof(timeText));

	mprLog(MPR_CONFIG, "Configuration for %s\n", mpr->getAppTitle());
	mprLog(MPR_CONFIG, "--------------------------------------------\n");
	mprLog(MPR_CONFIG, "Host:               %s\n", mpr->getHostName());
	mprLog(MPR_CONFIG, "CPU:                %s\n", mpr->getCpu());
	mprLog(MPR_CONFIG, "OS:                 %s\n", mpr->getOs());
	mprLog(MPR_CONFIG, "Distribution:       %s %s\n", BLD_HOST_DIST,
		BLD_HOST_DIST_VER);
	mprLog(MPR_CONFIG, "OS:                 %s\n", mpr->getOs());
	mprLog(MPR_CONFIG, "Version:            %s.%d\n", mpr->getVersion(), 
		mpr->getBuildNumber());
	mprLog(MPR_CONFIG, "BuildType:          %s\n", mpr->getBuildType());
	mprLog(MPR_CONFIG, "Started at:         %s", timeText);
	mprLog(MPR_CONFIG, "Log rotation count: %d\n", rotationCount);
	mprLog(MPR_CONFIG, "--------------------------------------------\n");

#if BLD_DEBUG
#if BLD_FEATURE_HIRES_TIME
	int64 elapsed = mprGetElapsedTime();
	mprSleep(250);
	elapsed = mprGetElapsedTime();
	mprLog(4, "1 second is   ~%,16Ld cycles\n", elapsed * 4);
#else
	int elapsed = mprGetTime(0);
	mprSleep(250);
	elapsed = mprGetTime(0) - elapsed;
	mprLog(4, "1 second is   ~%,8d cycles\n", elapsed * 4);
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Output a time stamp to the log file. Scheduled in mprOpenLog()
//	Write a timestamp whenever the time of the trace changes by > 5 secs
//

void MprLogToFile::writeTimeStamp()
{
	time_t		now;
	char		timeText[80];
	char		*cp;

	now = time(0);
	mprCtime(&now, timeText, sizeof(timeText));
	if ((cp = strchr(timeText, '\n')) != 0) {
		*cp = '\0';
	}
	mprLog(MPR_CONFIG, "[%s]\n", timeText);
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToFile::enableTimeStamps(bool on)
{
	timeStamps = on;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprLogToWindow ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprLogToWindow::MprLogToWindow()
{
}

////////////////////////////////////////////////////////////////////////////////

MprLogToWindow::~MprLogToWindow()
{
}

////////////////////////////////////////////////////////////////////////////////

void MprLogToWindow::logEvent(char *module, int flags, int level, char *thread,
	char *msg)
{
#if WIN
	Mpr		*mpr;

	mpr = mprGetMpr();
	if (!mpr->getHeadless() && (flags & MPR_USER)) {
		MessageBoxEx(NULL, msg, mpr->getAppTitle(), MB_OK, 0);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprLogService ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Open the log service
//

MprLogService::MprLogService()
{
	defaultLevel = 0;
	defaultLog = this;
	defaultModule = 0;
	logging = 0;
	moduleSpecs = 0;
	logSpec = 0;
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a log 
//

MprLogService::~MprLogService()
{
	MprLogListener	*lp, *nextLp;
	MprLogModule	*mp, *nextMp;

	//
	//	Destroy any remaining module structures and delete each object entry
	//
	mp = (MprLogModule*) moduleList.getFirst();
	while (mp) {
		nextMp = (MprLogModule*) moduleList.getNext(mp);
		delete mp;
		mp = nextMp;
	}

	lp = (MprLogListener*) listeners.getFirst();
	while (lp) {
		nextLp = (MprLogListener*) listeners.getNext(lp);
		listeners.remove(lp);
		lp = nextLp;
	}
	if (logSpec) {
		mprFree(logSpec);
	}

#if BLD_FEATURE_MULTITHREAD
	if (mutex) {
		delete mutex;
		mutex = 0;
	}
#endif
	defaultLog = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Open a log file. 
//
//	Format: filename:level.size
//

int MprLogService::setLogSpec(char *fileSpec)
{
	MprLogModule	*mp;
	MprLogListener	*lp;
	char			namBuf[MPR_MAX_FNAME];
	char			*spec, *cp, *sizeStr;
	int				maxSize;

	mprAssert(!logging);
#if BLD_FEATURE_MULTITHREAD
	if (mutex == 0) {
		mutex = new MprMutex();
	}
#endif
	lock();

	logging = 1;
	if (fileSpec == NULL || *fileSpec == '\0') {
		fileSpec = "trace.txt";
	}

	logSpec = mprStrdup(fileSpec);
	spec = mprStrdup(fileSpec);
	for (cp = spec; *cp; cp++) {
		if (*cp == ':' && isdigit(cp[1])) {
			break;
		}
	}

	maxSize = MPR_MAX_LOG_SIZE;
	if (*cp) {
		*cp++ = '\0';
		moduleSpecs = strchr(cp, ',');

		sizeStr = strchr(cp, '.');
		if (sizeStr != 0) {
			*sizeStr++ = '\0';
			maxSize = atoi(sizeStr);					// Size in MB
		}

		//
		//	Set all modules to the default trace level and then examine
		//	the modules spec to override specified modules
		//
		defaultLevel = atoi(cp);
		if (defaultLevel < 0) {
			defaultLevel = 0;
		}

		mp = (MprLogModule*) moduleList.getFirst();
		while (mp) {
			mp->setLevel(defaultLevel);
			mp = (MprLogModule*) moduleList.getNext(mp);
		}

		if (moduleSpecs) {
			moduleSpecs++;
			mp = (MprLogModule*) moduleList.getFirst();
			while (mp) {
				mprSprintf(namBuf, sizeof(namBuf), "%s:", mp->getName());
				if ((cp = strstr(moduleSpecs, namBuf)) != 0) {
					if ((cp = strchr(cp, ':')) != 0) {
						mp->setLevel(atoi(++cp));
					}
				}
				mp = (MprLogModule*) moduleList.getNext(mp);
			}
		}
	}
	if (spec != 0 && *spec != '\0') {
		lp = (MprLogListener*) listeners.getFirst();
		while (lp) {
			if (lp->setLogSpec(spec, maxSize) < 0) {
				mprFree(spec);
				unlock();
				return MPR_ERR_CANT_OPEN;
			}
			lp = (MprLogListener*) listeners.getNext(lp);
		}
	}
	mprFree(spec);
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::start()
{
	MprLogListener		*lp;

	lp = (MprLogListener*) listeners.getFirst();
	while (lp) {
		lp->start();
		lp = (MprLogListener*) listeners.getNext(lp);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::stop()
{
	MprLogListener		*lp;

	logging = 0;
	lp = (MprLogListener*) listeners.getFirst();
	while (lp) {
		lp->stop();
		lp = (MprLogListener*) listeners.getNext(lp);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::shuttingDown()
{
	MprLogListener		*lp;

	lp = (MprLogListener*) listeners.getFirst();
	while (lp) {
		lp->shuttingDown();
		lp = (MprLogListener*) listeners.getNext(lp);
	}
#if BLD_FEATURE_MULTITHREAD
	//
	//	Delete now so that we can have a clean exit if shutting down
	//
	if (mutex) {
		mutex->lock();
		delete mutex;
		mutex = 0;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MprLogService::isLogging()
{
	return logging;
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::addListener(MprLogListener *lp)
{
	lock();
	listeners.insert(lp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::removeListener(MprLogListener *lp)
{
	lock();
	listeners.remove(lp);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprLogService::setDefaultLevel(int l)
{
	MprLogModule	*mp;
	
	defaultLevel = l;
	if (defaultLevel < 0) {
		defaultLevel = 0;
	}

	mp = (MprLogModule*) moduleList.getFirst();
	while (mp) {
		mp->setLevel(defaultLevel);
		mp = (MprLogModule*) moduleList.getNext(mp);
	}
	mprLog(2, "Set log level for all modules to %d\n", defaultLevel);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a trace module
//

void MprLogService::insertModule(MprLogModule *module)
{
	lock();
	if (defaultModule == 0) {
		defaultModule = module;
	}
	if (module->getLevel() < 0) {
		module->setLevel(defaultLevel);
	}
	moduleList.insert(module);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a trace module
//

void MprLogService::removeModule(MprLogModule *module)
{
	lock();
	moduleList.remove(module);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Log an error. We must not allocate memory here. Use only local buffers.
//

void MprLogService::error(char *file, int line, int flags, char *fmt, 
	va_list args)
{
	static int	recurseGate = 0;
	Mpr			*mpr;
	char		msg[MPR_MAX_LOG_STRING], buf[MPR_MAX_LOG_STRING];

	mpr = mprGetMpr();

	//
	//	Errors post close
	//
#if BLD_FEATURE_MULTITHREAD
	if (mutex == 0) {
		return;
	}
#endif

	lock();
	if (recurseGate > 0) {
		unlock();
		return;
	}
	recurseGate++;

	mprVsprintf(msg, sizeof(msg), fmt, args);

	if (flags & MPR_TRAP) {
		mprSprintf(buf, sizeof(buf), "Assertion %s, failed (%s:%d)\n",
			msg, file, line);
		output(defaultModule, flags, MPR_ERROR, buf);
		// 
		//	Break to the debugger
		//
		breakpoint(file, line);

	} else if (flags & MPR_LOG) {
		//
		//	Write to both the O/S log and to the trace log
		//
		mprSprintf(buf, sizeof(buf), "Error: %s\n", msg);
		output(defaultModule, flags, MPR_ERROR, buf);
		if (mpr) {
			mpr->writeToOsLog(buf, flags | MPR_INFO);
		}

	} else if (flags & MPR_USER) {
		mprSprintf(buf, sizeof(buf), "Error: %s\n", msg);
		output(defaultModule, flags, MPR_ERROR, buf);
		if (mpr) {
			mpr->writeToOsLog(buf, flags | MPR_WARN);
		}

	} else if (flags & MPR_ALERT) {
		// FUTURE -- TBD 

	} else {
		mprAssert(0);
	}

	recurseGate--;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Core tracing function. We must not allocate memory here. Use only local 
//	buffers.
//

void MprLogService::traceCore(int level, int flags, MprLogModule *mod, 
	char *fmt, va_list args)
{
	char	buf[MPR_MAX_LOG_STRING];
	int		len;

	if (mod == 0) {
		mod = defaultModule;
	}

	//
	//	Test the level here first up to quickly eliminate verbose trace levels
	//
	if ((level & MPR_LOG_MASK) <= mod->getLevel()) {
		int max = sizeof(buf) - 12;
		len = mprVsprintf(buf, max, fmt, args);
		if (len >= (max - 1)) {
			mprStrcpy(&buf[len], 12, " TRUNCATED\n");
		}
		output(mod, flags, level, buf);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Do the actual output. Don't allocate memory here.
//

void MprLogService::output(MprLogModule *module, int flags, int level, 
	char *msg)
{
	MprLogListener	*lp;
	Mpr				*mpr;
	char			*threadName, *moduleName;

	mpr = mprGetMpr();

	lock();
	threadName = "";
#if BLD_FEATURE_MULTITHREAD
	if (mpr && mpr->threadService) {
		MprThread *tp = mpr->threadService->getCurrentThread();
		if (tp) {
			threadName = tp->getName();
		} 
	}
#endif

	moduleName = module ? module->getName() : (char*) "default";

	lp = (MprLogListener*) listeners.getFirst();
	while (lp) {
		lp->logEvent(moduleName, flags, level, threadName, msg);
		lp = (MprLogListener*) listeners.getNext(lp);
	}

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	All TRAP errors come through here
//

void MprLogService::breakpoint(char *file, int line)
{
	mprBreakpoint(file, line, 0);
}

////////////////////////////////////////////////////////////////////////////////

char *MprLogService::getModuleSpecs()
{
	return moduleSpecs;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprLogModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//

MprLogModule::MprLogModule(char *s)
{
	innerMprLogModule(s);
}

////////////////////////////////////////////////////////////////////////////////

void MprLogModule::innerMprLogModule(char *s)
{
	MprLogModule	*def;
	Mpr				*mpr;
	char			namBuf[MPR_MAX_FNAME];
	char			*moduleSpecs, *cp;

	name = mprStrdup(s);
	enabled = 1;
	level = -1;

	mpr = mprGetMpr();
	moduleSpecs = mpr->logService->getModuleSpecs();
	if (moduleSpecs) {
		mprSprintf(namBuf, sizeof(namBuf), "%s:", name);
		if ((cp = strstr(moduleSpecs, namBuf)) != 0) {
			if ((cp = strchr(cp, ':')) != 0) {
				level = atoi(++cp);
			}
		}
	}

	if (level < 0) {
		def = mpr->logService->getDefaultModule();
		if (def) {
			level = def->getLevel();
		}
	}
	mpr->logService->insertModule(this);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//

MprLogModule::~MprLogModule()
{
	mprGetMpr()->logService->removeModule(this);
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Simple trace output for debugging
//

void mprLog(char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	if (defaultLog) {
		defaultLog->traceCore(0, MPR_TRACE | MPR_RAW, 0, fmt, args);
	}
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Trace output to the default module
//

void mprLog(int level, char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	if (defaultLog) {
		defaultLog->traceCore(level, MPR_TRACE, 0, fmt, args);
	}
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Detailed trace output routine
//

void mprLog(int level, MprLogModule *module, char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	if (defaultLog) {
		defaultLog->traceCore(level, MPR_TRACE, module, fmt, args);
	}
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Detailed trace output routine
//

void mprLog(int level, int flags, MprLogModule *module, char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	if (defaultLog) {
		defaultLog->traceCore(level, flags, module, fmt, args);
	}
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
#else // BLD_FEATURE_LOG
extern "C" {
void mprBreakpoint(char *file, int line, char *msg) {}
} // extern "C"
#endif // BLD_FEATURE_LOG

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// C API ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Log an error message
//

extern "C" {
void mprError(char *file, int line, int flags, char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
#if BLD_FEATURE_LOG
	if (defaultLog) {
		defaultLog->error(file, line, flags, fmt, args);
	}
#else
	char	buf[MPR_MAX_LOG_STRING];
	mprVsprintf(buf, sizeof(buf), fmt, args);
	mprFprintf(MPR_STDERR, "%s\n", buf);
#endif
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
//
//	If BLD_FEATURE_LOG is not defined, we still need a stub for C code.
//

#if !BLD_FEATURE_LOG
void	mprLog(int level, char *fmt, ...) {};
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
