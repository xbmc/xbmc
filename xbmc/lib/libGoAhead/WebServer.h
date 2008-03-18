
/* WebServer.h: interface for the CWebServer class.
 * A darivation of:  main.c -- Main program for the GoAhead WebServer
 *
 * main.c -- Main program for the GoAhead WebServer
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef char char_t;
typedef struct websRec *webs_t;

int	  aspTest(int eid, webs_t wp, int argc, char_t **argv);
void  formTest(webs_t wp, char_t *path, char_t *query);
int	  XbmcWebsAspCommand(int eid, webs_t wp, int argc, char_t **argv);
void  XbmcWebsForm(webs_t wp, char_t *path, char_t *query);
void  XbmcHttpCommand(webs_t wp, char_t *path, char_t *query);
int	  XbmcAPIAspCommand(int eid, webs_t wp, int argc, char_t **argv);
bool  XbmcWebConfigInit();
void  XbmcWebConfigRelease();

// wrapers for XBMCConfiguration
int XbmcWebsAspConfigBookmarkSize(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigGetBookmark(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigAddBookmark(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigSaveBookmark(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigRemoveBookmark(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigSaveConfiguration(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigGetOption(int eid, webs_t wp, int argc, char_t **argv);
int XbmcWebsAspConfigSetOption(int eid, webs_t wp, int argc, char_t **argv);

// wrapers for HttpAPI XBMCConfiguration
int XbmcWebsHttpAPIConfigBookmarkSize(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigGetBookmark(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigAddBookmark(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigSaveBookmark(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigRemoveBookmark(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigSaveConfiguration(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigGetOption(CStdString& response, int argc, char_t **argv);
int XbmcWebsHttpAPIConfigSetOption(CStdString& response, int argc, char_t **argv);

#if defined(__cplusplus)
}
#endif 

// group for default xbox user
#define WEBSERVER_UM_GROUP "sys_xbox"

class CWebServer : public CThread
{
public:

	CWebServer();
	virtual ~CWebServer();
	bool						Start(const char* szLocalAddress, int port = 80, const char* web = "Q:\\web", bool wait = true);
	void						Stop();

	DWORD						SuspendThread();
	DWORD						ResumeThread();

	void						SetPassword(const char* strPassword);
	char*           GetPassword();

protected:

  virtual void		OnStartup();
  virtual void		OnExit();
  virtual void		Process();
	
  int							initWebs();
	
	char            m_szLocalAddress[128];		/* local ip address */
	char            m_szRootWeb[1024];	/* local directory */
	char            m_szPassword[128];	/* password */
	int							m_port;							/* Server port */
	bool						m_bFinished;				/* Finished flag */
	bool						m_bStarted;				/* Started flag */
	HANDLE					m_hEvent;
};
