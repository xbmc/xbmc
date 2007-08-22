/*****************************************************************
|
|   Platinum - HTTP Client Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_HTTP_CLIENT_TASK_H_
#define _PLT_HTTP_CLIENT_TASK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttp.h"
#include "PltThreadTask.h"

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask class
+---------------------------------------------------------------------*/
class PLT_HttpClientSocketTask : public PLT_ThreadTask
{
friend class PLT_ThreadTask;

public:
    PLT_HttpClientSocketTask(NPT_Socket* socket, NPT_HttpRequest* request);

protected:
    virtual ~PLT_HttpClientSocketTask();

protected:
    // PLT_ThreadTask methods
    virtual void DoAbort() { m_Socket->Disconnect(); }
    virtual void DoRun();

    virtual NPT_Result ProcessResponse(NPT_Result        res, 
                                       NPT_HttpRequest*  request, 
                                       NPT_SocketInfo&   info, 
                                       NPT_HttpResponse* response);

protected:
    NPT_Socket*         m_Socket;
    NPT_HttpRequest*    m_Request;
};

/*----------------------------------------------------------------------
|   PLT_HttpClientTask class
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpClientTask : public PLT_HttpClientSocketTask
{
public:
    PLT_HttpClientTask<T>(const NPT_HttpUrl& url, T* data) : 
        PLT_HttpClientSocketTask(new NPT_TcpClientSocket, 
                                 new NPT_HttpRequest(url, "GET")), 
                                 m_Data(data) {}
 protected:
    virtual ~PLT_HttpClientTask<T>() {}

protected:
    // PLT_HttpClientSocketTask method
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response) {
        return m_Data->ProcessResponse(res, request, info, response);
    }

protected:
    T* m_Data;
};

/*----------------------------------------------------------------------
|   PLT_FileHttpClientTask class
+---------------------------------------------------------------------*/
class PLT_FileHttpClientTask : public PLT_HttpClientSocketTask
{
public:
    PLT_FileHttpClientTask(const NPT_HttpUrl& url) : 
        PLT_HttpClientSocketTask(new NPT_TcpClientSocket, 
                                 new NPT_HttpRequest(url, "GET")) {}

protected:
    virtual ~PLT_FileHttpClientTask() {}

protected:
    // PLT_HttpClientSocketTask method
    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response);
};

#endif /* _PLT_HTTP_CLIENT_TASK_H_ */
