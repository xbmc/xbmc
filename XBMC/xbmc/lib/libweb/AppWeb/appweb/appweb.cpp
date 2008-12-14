///
///	@file 	appweb.cpp
/// @brief 	Appweb main program
///	@overview The appweb main program. It can run as a foreground 
///		(console) app or in the background as a daemon / service.
//
/////////////////////////////////// Copyright //////////////////////////////////
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

#include	"appweb.h"

//////////////////////////////////// Locals ////////////////////////////////////

#define APPWEB_SERVICE_NAME		BLD_COMPANY "-" BLD_PRODUCT
#define APPWEB_SERVICE_DISPLAY 	BLD_NAME

#if BLD_FEATURE_ROMFS
//
//	Change the name from defaultRomFiles to whatever romName you give to
//	httpComp when compiling your rom pages.
//
extern MaRomInode	defaultRomFiles[];
#endif

static int			autoScan;			// Scan for a free port to listen on
static int			background;			// Run as a daemon / system service
static char			*cmdSpec = "Aa:bcd:Df:gi:kl:mr:st:uvw:";
static char			*configFile;		// Configure via a config file
static char 		*docRoot;			// Location of the DocumentRoot
static MprFileSystem *fileSystem;		// File system object
static char			*ipAddr;			// IP address to listen on
static int			isService;			// Running as a service
static Mpr			*mp;				// Global MPR object
static char			*program;			// Program name
static MaServer		*server;			// Default server
static char 		*serverRoot;		// Location of the ServerRoot
static char			*writeFile;			// Save config file to named file

#if BLD_FEATURE_ROMFS
static MaRomFileSystem *romFileSystem;	// ROM based file system
#endif
#if BLD_FEATURE_LOG
static MprLogToFile	*logger;			// Error logging object
#if WIN
static MprLogToWindow *dialog;			// Error log to popup windows
#endif
#endif

#if WIN
static HINSTANCE	appInst;			// Current application instance 
static HWND			appHwnd;			// Application window handle 
static HWND			otherHwnd;			// Existing instance window handle 
static HANDLE		serviceWaitEvent;	// Service event to block on 
static int			serviceOp;			// Service operation
static char			*serviceCmdLine;	// Command line when run as a service
static HMENU		subMenu;			// As the name says
static int			taskBarIcon = 0;	// Icon in the task bar
static int			trayIcon = 1;		// Icon in the tray
static HMENU		trayMenu;			// As the name says
static HANDLE		threadHandle;		// Handle for the service thread
static MprTimer		*trayTimer;			// Timer to display the tray icon

#if BLD_FEATURE_RUN_AS_SERVICE
static MprWinService *winService;		// Global Windows Service object
#endif

//
//	Windows message defines
//
#define APPWEB_TRAY_MESSAGE		WM_USER+30
#define APPWEB_SOCKET_MESSAGE	WM_USER+32
#define APPWEB_ICON				"appweb.ico"
#define APPWEB_TRAY_ID			0x100
#endif

#define MAX_PORT_TRIES	50

#if FUTURE
char *okEnv[] =
{
	// variable name starts with 
	"HTTP_",
	"SSL_",

	// variable name is 
	"AUTH_TYPE=",
	"CONTENT_LENGTH=",
	"CONTENT_TYPE=",
	"DATE_GMT=",
	"DATE_LOCAL=",
	"DOCUMENT_NAME=",
	"DOCUMENT_PATH_INFO=",
	"DOCUMENT_ROOT=",
	"DOCUMENT_URI=",
	"FILEPATH_INFO=",
	"GATEWAY_INTERFACE=",
	"HTTPS=",
	"LAST_MODIFIED=",
	"PATH_INFO=",
	"PATH_TRANSLATED=",
	"QUERY_STRING=",
	"QUERY_STRING_UNESCAPED=",
	"REMOTE_ADDR=",
	"REMOTE_HOST=",
	"REMOTE_IDENT=",
	"REMOTE_PORT=",
	"REMOTE_USER=",
	"REDIRECT_QUERY_STRING=",
	"REDIRECT_STATUS=",
	"REDIRECT_URL=",
	"REQUEST_METHOD=",
	"REQUEST_URI=",
	"SCRIPT_FILENAME=",
	"SCRIPT_NAME=",
	"SCRIPT_URI=",
	"SCRIPT_URL=",
	"SERVER_ADMIN=",
	"SERVER_NAME=",
	"SERVER_ADDR=",
	"SERVER_PORT=",
	"SERVER_PROTOCOL=",
	"SERVER_SOFTWARE=",
	"UNIQUE_ID=",
	"USER_NAME=",
	"TZ=",
	NULL
};
#endif
////////////////////////////// Forward Declarations ////////////////////////////

static int 		configureViaApi();
static void		eventLoop();
static int 		findFreePort(char *ipAddr, int base);
static int	 	locateServerRoot(char *serverRoot);
static void		memoryFailure(int askSize, int totalHeapMem, int limit);
static void		printVersion();
static void		printUsage(char *program);
static int		realMain(MprCmdLine *cmdLine);
static int		securityChecks(char *program);
static void 	setupFileSystem();
static void 	setLogging(char *logSpec);
static int 		setupServer(MaHttp *http, int poolThreads);

#if WIN
static void		closeTrayIcon();
static int 		getBrowserPath(char **path, int max);
static int		findInstance();
static int		initWindow();
static void 	mapPathDelim(char *s);
static long		msgProc(HWND hwnd, uint msg, uint wp, long lp);
static int		openTrayIcon();
static void 	runBrowser(char *page);
static int		trayEvent(HWND hwnd, WPARAM wp, LPARAM lp);
static void		trayIconProc(void *arg, MprTimer *tp);
static int 		windowsInit();

#if BLD_FEATURE_RUN_AS_SERVICE
static void		svcThread(void *data);
static void WINAPI svcMainEntry(ulong argc, char **argv);
static int 		windowsServiceOps();
#endif // BLD_FEATURE_RUN_AS_SERVICE
#endif // WIN

#if BLD_HOST_UNIX || VXWORKS
static void 	catchSignal(int signo, siginfo_t *info, void *arg);
static void 	initSignals();
#endif // BLD_HOST_UNIX

#if BLD_FEATURE_CONFIG_PARSE
static int 		configureViaFile();
#endif

//////////////////////////////////// Code //////////////////////////////////////
#if WIN
#if BLD_FEATURE_RUN_AS_SERVICE

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *args, int junk2)
{
	appInst = inst;
	winService = new MprWinService(APPWEB_SERVICE_NAME);

	//
	//	Only talk to the windows service dispatcher if running as a service.
	//	This will block if we are a service and are being started by the
	//	service control manager. While blocked, the svcMain will be called
	//	which becomes the effective main program.
	//
	if (strstr(args, "-b") == 0 ||
			winService->startDispatcher(svcMainEntry) < 0) {
		MprCmdLine	*cmdLine = new MprCmdLine(args, cmdSpec);

		if (realMain(cmdLine) < 0) {
			delete cmdLine;
			return FALSE;
		}
		delete cmdLine;
		delete winService;
		mprMemClose();
		return 0;

	} else {
		delete winService;
		mprMemClose();
		return FALSE;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Secondary entry point when started by the service control manager. Remember 
//	that the main program thread is blocked in the startDispatcher called from
//	winMain and in fact will it will be used on callbacks in WinService.
// 

static void WINAPI svcMainEntry(ulong argc, char **argv)
{
	MprCmdLine		*cmdLine;
	char			keyPath[80], *argBuf, *cp;
	int				threadId;

	//
	//	Read the command line from the windows registry
	//
	argBuf = 0;
	mprSprintf(keyPath, sizeof(keyPath),
		"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\%s", 
		APPWEB_SERVICE_NAME);
	mprReadRegistry(keyPath, "ImagePath", &argBuf, MPR_MAX_STRING);

	//
	//	The program name may be in quotes and may have spaces in it
	//
	if (argBuf[0] == '\"') {
		if ((cp = strchr(&argBuf[1], '\"')) == 0) {
			cp = argBuf;
		} else {
			cp++;
		}
	} else {
		cp = argBuf;
	}
	while (isspace(*cp) || *cp == '\v') {
		cp++;
	}
	cmdLine = new MprCmdLine(cp, cmdSpec);

	serviceWaitEvent = CreateEvent(0, TRUE, FALSE, 0);
	threadHandle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) svcThread, 
		(void*) cmdLine, 0, (ulong*) &threadId);
	if (threadHandle == 0) {
		//	Should never happen, but try to keep going anyway.
		realMain(cmdLine);
	}
	WaitForSingleObject(serviceWaitEvent, INFINITE);
	CloseHandle(serviceWaitEvent);

	delete cmdLine;
	mprFree(argBuf);
}

////////////////////////////////////////////////////////////////////////////////

static void svcThread(void *data)
{
	MprCmdLine	*cmdLine;
	int			rc;

	cmdLine = (MprCmdLine*) data;

	if (winService->registerService(threadHandle, serviceWaitEvent) < 0) {
		//	Should never happen, but try to keep going anyway.
		rc = realMain(cmdLine);
		ExitThread(rc);
		return;
	}
	//
	//	Call the real main
	//
	isService++;
	winService->updateStatus(SERVICE_RUNNING, 0);
	rc = realMain(cmdLine);
	winService->updateStatus(SERVICE_STOPPED, rc);
	ExitThread(rc);
}

////////////////////////////////////////////////////////////////////////////////
#else 	// !BLD_FEATURE_RUN_AS_SERVICE

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *args, int junk2)
{
	MprCmdLine	*cmdLine = new MprCmdLine(args, cmdSpec);

	if (realMain(cmdLine) < 0) {
		delete cmdLine;
		return FALSE;
	}
	delete cmdLine;
	mprMemClose();
	return 0;
}
#endif 	// !BLD_FEATURE_RUN_AS_SERVICE
#endif	// WIN

////////////////////////////////////////////////////////////////////////////////
#if BLD_HOST_UNIX
//
//	UNIX main program
//
int main(int argc, char *argv[])
{
	MprCmdLine 	cmdLine(argc, argv, cmdSpec);
	int			rc;

	rc = realMain(&cmdLine);
	mprMemClose();
	return 0;
}

#endif	// BLD_HOST_UNIX
////////////////////////////////////////////////////////////////////////////////
#if VXWORKS
//
//	UNIX main program
//
int appwebMain(int argc, char *argv[])
{
	MprCmdLine 	cmdLine("appweb -r . -f appweb.conf", cmdSpec);

	return realMain(&cmdLine);
}

#endif	// VXWORKS
////////////////////////////////////////////////////////////////////////////////
//
//	For service and command users alike, this is the real main.
//

static int realMain(MprCmdLine *cmdLine)
{
	MaHttp		*http;
	char        portNumBuf[MPR_MAX_IP_PORT];
	char		*argp, *logSpec;
	int			c, errflg, kill, poolThreads, outputVersion;

	mprSetMemHandler(memoryFailure);
	mprCreateMemHeap(0, 16 * 1024, MAXINT);
	program = mprGetBaseName(cmdLine->getArgv()[0]);

	poolThreads = -1;
	kill = errflg = 0;
	logSpec = 0;
	outputVersion = 0;
	autoScan = 1;
	background = 0;

	serverRoot = 0;
	docRoot = "web";

#if !WIN && !VXWORKS
	if (getuid()) {
		ipAddr = mprStrdup("4000");

	} else {
		mprSprintf(portNumBuf, sizeof(portNumBuf), "%d", 
			MA_SERVER_DEFAULT_PORT_NUM);
		ipAddr = mprStrdup(portNumBuf);
	}
#else
	mprSprintf(portNumBuf, sizeof(portNumBuf), "%d", 
		MA_SERVER_DEFAULT_PORT_NUM);
	    
	ipAddr = mprStrdup(portNumBuf);
#endif

	while ((c = cmdLine->next(&argp)) != EOF) {
		switch(c) {
		case 'A':
			autoScan = 0;
			break;

		case 'a':
			mprFree(ipAddr);
			ipAddr = mprStrdup(argp);
			break;
			
		case 'b':
			background++;
			break;

		case 'c':
			/* Ignored */
			break;

		case 'D':
			mprSetDebugMode(1);
			break;

		case 'd':
			docRoot = argp;
			break;

		case 'f':
#if BLD_FEATURE_CONFIG_PARSE
			configFile = argp;
			autoScan = 0;
#else
			errflg++;
#endif
			break;

		case 'k':
			kill++;
			logSpec = 0;
			break;

		case 'l':
			logSpec = (*argp) ? argp : 0;
			break;

		case 'm':
			mprRequestMemStats(1);
			break;

		case 'r':
			serverRoot = argp;
			break;
		
		case 't':
			poolThreads = atoi(argp);
			break;

			//
			//	FUTURE -- just for test. Will be removed
			//
		case 'w':
			writeFile = argp;
			break;

		case 'v':
			outputVersion++;
			break;
		
#if WIN && BLD_FEATURE_RUN_AS_SERVICE
		case 'i':
			serviceOp = MPR_INSTALL_SERVICE;
			if (strcmp(argp, "none") == 0) {
				serviceCmdLine = "";
			} else if (strcmp(argp, "default") == 0) {
				serviceCmdLine = "-b -c -f " BLD_PRODUCT ".conf";
			} else {
				serviceCmdLine = argp;
			}
			break;

		case 'g':
			serviceOp = MPR_GO_SERVICE;
			break;

		case 's':
			serviceOp = MPR_STOP_SERVICE;
			break;

		case 'u':
			serviceOp = MPR_UNINSTALL_SERVICE;
			break;

#endif
		default:
			errflg++;
			break;
		}
	}

	if (errflg) {
		printUsage(program);
		return MPR_ERR_BAD_SYNTAX;
	}	

	mp = new Mpr(program);
	mp->setAppName(BLD_PRODUCT);
	mp->setAppTitle(BLD_NAME);
	mp->setHeadless(isService || background);

#if BLD_HOST_UNIX || VXWORKS
	initSignals();
#endif

	if (kill) {
		mp->killMpr();
		delete mp;
		exit(0);
	}

	if (outputVersion) {
		printVersion();
		delete mp;
		exit(0);
	}

	//
	//	Create the top level HTTP service and default HTTP server
	//
	http = new MaHttp();
	server = new MaServer(http, "default");
	setupFileSystem();
	
	setLogging(logSpec);

	//
	//	Confirm the location of the server root
	//
	if (locateServerRoot(serverRoot) < 0) {
		mprError(MPR_L, MPR_USER, "Can't start server, exiting.");
		exit(2);
	}

	if (securityChecks(cmdLine->getArgv()[0]) < 0) {
		exit(3);
	}

#if WIN
#if BLD_FEATURE_RUN_AS_SERVICE
	if (serviceOp) {
		windowsServiceOps();
		delete mp;
		return 0;
	}
#endif
	if (windowsInit() < 0) {
		delete mp;
		return MPR_ERR_CANT_INITIALIZE;
	}
#endif

	//
	//	Start the MPR. This starts Timer, Socket and Pool services
	//
	if (mp->start(MPR_KILLABLE) < 0) {
		mprError(MPR_L, MPR_USER, "Can't start MPR for %s", mp->getAppTitle());
		delete mp;
		return MPR_ERR_CANT_INITIALIZE;
	}

	//
	//	Load the statically linked modules
	//
	maLoadStaticModules();

	if (setupServer(http, poolThreads) < 0) {
		mprError(MPR_L, MPR_USER, "Can't configure the server, exiting.");
		exit(6);
	}

#if BLD_FEATURE_CONFIG_SAVE
	if (writeFile) {
		server->saveConfig(writeFile);
		mprLog(0, "Configuration saved to %s\nExiting ...\n", writeFile);
		exit(0);
	}
#endif

	if (http->start() < 0) {
		mprError(MPR_L, MPR_USER, "Can't start server, exiting.");
		exit(7); 

	} else {
#if LINUX && BLD_FEATURE_RUN_AS_SERVICE
		if (background && mp->makeDaemon(1) < 0) {
			mprError(MPR_L, MPR_USER, "Could not run in the background");
		} else 
#endif
		{
#if BLD_FEATURE_MULTITHREAD
			mprLog(MPR_CONFIG, 
				"HTTP services are ready with %d pool threads\n",
				http->getLimits()->maxThreads);
#else
			mprLog(MPR_CONFIG, 
				"HTTP services are ready (single-threaded).\n");
#endif
		}
		mp->setHeadless(1);

		eventLoop();

		mprLog(MPR_WARN, "Stopping HTTP services.\n");
		http->stop();
	}

#if WIN
	if (trayIcon > 0) {
		closeTrayIcon();
	}
	if (trayTimer) {
		trayTimer->stop(MPR_TIMEOUT_STOP);
		trayTimer->dispose();
		trayTimer = 0;
	}
#endif

	mprLog(MPR_WARN, "Stopping MPR services.\n");
	mp->stop(0);
	maUnloadStaticModules();
	delete server;
	delete http;
	delete mp;

	mprFree(ipAddr);

#if BLD_FEATURE_ROMFS
	delete romFileSystem;
#endif

#if BLD_FEATURE_LOG
	mprLog(MPR_WARN, "Closing log.\n");
	if (logger) {
		delete logger;
	}
#if WIN
	if (dialog) {
		delete dialog;
	}
#endif
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Setup the file system if using ROM.
//

static void setupFileSystem()
{
#if BLD_FEATURE_ROMFS
	romFileSystem = new MaRomFileSystem(defaultRomFiles);
	server->setFileSystem(romFileSystem);

	server->setServerRoot("/");
	serverRoot = server->getServerRoot();
#endif

	fileSystem = server->getFileSystem();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Turn on logging
//

static void setLogging(char *logSpec)
{
#if BLD_FEATURE_LOG
	mp->logService->setDefaultLevel(MPR_CONFIG);
	if (logSpec == 0 && !configFile && !background) {
		logSpec = "stdout:2";
	}
#if WIN
	dialog = new MprLogToWindow();
	mp->addListener(dialog);
#endif

	logger = new MprLogToFile();
	// logger->enableTimeStamps(1);
	mp->addListener(logger);

#if BLD_FEATURE_ROMFS
	//
	//	If ROMing, we can only log to stdout as we can't log to a read-only
	//	file system!! Alternatively, you can design your own listener and 
	//	install it here.
	//
	if (logSpec) {
		if (strncmp(logSpec, "stdout", 6) == 0) {
			mp->setLogSpec(logSpec);
		} else {
			mprFprintf(MPR_STDERR, "Can't log to %s when using ROMFS\n", 
				logSpec);
		}
	}
#else
	if (logSpec && mp->setLogSpec(logSpec) < 0) {
		mprFprintf(MPR_STDERR, "Can't open logSpec %s\n", logSpec);
		exit(2);
	}
#endif // BLD_FEATURE_ROMFS
#endif // BLD_FEATURE_LOG
}

////////////////////////////////////////////////////////////////////////////////
//
//	Intelligently find the server root directory if not specified.
//	Change the working directory to that location if not using ROMFS.
//

static int locateServerRoot(char *path)
{
#if BLD_FEATURE_CONFIG_PARSE && !BLD_FEATURE_ROMFS

	if (serverRoot == 0) {
		MprFileInfo	info;
		char		searchPath[MPR_MAX_FNAME * 8], pathBuf[MPR_MAX_FNAME];
		char 		cwd[MPR_MAX_FNAME];
		char		*tok, *searchBuf;

		//
		//	No explicit server root switch was supplied so search for 
		//	"mime.types" in the search path:
		//		.
		//		..
		//		../productName
		//		../../productName
		//	For windows, we also do:
		//		modDir							(where the exe was loaded from)
		//		modDir/..  
		//		modDir/../productName  
		//		modDir/../../productName 
		//	For all:
		//		BLD_PREFIX
		//
#if WIN
		char modDir[MPR_MAX_FNAME], modPath[MPR_MAX_FNAME];
		char module[MPR_MAX_FNAME];

		//
		//	Initially change directory to where the exe lives
		//
		GetModuleFileName(0, module, sizeof(module));
		mprGetDirName(modDir, sizeof(modDir), module);
		mprGetFullPathName(modPath, sizeof(modPath), modDir);

		/* NOTE: use tabs not spaces between items */
		mprSprintf(searchPath, sizeof(searchPath), 
			".	..	../%s	../../%s	"
			"%s	%s/..	%s/../%s	%s/../../%s	%s",
			BLD_PRODUCT, BLD_PRODUCT,
			modPath, modPath, modPath, BLD_PRODUCT, modPath, BLD_PRODUCT, 
				BLD_PREFIX);
#else
		mprSprintf(searchPath, sizeof(searchPath), 
			".	..	../%s	../../%s	%s",
			BLD_PRODUCT, BLD_PRODUCT, BLD_PREFIX);
#endif
		
		getcwd(cwd, sizeof(cwd) - 1);
		mprLog(3, "Root search path %s, cwd %s\n", searchPath, cwd);

		searchBuf = mprStrdup(searchPath);
		path = mprStrTok(searchBuf, "\t", &tok);
		while (path) {
			mprSprintf(pathBuf, sizeof(pathBuf), "%s/mime.types", path);
			mprLog(4, "Searching for %s\n", pathBuf);
			if (fileSystem->stat(pathBuf, &info) == 0) {
				break;
			}
			path = mprStrTok(0, "\t", &tok);
		}
		if (path == 0) {
			mprError(MPR_L, MPR_USER, 
				"Can't find suitable server root directory\n"
				"Using search path %s, and current directory %s\n"
				"Ensure you have adequate permissions to access the required "
				"directories.\n", searchPath, cwd);
			mprFree(searchBuf);
			return MPR_ERR_CANT_ACCESS;
		}
		server->setServerRoot(path);
		mprFree(searchBuf);

	} else {

		//
		//	Must program the server up for the server root. It will convert this
		//	path to an absolute path which we reassign to our notion of
		//	serverRoot.
		//
		server->setServerRoot(path);
	}
	serverRoot = server->getServerRoot();

#endif

#if !BLD_FEATURE_ROMFS
	chdir(serverRoot);
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Configure the server
//

static int setupServer(MaHttp *http, int poolThreads)
{
	mprLog(MPR_CONFIG, "Using server root: %s\n", serverRoot);

#if BLD_FEATURE_CONFIG_PARSE
	if (configFile) {
		if (configureViaFile() < 0) {
			return MPR_ERR_CANT_OPEN;
		}
	} else
#endif
	{
		if (configureViaApi() < 0) {
			return MPR_ERR_CANT_OPEN;
		}
	}

#if BLD_FEATURE_MULTITHREAD
	//
	//	The default limits my be updated via the config file
	//
	MaLimits *limits = http->getLimits();
	if (poolThreads >= 0) {
		limits->maxThreads = poolThreads;
		if (limits->minThreads > limits->maxThreads) {
			limits->minThreads = limits->maxThreads;
		}
	}
	if (limits->maxThreads > 0) {
		mprGetMpr()->setMaxPoolThreads(limits->maxThreads);
		mprGetMpr()->setMinPoolThreads(limits->minThreads);
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE
//
//	Use an Apache style configuration file to configure Appweb
//

static int configureViaFile()
{
	char	*path;

	mprAllocSprintf(&path, MPR_MAX_FNAME, "%s/%s", serverRoot, configFile);
	mprLog(MPR_CONFIG, "Using configuration file: \n"
		"                       \"%s\"\n", path);

	//
	//	Configure the http service and hosts specified in the config file.
	//
	if (server->configure(path) < 0) {
		mprError(MPR_L, MPR_USER, "Can't configure server using %s", path);
		mprFree(path);
		return MPR_ERR_CANT_INITIALIZE;
	}
	mprFree(path);
	return 0;
}

#endif	// BLD_FEATURE_CONFIG_PARSE
////////////////////////////////////////////////////////////////////////////////
//
//	Configure via APIs without the configure file. Hand-craft the server.
//	This is fairly complex as we are being thorough.
//

static int configureViaApi()
{
	MaHostAddress 	*address;
	MaHost			*host;
	MprList			*listens;
	MaListen		*lp;
	MprHashTable	*hostAddresses;
	MaDir			*dir;
	MaAlias			*ap;
	MaLocation		*loc;
	char			*cp, *docRootPath;
	char			addrBuf[MPR_MAX_IP_ADDR_PORT], pathBuf[MPR_MAX_FNAME];
	int				port;

	mprLog(MPR_CONFIG, "Configuration via Command Line\n");

#if BLD_FEATURE_ROMFS
	mprLog(MPR_CONFIG, "Server Root \"%s\" in ROM\n", serverRoot);
	docRootPath = mprStrdup(docRoot);
#else
	//
	//	Set the document root. Is relative to the server root unless an 
	//	absolute path is used.
	//	
#if WIN
	if (*docRoot != '/' && docRoot[1] != ':' && docRoot[2] != '/') 
#else
	if (*docRoot != '/')
#endif
	{
		mprAllocSprintf(&docRootPath, MPR_MAX_FNAME, "%s/%s", serverRoot, 
			docRoot);
	} else {
		docRootPath = mprStrdup(docRoot);
	}
#endif // BLD_FEATURE_ROMFS

	mprLog(MPR_CONFIG, "Document Root \"%s\"\n", docRootPath);

	//
	//	Setup the listening addresses. If only a port is specified, listen on
	//	all interfaces. If only the IP address is specified without a port,
	//	then default to port 80. IF autoScan is on, scan for a free port
	//	starting from the base address.
	//
	listens = server->getListens();

	port = MA_SERVER_DEFAULT_PORT_NUM;
	if ((cp = strchr(ipAddr, ':')) != 0) {
		*cp++ = '\0';
		port = atoi(cp);
		if (port <= 0 || port > 65535) {
			mprError(MPR_L, MPR_USER, "Bad listen port number %d", port);
			return MPR_ERR_BAD_SYNTAX;
		}
		if (autoScan) {
			port = findFreePort(ipAddr, port);
		}
		listens->insert(new MaListen(ipAddr, port, 0));

	} else {
		if (isdigit(*ipAddr) && strchr(ipAddr, '.') == 0) {
			port = atoi(ipAddr);
			if (port <= 0 || port > 65535) {
				mprError(MPR_L, MPR_USER, "Bad listen port number %d", port);
				return MPR_ERR_BAD_SYNTAX;
			}
			if (autoScan) {
				port = findFreePort("", port);
			}
			listens->insert(new MaListen("", port));

		} else {
			if (autoScan) {
				port = findFreePort(ipAddr, MA_SERVER_DEFAULT_PORT_NUM);
			}
			listens->insert(new MaListen(ipAddr, port));
		}
	}
	mprFree(ipAddr);
	ipAddr = 0;

	host = server->newHost(docRootPath);
	if (host == 0) {
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Add the default server listening addresses to the HostAddress hash.
	//	FUTURE -- this should be moved into newHost
	//
	hostAddresses = server->getHostAddresses();
	lp = (MaListen*) listens->getFirst();
	while (lp) {
		mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", lp->getIpAddr(), 
			lp->getPort());
		address = (MaHostAddress*) hostAddresses->lookup(addrBuf);
		if (address == 0) {
			address = new MaHostAddress(addrBuf);
			hostAddresses->insert(address);
		}
		mprLog(MPR_CONFIG, "Listening for HTTP on %s\n", addrBuf);
		address->insertVhost(new MaVhost(host));
		lp = (MaListen*) listens->getNext(lp);
		mprFree(ipAddr);
		ipAddr = mprStrdup(addrBuf);
	}

	//
	//	Setup a module search path that works for production and developement.
	//
#if BLD_FEATURE_DLL
	char	searchPath[MPR_MAX_FNAME];
	mprSprintf(searchPath, sizeof(searchPath), 
			"./lib ../lib ../../lib %s/lib", BLD_PREFIX);
	host->setModuleDirs(searchPath);
#endif

	//
	//	Load all possible modules
	//
#if BLD_FEATURE_AUTH_MODULE
	//
	//	Handler must be added first to authorize all requests.
	//
	if (server->loadModule("auth") == 0) {
		host->addHandler("authHandler", "");
	}
#endif
#if BLD_FEATURE_UPLOAD_MODULE
	//
	//	Must be after auth and before ESP, EGI.
	//
	if (server->loadModule("upload") == 0) {
		host->addHandler("uploadHandler", "");
	}
#endif
#if BLD_FEATURE_CGI_MODULE
	if (server->loadModule("cgi") == 0) {
		host->addHandler("cgiHandler", ".cgi .cgi-nph .bat .cmd .pl .py");
	}
#endif
#if BLD_FEATURE_EGI_MODULE
	if (server->loadModule("egi") == 0) {
		host->addHandler("egiHandler", ".egi");
	}
#endif
#if BLD_FEATURE_ESP_MODULE
	if (server->loadModule("esp") == 0) {
		host->addHandler("espHandler", ".esp .asp");
	}
#endif
#if BLD_FEATURE_C_API_MODULE
	server->loadModule("capi");
#endif
#if BLD_FEATURE_GACOMPAT_MODULE
	server->loadModule("compat");
#endif
#if BLD_FEATURE_SSL_MODULE
	server->loadModule("ssl");
#endif
	//
	//	Only load one of matrixSsl / openssl
	//
#if BLD_FEATURE_OPENSSL_MODULE
	server->loadModule("openSsl");
#elif BLD_FEATURE_MATRIXSSL_MODULE
	server->loadModule("matrixSsl");
#endif
#if BLD_FEATURE_PHP5_MODULE
	if (server->loadModule("php5") == 0) {
		host->addHandler("php5Handler", ".php");
	}
#endif
#if BLD_FEATURE_COPY_MODULE
	//
	//	Handler must be added last to be the catch all
	//
	if (server->loadModule("copy") == 0) {
		host->addHandler("copyHandler", "");
	}
#endif

	//
	//	Create the top level directory
	//
	dir = new MaDir(host);
	dir->setPath(docRootPath);
	host->insertDir(dir);

	//
	//	Add cgi-bin
	//
	mprSprintf(pathBuf, sizeof(pathBuf), "%s/cgi-bin", serverRoot);
	ap = new MaAlias("/cgi-bin/", pathBuf);
	mprLog(4, "ScriptAlias \"/cgi-bin/\":\n\t\t\t\"%s\"\n", pathBuf);
	host->insertAlias(ap);
	loc = new MaLocation(dir->getAuth());
	loc->setPrefix("/cgi-bin/");
	loc->setHandler("cgiHandler");
	host->insertLocation(loc);

	mprFree(docRootPath);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Try to find a free open port
//

static int findFreePort(char *ip, int base)
{
	MprSocket	*sock;
	int			port;

	sock = new MprSocket();

	for (port = base; port < base + MAX_PORT_TRIES; port++) {
		if (sock->openClient(ip, port, 0) >= 0) {
			sock->forcedClose();
			sock->dispose();
			return port;
		}
	}
	return port;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Sample main event loop. This demonstrates how to integrate Mpr with your
//	applications event loop using select()
//

void eventLoop()
{
#if WIN
	MSG		msg;
	int		till;

	mp->setRunningEventsThread(1);

	//
	//	If single threaded or if you desire control over the event loop, you
	//	should code an event loop similar to that below:
	//
	while (!mp->isExiting()) {

		if (mp->runTimers() > 0) {
			till = 0;
		} else {
			till = mp->getIdleTime();
		}

		//
		//	This will run tasks if poolThreads == 0 (single threaded). If 
		//	multithreaded, the thread pool will run tasks
		//
		if (mp->runTasks() > 0) {			// Returns > 0 if more work to do
			till = 0;						// So don't block in select
		}
		SetTimer(appHwnd, 0, till, NULL);

		//
		//	Socket events will be serviced in the msgProc
		//
		if (GetMessage(&msg, NULL, 0, 0) == 0) {
			//	WM_QUIT received
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#else
	mp->serviceEvents(0, -1);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Security checks. Make sure we are staring with a safe environment
//

static int securityChecks(char *program)
{
#if LINUX
	char			dir[MPR_MAX_FNAME];
	struct stat		sbuf;
	uid_t			uid;

	uid = getuid();
	if (getpwuid(uid) == 0) {
	    mprError(MPR_L, MPR_USER, "Bad user id: %d", uid);
	    return MPR_ERR_BAD_STATE;
	}

	dir[sizeof(dir) - 1] = '\0';
	if (getcwd(dir, sizeof(dir) - 1) == NULL) {
	    mprError(MPR_L, MPR_USER, "Can't get the current working directory");
	    return MPR_ERR_BAD_STATE;
	}

	if (((stat(dir, &sbuf)) != 0) || !(S_ISDIR(sbuf.st_mode))) {
	    mprError(MPR_L, MPR_USER, "Can't access directory: %s", dir);
	    return MPR_ERR_BAD_STATE;
	}
	if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
	    mprError(MPR_L, MPR_USER, 
			"Security risk, directory %s is writable by others", dir);
	}

	//
	//	Should always convert the program name into a fully qualified path
	//	Otherwise this fails
	//
	if (*program == '/') {
		if (((lstat(program, &sbuf)) != 0) || (S_ISLNK(sbuf.st_mode))) {
			mprError(MPR_L, MPR_USER, "Can't access program: %s", program);
			return MPR_ERR_BAD_STATE;
		}
		if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
			mprError(MPR_L, MPR_USER, 
				"Security risk, Program %s is writable by others", program);
		}
		if (sbuf.st_mode & S_ISUID) {
			mprError(MPR_L, MPR_USER, "Security risk, %s is setuid", program);
		}
		if (sbuf.st_mode & S_ISGID) {
			mprError(MPR_L, MPR_USER, "Security risk, %s is setgid", program);
		}
	}
#endif
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
#if WIN
//
//	Start the user's default browser
//

static void runBrowser(char *page)
{
#if BLD_FEATURE_CGI_MODULE
	MprCmd		*cmd;
	char		cmdBuf[MPR_MAX_STRING];
	char		*path, *ip;
	char		*pathArg;

	getBrowserPath(&path, MPR_MAX_STRING);

	ip = server->getDefaultHost()->getIpSpec();
	pathArg = strstr(path, "\"%1\"");
	if (*page == '/') {
		page++;
	}

	if (pathArg == 0) {
		mprSprintf(cmdBuf, MPR_MAX_STRING, "%s http://%s/%s", path, 
			ip, page);
	} else {
		//
		//	Patch out the "%1"
		//
		*pathArg = '\0';
		mprSprintf(cmdBuf, MPR_MAX_STRING, "%s \"http://%s/%s\"", path, 
			ip, page);
	}

	mprLog(4, "Running %s\n", cmdBuf);
	cmd = new MprCmd();
	cmd->start(cmdBuf, MPR_CMD_SHOW);
	mprFree(path);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the path to run the user's default browser. Caller must free the 
//	return string.
// 

static int getBrowserPath(char **path, int max)
{
	mprAssert(path);

#if LINUX
	if (access("/usr/bin/htmlview", X_OK) == 0) {
		*path = mprStrdup("/usr/bin/htmlview");
		return 0;
	}
	if (access("/usr/bin/mozilla", X_OK) == 0) {
		*path = mprStrdup("/usr/bin/mozilla");
		return 0;
	}
	if (access("/usr/bin/konqueror", X_OK) == 0) {
		*path = mprStrdup("/usr/bin/knonqueror");
		return 0;
	}
	return MPR_ERR_CANT_ACCESS;

#endif
#if WIN
	char	cmd[MPR_MAX_STRING];
	char	*type;
	char	*cp;

	if (mprReadRegistry("HKEY_CLASSES_ROOT\\.htm", "", &type, 
			MPR_MAX_STRING) < 0) {
		return MPR_ERR_CANT_ACCESS;
	}

	mprSprintf(cmd, MPR_MAX_STRING,
		"HKEY_CLASSES_ROOT\\%s\\shell\\open\\command", type);
	mprFree(type);

	if (mprReadRegistry(cmd, "", path, max) < 0) {
		mprFree(cmd);
		return MPR_ERR_CANT_ACCESS;
	}

	for (cp = *path; *cp; cp++) {
		if (*cp == '\\') {
			*cp = '/';
		}
		*cp = tolower(*cp);
	}
#endif
	mprLog(4, "Browser path: %s\n", *path);
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Print the version information
//

static void printVersion()
{
#if WIN
	mprError(MPR_L, MPR_USER, "Version: %s %s", mp->getAppTitle(), BLD_VERSION);
	return;
#else
	mprPrintf("%s: Version: %s\n", mp->getAppTitle(), BLD_VERSION);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Display the usage
//

static void printUsage(char *programName)
{
#if WIN
	if (!isService) {
		mprError(MPR_L, MPR_USER, "Bad command line arguments");
		return;
	}
#endif
	mprFprintf(MPR_STDERR, 
		"usage: %s [-Abdkmv] [-a IP:PORT] [-d docRoot] [-f configFile]\n"
		"       [-l logSpec] [-r serverRootDir] [-t numThreads]\n\n",  
		programName);

	mprFprintf(MPR_STDERR, "Options:\n");
	mprFprintf(MPR_STDERR, "  -a IP:PORT     Address to listen on\n");
	mprFprintf(MPR_STDERR, "  -A             Auto-scan for a free port\n");
	mprFprintf(MPR_STDERR, "  -b             Run in background as a daemon\n");
	mprFprintf(MPR_STDERR, "  -d docRoot     Web directory (DocumentRoot)\n");

#if BLD_FEATURE_CONFIG_PARSE
	mprFprintf(MPR_STDERR, "  -f configFile  Configuration file name\n");
#else
	mprFprintf(MPR_STDERR, "  -f configFile  NOT SUPPORTED in this build\n");
#endif

	mprFprintf(MPR_STDERR, "  -k             Kill existing running http\n");
	mprFprintf(MPR_STDERR, "  -l file:level  Log to file at verbosity level\n");
	mprFprintf(MPR_STDERR, "  -r serverRoot  Alternate Home directory\n");

	mprFprintf(MPR_STDERR, "\nDebug options\n");
	mprFprintf(MPR_STDERR, "  -D             Debug mode (no timeouts)\n");
	mprFprintf(MPR_STDERR, "  -m             Output memory stats\n");
	mprFprintf(MPR_STDERR, "  -t number      Use number of pool threads\n");
	mprFprintf(MPR_STDERR, "  -v             Output version information\n");

	mprFprintf(MPR_STDERR, "\nWindows options\n");
	mprFprintf(MPR_STDERR, "  -i             Install service\n");
	mprFprintf(MPR_STDERR, "  -g             Go (start) service\n");
	mprFprintf(MPR_STDERR, "  -s             Stop service\n");
	mprFprintf(MPR_STDERR, "  -u             Uninstall service\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//	Emergency memory failure handler. FUTURE -- add reboot code here
//	Need a -C rebootCount switch. Must set all this up first thing on booting
//	as we won't be able to get ram here.
//

static void memoryFailure(int askSize, int totalHeapMem, int limit)
{
#if WIN
	char	buf[MPR_MAX_STRING];

	mprSprintf(buf, sizeof(buf), "Can't get %d bytes of memory\n"
		"Total heap is %d. Limit set to %d", askSize, totalHeapMem, limit);
	mprError(MPR_L, MPR_USER, buf);
#else
	mprPrintf("Can't get %d bytes of memory\n", askSize);
	mprPrintf("Total heap is %d. Limit set to %d\n", totalHeapMem, limit);
#endif
	exit(8);
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
//
//	Do windows specific intialization
//

static int windowsInit()
{
#if ONLY_SINGLE_INSTANCE
	if (serviceOp == 0 && findInstance()) {
		mprError(MPR_L, MPR_USER, "Application %s is already active.", program);
		return MPR_ERR_BUSY;
	}
#endif

	//
	//	Create the window
	// 
	if (initWindow() < 0) {
		mprError(MPR_L, MPR_ERROR, "Can't initialize application Window");
		return MPR_ERR_CANT_INITIALIZE;
	}

	if (trayIcon > 0) {
		if (openTrayIcon() < 0 && mp->isService()) {
			trayTimer = new MprTimer(10 * 1000, trayIconProc, (void *) NULL);
		}
	}
	mprGetMpr()->setAsyncSelectMode(MPR_ASYNC_SELECT);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_RUN_AS_SERVICE
//
//	Do Windows service commands
//

static int windowsServiceOps()
{
	winService = new MprWinService(APPWEB_SERVICE_NAME);
	switch (serviceOp) {
	case MPR_INSTALL_SERVICE:
		char path[MPR_MAX_FNAME], cmd[MPR_MAX_FNAME];
		GetModuleFileName(0, path, sizeof(path));
		mprSprintf(cmd, sizeof(cmd), "\"%s\" %s", path, serviceCmdLine);
		winService->install(APPWEB_SERVICE_DISPLAY, cmd);
		break;

	case MPR_UNINSTALL_SERVICE:
		winService->remove(1);
		break;

	case MPR_GO_SERVICE:
		winService->start();
		//
		//	Give time for service to actually start 
		//
		mprSleep(2000);
		break;

	case MPR_STOP_SERVICE:
		winService->remove(0);
		break;
	}
	if (isService) {
		mprGetMpr()->setService(1);
	}
	return 0;
}
#endif 	//	BLD_FEATURE_RUN_AS_SERVICE

///////////////////////////////////////////////////////////////////////////////
//
//	See if an instance of this product is already running
//

static int findInstance()
{
	HWND	hwnd;

	hwnd = FindWindow(mp->getAppName(), mp->getAppTitle());
	if (hwnd) {
		otherHwnd = hwnd;
		if (IsIconic(hwnd)) {
			ShowWindow(hwnd, SW_RESTORE);
		}
		SetForegroundWindow(hwnd);
		// SendMessage(hwnd, WM_COMMAND, MPR_MENU_HOME, 0);
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Initialize the applications's window
// 

static int initWindow()
{
	WNDCLASS 	wc;
	int			rc;

	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= (HINSTANCE) appInst;
	wc.hIcon			= NULL;
	wc.lpfnWndProc		= (WNDPROC) msgProc;
	wc.lpszMenuName		= wc.lpszClassName = mp->getAppName();

	rc = RegisterClass(&wc);
	if (rc == 0) {
		mprError(MPR_L, MPR_ERROR, "Can't register windows class");
		return -1;
	}

	appHwnd = CreateWindow(mp->getAppName(), mp->getAppTitle(), WS_OVERLAPPED,
		CW_USEDEFAULT, 0, 0, 0, NULL, NULL, appInst, NULL);

	if (! appHwnd) {
		mprError(MPR_L, MPR_ERROR, "Can't create window");
		return -1;
	}
	mp->setHwnd(appHwnd);
	mp->setSocketHwnd(appHwnd);
	mp->setSocketMessage(APPWEB_SOCKET_MESSAGE);

	if (taskBarIcon > 0) {
		ShowWindow(appHwnd, SW_MINIMIZE);
		UpdateWindow(appHwnd);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Windows message processing loop
//

static long msgProc(HWND hwnd, uint msg, uint wp, long lp)
{
	char	buf[MPR_MAX_FNAME];
	int		sock, winMask;

	switch (msg) {
	case WM_DESTROY:
	case WM_QUIT:
		mp->terminate(1);
		break;
	
	case APPWEB_SOCKET_MESSAGE:
		sock = wp;
		winMask = LOWORD(lp);
		// errCode = HIWORD(lp);
		mp->serviceIO(sock, winMask);
		break;

	case APPWEB_TRAY_MESSAGE:
		return trayEvent(hwnd, wp, lp);
		break;

	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case MPR_HTTP_MENU_CONSOLE:
			runBrowser("/admin/index.html");
			break;

		case MPR_HTTP_MENU_HELP:
			runBrowser("/doc/product/index.html");
			break;

		case MPR_HTTP_MENU_ABOUT:
			//
			//	Single-threaded users beware. This blocks !!!
			//
			mprSprintf(buf, sizeof(buf), "Mbedthis %s %s-%s", 
				BLD_NAME, BLD_VERSION, BLD_NUMBER);
			MessageBoxEx(NULL, buf, mp->getAppTitle(), MB_OK, 0);
			break;

		case MPR_HTTP_MENU_STOP:
			mp->terminate(1);
			break;

		default:
			return DefWindowProc(hwnd, msg, wp, lp);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Redisplay the icon. If running as a service, the icon should be retried
//	incase the user logs in.
// 

static void trayIconProc(void *arg, MprTimer *tp)
{
	closeTrayIcon();
	if (openTrayIcon() < 0) {
		tp->reschedule();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Can be called multiple times 
//

static int openTrayIcon()
{
	NOTIFYICONDATA	data;
	HICON			iconHandle;
	static int		doOnce = 0;


	if (trayMenu == NULL) {
		trayMenu = LoadMenu(appInst, "trayMenu");
		if (! trayMenu) {
			mprError(MPR_L, MPR_LOG, "Can't locate trayMenu");
			return MPR_ERR_CANT_OPEN;
		}
	}
	if (subMenu == NULL) {
		subMenu = GetSubMenu(trayMenu, 0);
	}

	iconHandle = (HICON) LoadImage(appInst, APPWEB_ICON, IMAGE_ICON, 0, 0,
		LR_LOADFROMFILE | LR_DEFAULTSIZE);
	if (iconHandle == 0) {
		mprError(MPR_L, MPR_LOG, "Can't load icon %s", APPWEB_ICON);
		return MPR_ERR_CANT_INITIALIZE;
	}

	data.uID = APPWEB_TRAY_ID;
	data.hWnd = appHwnd;
	data.hIcon = iconHandle;
	data.cbSize = sizeof(NOTIFYICONDATA);
	data.uCallbackMessage = APPWEB_TRAY_MESSAGE;
	data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	mprStrcpy(data.szTip, sizeof(data.szTip), mp->getAppTitle());

	Shell_NotifyIcon(NIM_ADD, &data);

	if (iconHandle) {
		DestroyIcon(iconHandle);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Can be caleld multiple times
//

static void closeTrayIcon()
{
	NOTIFYICONDATA	data;

	data.uID = APPWEB_TRAY_ID;
	data.hWnd = appHwnd;
	data.cbSize = sizeof(NOTIFYICONDATA);
	Shell_NotifyIcon(NIM_DELETE, &data);
	if (trayMenu) {
		DestroyMenu(trayMenu);
		trayMenu = NULL;
	}
	if (subMenu) {
		DestroyMenu(subMenu);
		subMenu = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Respond to tray icon events
//

static int trayEvent(HWND hwnd, WPARAM wp, LPARAM lp)
{
	RECT		windowRect;
	POINT		p, pos;
	uint		msg;

	msg = (uint) lp;

	//
	//	Show the menu on single right click
	//
	if (msg == WM_RBUTTONUP) {
		HWND	h = GetDesktopWindow();
		GetWindowRect(h, &windowRect);
		GetCursorPos(&pos);

		p.x = pos.x;
		p.y = windowRect.bottom;

		SetForegroundWindow(appHwnd);
		TrackPopupMenu(subMenu, TPM_RIGHTALIGN | TPM_RIGHTBUTTON, p.x, p.y, 
			0, appHwnd, NULL);
		// FUTURE -- PostMessage(appHwnd, WM_NULL, 0, 0);
		mp->selectService->awaken();
		return 0;
	}

	//
	//	Launch the browser on a double click
	//
	if (msg == WM_LBUTTONDBLCLK) {
		runBrowser("/");
		return 0;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////

static void mapPathDelim(char *s)
{
	while (*s) {
		if (*s == '\\') {
			*s = '/';
		}
		s++;
	}
}
#endif 	//	!WIN

////////////////////////////////////////////////////////////////////////////////
#if BLD_HOST_UNIX || VXWORKS

static void initSignals()
{
	struct sigaction	act;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = catchSignal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (mp->isService()) {
		sigaction(SIGINT, &act, 0);
		sigaction(SIGQUIT, &act, 0);
	}
	sigaction(SIGTERM, &act, 0);
	signal(SIGPIPE, SIG_IGN);

#if LINUX
	//
	//	We will catch these because the write request will fail anyway.
	//
	signal(SIGXFSZ, SIG_IGN);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Catch signals. Do a graceful shutdown.
//

static void catchSignal(int signo, siginfo_t *info, void *arg)
{
	char	filler[32];

	//
	//	Fix for GCC optimization bug on Linux
	//
	filler[0] = filler[sizeof(filler) - 1];

	mprLog(MPR_INFO, "Received signal %d\nExiting ...\n", signo);
	if (mp) {
		mp->terminate(1);
	}
}

#endif // BLD_HOST_UNIX || VXWORKS

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
