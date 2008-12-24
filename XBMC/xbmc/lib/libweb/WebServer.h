#ifndef WEBSERVER_H__
#define WEBSERVER_H__

#include "utils/Thread.h"
#include "appweb.h"

#define WEBSERVER_GROUP     "xbmc"
#define WEBSERVER_USER      "xbmc"
#define WEBSERVER_PORT      8080
#define WEBSERVER_DOC_ROOT  "Q:\\web"

//static CXbmcWeb* pXbmcWeb;
//static CXbmcConfiguration* pXbmcWebConfig;

class CWebServer : public CThread
{
public:

	CWebServer();
	virtual ~CWebServer();
	bool						Start(const char *szLocalAddress, 
                        int port = WEBSERVER_PORT, 
                        const char *docroot = WEBSERVER_DOC_ROOT, 
                        bool wait = true);
	void						Stop();

	void						SetPassword(const char *strPassword);
	char            *GetPassword();

protected:

  virtual void		OnStartup();
  virtual void		OnExit();
  virtual void		Process();
  
  MaHttp          *m_pHttp;
  MaServer        *m_pServer;
  MaHost          *m_pHost;
  Mpr             *m_pMpr;
	char            m_szLocalAddress[128];		/* local ip address */
	char            m_szRootWeb[PATH_MAX];    /* local directory */
	char            m_szPassword[128];	      /* password */
	int							m_port;							      /* Server port */
	bool						m_bFinished;				      /* Finished flag */
	bool						m_bStarted;				        /* Started flag */
	HANDLE					m_hEvent;
};

#endif // WEBSERVER_H__

