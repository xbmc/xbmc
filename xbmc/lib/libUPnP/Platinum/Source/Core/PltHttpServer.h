/*****************************************************************
|
|   Platinum - HTTP Server
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_HTTP_SERVER_H_
#define _PLT_HTTP_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttpServerListener.h"
#include "PltHttpServerTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServerStartIterator;

/*----------------------------------------------------------------------
|   PLT_HttpServer class
+---------------------------------------------------------------------*/
class PLT_HttpServer : public PLT_HttpServerListener,
                       public NPT_HttpServer
{
    friend class PLT_HttpServerTask<class PLT_HttpServer>;
    friend class PLT_HttpServerStartIterator;

public:
    PLT_HttpServer(unsigned int port = 0,
                   NPT_Cardinal max_clients = 0,
                   bool         reuse_address = false);
    virtual ~PLT_HttpServer();

    // PLT_HttpServerListener method
    NPT_Result ProcessHttpRequest(NPT_HttpRequest&   request, 
                                  NPT_SocketInfo     info, 
                                  NPT_HttpResponse*& response,
                                  bool&              headers_only);

    virtual NPT_Result   Start();
    virtual NPT_Result   Stop();
    virtual unsigned int GetPort() { return m_Port; }

private:
    PLT_TaskManager*          m_TaskManager;
    unsigned int              m_Port;
    bool                      m_ReuseAddress;
    PLT_HttpServerListenTask* m_HttpListenTask;
};

/*----------------------------------------------------------------------
|   PLT_FileServer class
+---------------------------------------------------------------------*/
class PLT_FileServer
{
public:
    // class methods
    static NPT_Result ServeFile(NPT_String        filename, 
                                NPT_HttpResponse* response, 
                                NPT_Integer       start = -1, 
                                NPT_Integer       end = -1,
                                bool              request_is_head = false);
};

#endif /* _PLT_HTTP_SERVER_H_ */
