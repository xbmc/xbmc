
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

CWebServer::CWebServer()
{
  m_pHttp = NULL;
  m_pServer = NULL;
  m_pHost = NULL;
  m_pMpr = NULL;
  m_port = WEBSERVER_PORT;					/* Server port */
  m_szPassword[0] = '\0';
  m_szLocalAddress[0] = '\0';
  m_szRootWeb[0] = '\0';
  m_hEvent = CreateEvent(NULL, true, false, NULL);
}

CWebServer::~CWebServer()
{
  Stop();
  CloseHandle(m_hEvent);
}

bool CWebServer::Start(const char *szLocalAddress, int port, 
                       const char* docroot, bool wait)
{
  m_bFinished = false;
  m_bStarted = false;
  ResetEvent(m_hEvent);

  strcpy(m_szLocalAddress, szLocalAddress);
  strncpy(m_szRootWeb, docroot, PATH_MAX);
  m_port = port;

  Create(false, THREAD_MINSTACKSIZE);
  if (m_ThreadHandle == NULL) 
    return false;  

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
	CLog::Log(LOGDEBUG, "WebServer:OnStartup - Starting web server.\n");
  m_pMpr = new Mpr("xbmcESP");
  m_pMpr->start(MPR_SERVICE_THREAD);
  //m_pMpr->poolService->setMaxPoolThreads(10);

  m_pHttp = new MaHttp;
  m_pServer = new MaServer(m_pHttp, "xbmcWebServer", ".", 
                           m_szLocalAddress, m_port);
  m_pHost = m_pServer->newHost(".");
  new MaCopyModule(NULL);
  m_pHost->addHandler("copyHandler", "");
  m_pHost->insertAlias(new MaAlias("/", "index.html", 404));

  if (!m_pServer || m_pServer->start() < 0)
    CLog::Log(LOGDEBUG, "WebServer::OnStartup - Failed to start server");
  else
    m_bStarted = true;
  // notify we are ready
  SetEvent(m_hEvent);
}

void CWebServer::OnExit()
{
	CLog::Log(LOGDEBUG, "WebServer:OnExit - Exit web server.\n");
  
  m_pHttp->stop();
  delete m_pServer;
  m_pServer = NULL;
  delete m_pHttp;
  m_pHttp = NULL;
  delete m_pMpr;
  m_pMpr = NULL;
  
  // notify we are ready
	PulseEvent(m_hEvent);
}


//*************************************************************************************
void CWebServer::Process()
{
  /* set our thread priority */
  SetPriority(THREAD_PRIORITY_NORMAL);

	while (!m_bFinished) 
	{
    m_pMpr->serviceEvents(false, 5000); // millis?
	}
}

/*
 * Sets password for user "xbox"
 * this is done in group "sys_xbox".
 * Note that when setting the password this function will delete all database info!!
 */
#if 0
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
    umAddGroup((char*)WEBSERVER_UM_GROUP, PRIV_READ | PRIV_WRITE | PRIV_ADMIN, AM_BASIC, false, false);
    
    // greate user
    umAddUser((char*)"xbox", (char_t*)strPassword, (char*)WEBSERVER_UM_GROUP, false, false);
    
    // create access limit
    umAddAccessLimit((char*)"/", AM_BASIC, 0, (char*)WEBSERVER_UM_GROUP);
  }

  // save new information in database
  umCommit((char*)"umconfig.txt");
  umClose();
}

char* CWebServer::GetPassword()
{
  // wait until the webserver is ready
  if( WaitForSingleObject(m_hEvent, 5000) != WAIT_OBJECT_0 ) 
    return (char*)"";

  char* pPass = (char*)"";
  
  umOpen();
  if (umUserExists((char*)"xbox")) pPass = umGetUserPassword((char*)"xbox");
  
  umClose();
  
  return pPass;
}
#endif // 0

