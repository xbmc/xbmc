#include "stdafx.h"
/* WebServer.cpp: implementation of the CWebServer class.
 * A darivation of:  main.c -- Main program for the GoAhead WebServer
 *
 * main.c -- Main program for the GoAhead WebServer
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 */

#include <io.h>
#include <vector>
#include "WebServer.h"
#include "XBMCWeb.h"

#ifdef SPYCE_SUPPORT
#include "SpyceModule.h"
#endif

#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

void WebsOutputString(const char* pszFormat, ...)
{
	va_list argList;
	char m_pszBuffer[1024];

	va_start( argList, pszFormat );
	vsprintf( m_pszBuffer, pszFormat, argList );
	OutputDebugString( m_pszBuffer );
	va_end( argList );
}

// this is from a C library so use C style function calls
#ifdef __cplusplus
extern "C" {
#endif

/**************************** Forward Declarations ****************************/
extern void		basicSetProductDir(char_t *proddir);
static int		websHomePageHandler(webs_t wp, char_t *urlPrefix,
					char_t *webDir, int arg, char_t *url, char_t *path,
					char_t *query);
extern void		defaultErrorHandler(int etype, char_t *msg);
extern void		defaultTraceHandler(int level, char_t *buf);
static unsigned char	*tableToBlock(char **table);
static void		printMemStats(int handle, char_t *fmt, ...);
static void		memLeaks();
static int		nCopyAnsiToWideChar(LPWORD, LPSTR);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWebServer::CWebServer()
{
	/*********************************** Locals ***********************************/
	/*
	 *	Change configuration here
	 */
	pXbmcWeb = new CXbmcWeb();
	m_password = T("");				/* Security password */
	m_port = 80;					/* Server port */

	m_hEvent = CreateEvent(NULL, false, false, "webserverEvent");
}


CWebServer::~CWebServer()
{
	CloseHandle(m_hEvent);
	if (pXbmcWeb != NULL) delete pXbmcWeb;
}

DWORD CWebServer::SuspendThread()
{
	return ::SuspendThread(m_ThreadHandle);
}

DWORD CWebServer::ResumeThread()
{
	BOOL res=::ResumeThread(m_ThreadHandle);
	if (res)
	{
		WaitForSingleObject(m_hEvent, INFINITE);
	}
	return res;
}

bool CWebServer::Start(const char *szLocalAddress, int port, const char_t* web)
{
	m_szLocalAddress = szLocalAddress;
	strcpy(m_szRootWeb, web);
	m_port = port;

	m_bFinished = false;
	WebsOutputString( "WebServer: Server starting using %s on %s:%i\n", 
		m_szRootWeb, m_szLocalAddress, m_port);

	Create(false);
	if( m_ThreadHandle == NULL)
		return false;

	// wait for initinstance has been executed
	WaitForSingleObject(m_hEvent, INFINITE);

	return true;
}

void CWebServer::Stop()
{
	m_bFinished = true;
	// wait until onexit() is finished.
	WaitForSingleObject(m_hEvent, INFINITE);
}


//*************************************************************************************
void CWebServer::OnStartup()
{
	/*
	 *	Initialize the memory allocator. Allow use of malloc and start 
	 *	with a 60K heap.  For each page request approx 8KB is allocated.
	 *	60KB allows for several concurrent page requests.  If more space
	 *	is required, malloc will be used for the overflow.
	 */
	bopen(NULL, (60 * 1024), B_USE_MALLOC);

	/*
	 *	Initialize the web server
	 */
	if (initWebs() < 0) 
	{
		m_bFinished = true;
		SetEvent(m_hEvent);
		return;
	}

	#ifdef WEBS_SSL_SUPPORT
		websSSLOpen();
	#endif

	SetEvent(m_hEvent);
}

/******************************************************************************/
/*
 *	Initialize the web server.
 */

int CWebServer::initWebs()
{
	/*
	 *	Initialize the socket subsystem
	 */
	socketOpen();

	/*
	 *	Initialize the User Management database
	 */
	
	#ifdef USER_MANAGEMENT_SUPPORT
		umOpen();
		basicSetProductDir(m_szRootWeb);
		umRestore("umconfig.txt");
	#endif

	#ifdef SPYCE_SUPPORT
		setSpyOpenCallback(WEBS_SPYCE::spyceOpen);
		setSpyCloseCallback(WEBS_SPYCE::spyceClose);
		setSpyRequestCallback(WEBS_SPYCE::spyceRequest);
	#endif
	/*
	 *	Define the local Ip address, host name, default home page and the 
	 *	root web directory.
	 */
	websSetDefaultDir(m_szRootWeb);
	websSetIpaddr((char*)m_szLocalAddress);
	websSetHost((char*)m_szLocalAddress);

	/*
	 *	Configure the web server options before opening the web server
	 */
	websSetDefaultPage(T("default.asp"));
	websSetPassword((char*)m_password);

	/* 
	 *	Open the web server on the given port. If that port is taken, try
	 *	the next sequential port for up to "retries" attempts.
	 */

	if(websOpenServer(m_port, 5 /* retries*/) < 0) return -1;

	/*
	 * 	First create the URL handlers. Note: handlers are called in sorted order
	 *	with the longest path handler examined first. Here we define the security 
	 *	handler, forms handler and the default web page handler.
	 */
	websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, WEBS_HANDLER_FIRST);
	websUrlHandlerDefine(T("/goform"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T("/xbmcCmds"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, WEBS_HANDLER_LAST); 

	/*
	 *	Now define two test procedures. Replace these with your application
	 *	relevant ASP script procedures and form functions.
	 */
	websAspDefine(T("aspTest"), aspTest);
	websFormDefine(T("formTest"), formTest);
	
	websAspDefine(T("xbmcCommand"), XbmcWebsAspCommand);
	websAspDefine(T("xbmcCfg"), XbmcWebsAspConfiguration);
	websFormDefine(T("xbmcForm"), XbmcWebsForm);

	/*
	 *	Create the Form handlers for the User Management pages
	 */

	#ifdef USER_MANAGEMENT_SUPPORT
		formDefineUserMgmt();
	#endif

	/*
	 *	Create a handler for the default home page
	 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 

	/* 
	 *	Set the socket service timeout to the default
	 */
	m_sockServiceTime = SOCK_DFT_SVC_TIME;				

	return 0;
}


//*************************************************************************************
void CWebServer::OnExit()
{
	OutputDebugString("WebServer:OnExit - Exit web server.\n");

	#ifdef WEBS_SSL_SUPPORT
		websSSLClose();
	#endif

	/*
	 *	Close the User Management database
	 */
	#ifdef USER_MANAGEMENT_SUPPORT
		umClose();
	#endif

	/*
	 *	Close the socket module, report memory leaks and close the memory allocator
	 */
	websCloseServer();
	socketClose();

	/*
	 *	Free up Windows resources
	 */

	#ifdef B_STATS
		memLeaks();
	#endif
	
	bclose();

	PulseEvent(m_hEvent);
}


//*************************************************************************************
void CWebServer::Process()
{

	// always be checking he windows message q and can kill the process...
	/*
	 *	Basic event loop. SocketReady returns true when a socket is ready for
	 *	service. SocketSelect will block until an event occurs. SocketProcess
	 *	will actually do the servicing.
	 */
	int sockReady, sockSelect;

	while (!m_bFinished) 
	{
		sockReady = socketReady(-1);
		sockSelect = socketSelect(-1, m_sockServiceTime);
		if (sockReady || sockSelect) {
			socketProcess(-1);
		}
		emfSchedProcess();
		// QS - we do not support cgi for the moment
		// websCgiCleanup();
	}
	
	WebsOutputString("WebServer:Exiting thread sockReady=%i, sockSelect=%i.\n", sockReady, sockSelect);
}

const char_t* CWebServer::GetPassword()
{
	return m_password;
}

void CWebServer::SetPassword(char_t* Password)
{
	m_password = Password;
}

/******************************************************************************/
/*
 *	Home page handler
 */
static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
							int arg, char_t *url, char_t *path, char_t *query)
{
/*
 *	If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
	bool redirected = false;
	char dir[1024];
	char files[][20] = {
			"index.html",
			"index.htm",
			"home.htm",
			"home.html",
			"default.asp",
			"home.asp",
			NULL, };

	// check if one of the above files exist, if one does then redirect to it.
	strcpy(dir, websGetDefaultDir());
	strcat(dir, path);
	for(u_int pos = 0; pos < strlen(dir); pos++)
		if (dir[pos] == '/') dir[pos] = '\\';
	
	DWORD attributes = GetFileAttributes(dir);
	if (FILE_ATTRIBUTE_DIRECTORY == attributes)
	{
    int i = 0;
		char buf[1024];
		while (files[i][0])
		{
			strcpy(buf, dir);
			if (buf[strlen(buf)-1] != '\\') strcat(buf, "\\");
			strcat(buf, files[i]);

			if (!access(buf, 0))
			{
				strcpy(buf, path);
				if (path[strlen(path)-1] != '/') strcat(buf, "/");
				strcat(buf, files[i]);
				websRedirect(wp, buf);
				return 1;
			}
			i++;
		}

		//no default file found, list directory contents
		string strPath = wp->path;
		if(strPath[strPath.length()-1] != '/')
		{
			websRedirect(wp, (char*)(strPath + "/").c_str());
			return 0;
		}
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		vector<string> vecFiles;
		strcat(dir, "\\*");
		hFind=FindFirstFile(dir, &FindFileData);
	
		do
		{
			string fn = FindFileData.cFileName;
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) fn+= "/";
			vecFiles.push_back(fn);
		}
		while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);

		sort(vecFiles.begin(), vecFiles.end());

		websWrite(wp, "%s", "<title>Directory listing for /</title>\n");
		websWrite(wp, "%s", "<h2>Directory listing for /</h2>\n");
		websWrite(wp, "%s", "<hr>\n");
		websWrite(wp, "%s", "<ul>\n");
		 
		vector<string>::iterator it = vecFiles.begin();
		while (it != vecFiles.end())
		{
			string w = "<li><a href='";
			//wp->lpath
			//if(path[0]
			w += *it;
			w += "'>";
			w += *it;
			w += "</a>\n";
			websWrite(wp, "%s", w.c_str());
			++it;
		}
		
		websWrite(wp, "%s", "</ul></hr>\n");
		websDone(wp, 200);
		return 1;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Default error handler.  The developer should insert code to handle
 *	error messages in the desired manner.
 */

void defaultErrorHandler(int etype, char_t *msg)
{
	OutputDebugString(msg);//write(1, msg, gstrlen(msg));
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void defaultTraceHandler(int level, char_t *buf)
{
/*
 *	The following code would write all trace regardless of level
 *	to stdout.
 */
	if (buf) {
		//write(1, buf, gstrlen(buf));
	}
}

/******************************************************************************/
/*
 *	Returns a pointer to an allocated qualified unique temporary file name.
 *	This filename must eventually be deleted with bfree().
 */

char_t *websGetCgiCommName()
{
	char_t	*pname1, *pname2;

	pname1 = tempnam(NULL, T("cgi"));
	pname2 = bstrdup(B_L, pname1);
	free(pname1);
	return pname2;
}

/******************************************************************************/
/*
 *	Convert a table of strings into a single block of memory.
 *	The input table consists of an array of null-terminated strings,
 *	terminated in a null pointer.
 *	Returns the address of a block of memory allocated using the balloc() 
 *	function.  The returned pointer must be deleted using bfree().
 *	Returns NULL on error.
 */

static unsigned char *tableToBlock(char **table)
{
    unsigned char	*pBlock;		/*  Allocated block */
    char			*pEntry;		/*  Pointer into block      */
    size_t			sizeBlock;		/*  Size of table           */
    int				index;			/*  Index into string table */

    a_assert(table);

/*  
 *	Calculate the size of the data block.  Allow for final null byte. 
 */
    sizeBlock = 1;                    
    for (index = 0; table[index]; index++) {
        sizeBlock += strlen(table[index]) + 1;
	}

/*
 *	Allocate the data block and fill it with the strings                   
 */
    pBlock = (unsigned char *)balloc(B_L, sizeBlock);

	if (pBlock != NULL) {
		pEntry = (char *) pBlock;

        for (index = 0; table[index]; index++) {
			strcpy(pEntry, table[index]);
			pEntry += strlen(pEntry) + 1;
		}

/*		
 *		Terminate the data block with an extra null string                
 */
		*pEntry = '\0';              
	}

	return pBlock;
}


/******************************************************************************/
/*
 *	Create a temporary stdout file and launch the CGI process.
 *	Returns a handle to the spawned CGI process.
 */

int websLaunchCgiProc(char_t *cgiPath, char_t **argp, char_t **envp, 
					  char_t *stdIn, char_t *stdOut)
{
#ifndef _XBOX

	STARTUPINFO			newinfo;	
	SECURITY_ATTRIBUTES security;
	PROCESS_INFORMATION	procinfo;		/*  Information about created proc   */
	DWORD				dwCreateFlags;
	char_t				*cmdLine;
	char_t				**pArgs;
	BOOL				bReturn;
	int					i, nLen;
	unsigned char		*pEnvData;

/*
 *	Replace directory delimiters with Windows-friendly delimiters
 */
	nLen = gstrlen(cgiPath);
	for (i = 0; i < nLen; i++) {
		if (cgiPath[i] == '/') {
			cgiPath[i] = '\\';
		}
	}
/*
 *	Calculate length of command line
 */
	nLen = 0;
	pArgs = argp;
	while (pArgs && *pArgs && **pArgs) {
		nLen += gstrlen(*pArgs) + 1;
		pArgs++;
	}

/*
 *	Construct command line
 */
	cmdLine = ( char_t *) balloc(B_L, sizeof(char_t) * nLen);
	a_assert (cmdLine);
	gstrcpy(cmdLine, "");

	pArgs = argp;
	while (pArgs && *pArgs && **pArgs) {
		gstrcat(cmdLine, *pArgs);
		gstrcat(cmdLine, T(" "));
		pArgs++;
	}

 /*
 *	Create the process start-up information 
 */
	memset (&newinfo, 0, sizeof(newinfo));
	newinfo.cb          = sizeof(newinfo);
	newinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	newinfo.wShowWindow = SW_HIDE;
	newinfo.lpTitle     = NULL;

/*
 *	Create file handles for the spawned processes stdin and stdout files
 */
	security.nLength = sizeof(SECURITY_ATTRIBUTES);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = TRUE;

/*
 *	Stdin file should already exist.
 */
	newinfo.hStdInput = CreateFile(stdIn, GENERIC_READ,
		FILE_SHARE_READ, &security, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

/*
 *	Stdout file is created and file pointer is reset to start.
 */
	newinfo.hStdOutput = CreateFile(stdOut, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ + FILE_SHARE_WRITE, &security, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer (newinfo.hStdOutput, 0, NULL, FILE_END);

/*
 *	Stderr file is set to Stdout.
 */
	newinfo.hStdError =	newinfo.hStdOutput;

	dwCreateFlags = CREATE_NEW_CONSOLE;
	pEnvData = tableToBlock(envp);

/*
 *	CreateProcess returns errors sometimes, even when the process was    
 *	started correctly.  The cause is not evident.  For now: we detect    
 *	an error by checking the value of procinfo.hProcess after the call.  
 */
    procinfo.hProcess = NULL;
    bReturn = CreateProcess(
        NULL,				/*  Name of executable module        */
        cmdLine,			/*  Command line string              */
        NULL,				/*  Process security attributes      */
        NULL,				/*  Thread security attributes       */
        TRUE,				/*  Handle inheritance flag          */
        dwCreateFlags,		/*  Creation flags                   */
        pEnvData,			/*  New environment block            */
        NULL,				/*  Current directory name           */
        &newinfo,			/*  STARTUPINFO                      */
        &procinfo);			/*  PROCESS_INFORMATION              */

	if (procinfo.hThread != NULL)  {
		CloseHandle(procinfo.hThread);
	}

	if (newinfo.hStdInput) {
		CloseHandle(newinfo.hStdInput);
	}

	if (newinfo.hStdOutput) {
		CloseHandle(newinfo.hStdOutput);
	}

	bfree(B_L, pEnvData);
	bfree(B_L, cmdLine);

	if (bReturn == 0) {
		return -1;
	} else {
		return (int) procinfo.hProcess;
	}
#endif //_XBOX
	// QS so the other part of the server knows that we did nothing
	return -1;
}

/******************************************************************************/
/*
 *	Check the CGI process.  Return 0 if it does not exist; non 0 if it does.
 */

int websCheckCgiProc(int handle)
{
#ifndef _XBOX
	int		nReturn;
	DWORD	exitCode;

	nReturn = GetExitCodeProcess((HANDLE)handle, &exitCode);
/*
 *	We must close process handle to free up the window resource, but only
 *  when we're done with it.
 */
	if ((nReturn == 0) || (exitCode != STILL_ACTIVE)) {
		CloseHandle((HANDLE)handle);
		return 0;
	}
#endif //_XBOX

	return 1;
}

#ifdef B_STATS
static void memLeaks() 
{
	int		fd;

	if ((fd = gopen(T("leak.txt"), O_CREAT | O_TRUNC | O_WRONLY)) >= 0) {
		bstats(fd, printMemStats);
		close(fd);
	}
}

/******************************************************************************/
/*
 *	Print memory usage / leaks  
 */

static void printMemStats(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	write(handle, buf, strlen(buf));
}
#endif

/* Test Javascript binding for ASP. This will be invoked when "aspTest" is
 * embedded in an ASP page. See web/asp.asp for usage. Set browser to 
 * "localhost/asp.asp" to test.
 */
int aspTest(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*name, *address;

	if (ejArgs(argc, argv, T("%s %s"), &name, &address) < 2) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	return websWrite(wp, T("Name: %s, Address %s"), name, address);
}

/* Test form for posted data (in-memory CGI). This will be called when the
 * form in web/forms.asp is invoked. Set browser to "localhost/forms.asp" to test.
 */
void formTest(webs_t wp, char_t *path, char_t *query)
{
	char_t	*name, *address;

	name = websGetVar(wp, T("name"), T("Joe Smith")); 
	address = websGetVar(wp, T("address"), T("1212 Milky Way Ave.")); 

	websHeader(wp);
	websWrite(wp, T("<body><h2>Name: %s, Address: %s</h2>\n"), name, address);
	websFooter(wp);
	websDone(wp, 200);
}

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

int  XbmcWebsAspCommand(int eid, webs_t wp, int argc, char_t **argv)
{
	if (!pXbmcWeb) return -1;
	return pXbmcWeb->xbmcCommand(eid, wp, argc, argv);
}

int  XbmcWebsAspConfiguration( int eid, webs_t wp, int argc, char_t **argv)
{
	if (!pXbmcWeb) return -1;
	return pXbmcWeb->xbmcConfiguration(eid, wp, argc, argv);
}

void  XbmcWebsForm(webs_t wp, char_t *path, char_t *query)
{
	if (!pXbmcWeb) return;
	return pXbmcWeb->xbmcForm(wp, path, query);
}

#if defined(__cplusplus)
}
#endif 
