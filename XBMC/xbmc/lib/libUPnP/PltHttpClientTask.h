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
//#include "NptUtils.h"
#include "PltLog.h"
#include "NptSockets.h"
#include "PltHttp.h"
#include "PltThreadTask.h"

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask class
+---------------------------------------------------------------------*/
class PLT_HttpClientSocketTask : public PLT_ThreadTask
{
public:
    PLT_HttpClientSocketTask(NPT_Socket*       socket,
                             NPT_HttpRequest*  request);
    virtual ~PLT_HttpClientSocketTask();

    // PLT_ThreadTask methods
    virtual NPT_Result Abort() { 
        // abort first PLT_ThreadTask to set the aborted flag
        NPT_Result res = PLT_ThreadTask::Abort();

        // unblock socket if necessary
        m_Socket->Disconnect();
        return res;
    }

    virtual NPT_Result DoRun();

protected:

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
        PLT_HttpClientSocketTask(new NPT_TcpClientSocket, new NPT_HttpRequest(url, "GET")), m_Data(data) {}
    virtual ~PLT_HttpClientTask<T>() {}

    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response) {
        return m_Data->ProcessResponse(res,
                                       request,
                                       info,
                                       response);
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
        PLT_HttpClientSocketTask(new NPT_TcpClientSocket, new NPT_HttpRequest(url, "GET")) {}
    virtual ~PLT_FileHttpClientTask() {}

    NPT_Result ProcessResponse(NPT_Result        res, 
                               NPT_HttpRequest*  request, 
                               NPT_SocketInfo&   info, 
                               NPT_HttpResponse* response) {
        NPT_COMPILER_UNUSED(request);
        NPT_COMPILER_UNUSED(info);
        NPT_COMPILER_UNUSED(response);

        PLT_Log(PLT_LOG_LEVEL_3, "PLT_FileHttpClientTask::ProcessResponse (status=%d)\n", res);
        return NPT_SUCCESS;
    }

// members
private:
    // body?
};

#endif /* _PLT_HTTP_CLIENT_TASK_H_ */
