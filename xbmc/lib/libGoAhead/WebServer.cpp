
/* WebServer.cpp: implementation of the CWebServer class.
 * A darivation of:  main.c -- Main program for the GoAhead WebServer
 *
 * main.c -- Main program for the GoAhead WebServer
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 */

#include "stdafx.h"
#include "WebServer.h"
#include "XBMCweb.h"

#ifdef SPYCE_SUPPORT
#include "SpyceModule.h"
#endif

#include "XBMCweb.h"
#include "XBMCConfiguration.h"
#include "XBMChttp.h"
#include "includes.h"

static CXbmcWeb* pXbmcWeb;
static CXbmcConfiguration* pXbmcWebConfig;

#ifndef __GNUC__
#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

#pragma comment(linker, "/merge:WEB_TEXT=LIBHTTP")
#pragma comment(linker, "/merge:WEB_DATA=LIBHTTP")
#pragma comment(linker, "/merge:WEB_BSS=LIBHTTP")
#pragma comment(linker, "/merge:WEB_RD=LIBHTTP")
#pragma comment(linker, "/section:LIBHTTP,RWE")
#endif

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
//static void		memLeaks();
//static int		nCopyAnsiToWideChar(LPWORD, LPSTR);
extern void		dbZero(int did);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWebServer::CWebServer()
{
  pXbmcWeb = new CXbmcWeb();
  pXbmcWebConfig = new CXbmcConfiguration();
  if (!pXbmcHttpShim)
    pXbmcHttpShim = new CXbmcHttpShim();
  if (!m_pXbmcHttp)
    m_pXbmcHttp = new CXbmcHttp();
  m_port = 80;					/* Server port */
  m_szPassword[0] = '\0';

  m_hEvent = CreateEvent(NULL, true, false, NULL);
}


CWebServer::~CWebServer()
{
  CloseHandle(m_hEvent);
  if (pXbmcWeb) delete pXbmcWeb;
  if (pXbmcWebConfig) delete pXbmcWebConfig;
  if (pXbmcHttpShim)
  {
    delete pXbmcHttpShim;
    pXbmcHttpShim=NULL;
  }
  if (m_pXbmcHttp)
  {
    delete m_pXbmcHttp;
    m_pXbmcHttp=NULL;
  }
}
/*
DWORD CWebServer::SuspendThread()
{
  return ::SuspendThread(m_ThreadHandle);
}

DWORD CWebServer::ResumeThread()
{
  BOOL res=::ResumeThread(m_ThreadHandle);
  if (res) WaitForSingleObject(m_hEvent, INFINITE);
  return res;
}
*/
bool CWebServer::Start(const char *szLocalAddress, int port, const char_t* web, bool wait)
{
  m_bFinished = false;
  m_bStarted = false;
  ResetEvent(m_hEvent);

  strcpy(m_szLocalAddress, szLocalAddress);
  strcpy(m_szRootWeb, web);
  m_port = port;

  Create(false, THREAD_MINSTACKSIZE);
  if (m_ThreadHandle == NULL) return false;  

  CThread::SetName("Webserver");
  if( wait )
  {    
    // wait until the webserver is ready
    WaitForSingleObject(m_hEvent, INFINITE);
  }
  else
  {
    SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
  }

  return true;
}

void CWebServer::Stop()
{
  m_bFinished = true;
  
  StopThread();
}


//*************************************************************************************
void CWebServer::OnStartup()
{
  /*
   * Initialize the memory allocator. Allow use of malloc and start 
   * with a 60K heap.  For each page request approx 8KB is allocated.
   * 60KB allows for several concurrent page requests.  If more space
   * is required, malloc will be used for the overflow.
   */
  bopen(NULL, (60 * 1024), B_USE_MALLOC);

  // Initialize the web server, error is written to log by initWebs()
  if (initWebs() < 0) 
  {
    m_bFinished = true;
    SetEvent(m_hEvent);
    return;
  }

  m_bStarted = true;

  #ifdef WEBS_SSL_SUPPORT
    websSSLOpen();
  #endif

  // notify we are ready
  SetEvent(m_hEvent);
}

/*
 *	Initialize the web server.
 */
int CWebServer::initWebs()
{
	CLog::Log(LOGINFO, "WebServer: Server starting using %s on %s:%i\n", m_szRootWeb, m_szLocalAddress, m_port);
	
	// Initialize the socket subsystem
	if (socketOpen() == -1)
	{
	  CLog::Log(LOGERROR, "Unable to open socket for webserver");
	  return -1;
	}

  // Initialize the User Management database
	#ifdef USER_MANAGEMENT_SUPPORT
		umOpen();
		basicSetProductDir(m_szRootWeb);
		umRestore("umconfig.txt"); // only change this if done in the go-ahead source too
	#endif

  // set callbacks for spyce parser
	#ifdef SPYCE_SUPPORT
		setSpyOpenCallback(WEBS_SPYCE::spyceOpen);
		setSpyCloseCallback(WEBS_SPYCE::spyceClose);
		setSpyRequestCallback(WEBS_SPYCE::spyceRequest);
	#endif

  // Configure the web server options before opening the web server
	websSetIpaddr((char*)m_szLocalAddress);
	websSetHost((char*)m_szLocalAddress);
	websSetDefaultDir(m_szRootWeb);
	websSetDefaultPage(T("default.asp"));

	/* 
	 * Open the web server on the given port. If that port is taken, try
	 * the next sequential port for up to "retries" attempts.
	 */
	if(websOpenServer(m_port, 5 /* retries*/) < 0)
	{
	  CLog::Log(LOGERROR, "Unable to start webserver on port : %i", m_port);
	  return -1;
	}

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
	websFormDefine(T("xbmcForm"), XbmcWebsForm);
	websFormDefine(T("xbmcHttp"), XbmcHttpCommand);

	//Create the Form handlers for the User Management pages
	#ifdef USER_MANAGEMENT_SUPPORT
		formDefineUserMgmt();
	#endif

	// asp commands for xbmc
	websAspDefine(T("xbmcCommand"), XbmcWebsAspCommand);
  websAspDefine(T("xbmcAPI"), XbmcAPIAspCommand);

	// asp command for xbmc Configuration
	websAspDefine(T("xbmcCfgBookmarkSize"), XbmcWebsAspConfigBookmarkSize);
	websAspDefine(T("xbmcCfgGetBookmark"), XbmcWebsAspConfigGetBookmark);
	websAspDefine(T("xbmcCfgAddBookmark"), XbmcWebsAspConfigAddBookmark);
	websAspDefine(T("xbmcCfgSaveBookmark"), XbmcWebsAspConfigSaveBookmark);
	websAspDefine(T("xbmcCfgRemoveBookmark"), XbmcWebsAspConfigRemoveBookmark);
	websAspDefine(T("xbmcCfgSaveConfiguration"), XbmcWebsAspConfigSaveConfiguration);
	websAspDefine(T("xbmcCfgGetOption"), XbmcWebsAspConfigGetOption);
	websAspDefine(T("xbmcCfgSetOption"), XbmcWebsAspConfigSetOption);

	// Create a handler for the default home page
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 

  CLog::Log(LOGNOTICE, "Webserver: Started");
	return 0;
}


//*************************************************************************************
void CWebServer::OnExit()
{
	CLog::Log(LOGDEBUG, "WebServer:OnExit - Exit web server.\n");

	#ifdef WEBS_SSL_SUPPORT
		websSSLClose();
	#endif

  //Close the User Management database
	#ifdef USER_MANAGEMENT_SUPPORT
		umClose();
	#endif

  // Close the socket module
        if (m_bStarted)
        {
	  websCloseServer();
	  socketClose();
        }

	// Free up resources
	websDefaultClose();
	
	// close memory allocator
	bclose();

  // notify we are ready
	PulseEvent(m_hEvent);
}


//*************************************************************************************
void CWebServer::Process()
{
	/*
	 *	Basic event loop. SocketReady returns true when a socket is ready for
	 *	service. SocketSelect will block until an event occurs. SocketProcess
	 *	will actually do the servicing.
	 */
	int sockReady, sockSelect;

  /* set our thread priority */
  SetPriority(THREAD_PRIORITY_NORMAL);

	while (!m_bFinished) 
	{
		sockReady = socketReady(-1);
		// wait for event or timeout (default 20 msec)
		sockSelect = socketSelect(-1, SOCK_DFT_SVC_TIME);
		if (sockReady || sockSelect) {
			socketProcess(-1);
		}
		emfSchedProcess();
	}
	CLog::Log(LOGDEBUG, "WebServer:Exiting thread sockReady=%i, sockSelect=%i.\n", sockReady, sockSelect);
}

/*
 * Sets password for user "xbox"
 * this is done in group "sys_xbox".
 * Note that when setting the password this function will delete all database info!!
 */
void CWebServer::SetPassword(const char* strPassword)
{
  // wait until the webserver is ready
  if( WaitForSingleObject(m_hEvent, 5000) != WAIT_OBJECT_0 ) 
    return;

  // open the database and clean it
  int did = umOpen();
  dbZero(did);

  // save password in member var for later usage by GetPassword()
  if (strPassword) strcpy(m_szPassword, strPassword);
  
  // if password !NULL and greater then 0, enable user access
  if (strPassword && strlen(strPassword) > 0)
  {  
    // create group
    umAddGroup(WEBSERVER_UM_GROUP, PRIV_READ | PRIV_WRITE | PRIV_ADMIN, AM_BASIC, false, false);
    
    // greate user
    umAddUser("xbox", (char_t*)strPassword, WEBSERVER_UM_GROUP, false, false);
    
    // create access limit
    umAddAccessLimit("/", AM_BASIC, 0, WEBSERVER_UM_GROUP);
  }

  // save new information in database
  umCommit("umconfig.txt");
  umClose();
}

char* CWebServer::GetPassword()
{
  // wait until the webserver is ready
  if( WaitForSingleObject(m_hEvent, 5000) != WAIT_OBJECT_0 ) 
    return "";

  char* pPass = "";
  
  umOpen();
  if (umUserExists("xbox")) pPass = umGetUserPassword("xbox");
  
  umClose();
  
  return pPass;
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
	//bool redirected = false;
	char dir[1024];
	char files[][20] = {
			{"index.html"},
			{"index.htm"},
			{"home.htm"},
			{"home.html"},
			{"default.asp"},
			{"home.asp"},
			{'\0' }
                    };

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

/*
 *	Default error handler.  The developer should insert code to handle
 *	error messages in the desired manner.
 */
void defaultErrorHandler(int etype, char_t *msg)
{
	if (msg) CLog::Log(LOGERROR, msg);
}

/*
 *	Trace log. Customize this function to log trace output
 */
void defaultTraceHandler(int level, char_t *buf)
{
	if (buf) CLog::Log(LOGDEBUG, buf);
}

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
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

void  XbmcHttpCommand(webs_t wp, char_t *path, char_t *query) 
{																
	if (!pXbmcHttpShim) return ;
	return pXbmcHttpShim->xbmcForm(wp, path, query);	
}			

int XbmcAPIAspCommand(int eid, webs_t wp, int argc, char_t **argv) 
{																
	if (!pXbmcHttpShim) return -1;
	return pXbmcHttpShim->xbmcCommand(eid, wp, argc, argv);	
}		

int XbmcWebsAspCommand(int eid, webs_t wp, int argc, char_t **argv)
{
	if (!pXbmcWeb) return -1;
	return pXbmcWeb->xbmcCommand(eid, wp, argc, argv);
}

void XbmcWebsForm(webs_t wp, char_t *path, char_t *query)
{
	if (!pXbmcWeb) return;
	return pXbmcWeb->xbmcForm(wp, path, query);
}

/*
 * wrappers for xbmcConfig
 */
int XbmcWebsAspConfigBookmarkSize(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->BookmarkSize(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigGetBookmark(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->GetBookmark(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigAddBookmark(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->AddBookmark(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigSaveBookmark(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->SaveBookmark(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigRemoveBookmark(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->RemoveBookmark(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigSaveConfiguration(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->SaveConfiguration(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigGetOption(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->GetOption(eid, wp, response, argc, argv) : -1; }
int XbmcWebsAspConfigSetOption(int eid, webs_t wp, int argc, char_t **argv) { CStdString response; return pXbmcWebConfig ? pXbmcWebConfig->SetOption(eid, wp, response, argc, argv) : -1; }

/*
 * wrappers for HttpAPI xbmcConfig
 */
int XbmcWebsHttpAPIConfigBookmarkSize(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->BookmarkSize(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigGetBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->GetBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigAddBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->AddBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSaveBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SaveBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigRemoveBookmark(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->RemoveBookmark(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSaveConfiguration(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SaveConfiguration(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigGetOption(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->GetOption(-1, NULL, response, argc, argv) : -1; }
int XbmcWebsHttpAPIConfigSetOption(CStdString& response, int argc, char_t **argv) { return pXbmcWebConfig ? pXbmcWebConfig->SetOption(-1, NULL, response, argc, argv) : -1; }

  
#if defined(__cplusplus)
}
#endif 

