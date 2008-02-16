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
|   PLT_HttpTcpConnector
+---------------------------------------------------------------------*/
class PLT_HttpTcpConnector : public NPT_HttpClient::Connector
{
    virtual NPT_Result Connect(const char*                   hostname, 
                               NPT_UInt16                    port, 
                               NPT_Timeout                   connection_timeout,
                               NPT_Timeout                   io_timeout,
                               NPT_Timeout                   name_resolver_timeout,
                               NPT_InputStreamReference&     input_stream, 
                               NPT_OutputStreamReference&    output_stream);

public:
    void    GetInfo(NPT_SocketInfo& info) { info = m_SocketInfo;}
    void    Abort() { if (!m_Socket.IsNull()) m_Socket->Disconnect(); }

private:
    NPT_String                 m_HostName;
    NPT_UInt16                 m_Port;
    NPT_InputStreamReference   m_InputStream;
    NPT_OutputStreamReference  m_OutputStream;
    NPT_SocketInfo             m_SocketInfo;
    NPT_Reference<NPT_Socket>  m_Socket;
};

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask class
+---------------------------------------------------------------------*/
class PLT_HttpClientSocketTask : public PLT_ThreadTask
{
friend class PLT_ThreadTask;

public:
    PLT_HttpClientSocketTask(NPT_HttpRequest* request = NULL, bool wait_forever = false);
    NPT_Result AddRequest(NPT_HttpRequest* request);

protected:
    virtual ~PLT_HttpClientSocketTask();

protected:
    // PLT_ThreadTask methods
    virtual void DoAbort() { if (m_Connector) m_Connector->Abort(); }
    virtual void DoRun();

    virtual NPT_Result ProcessResponse(NPT_Result        res, 
                                       NPT_HttpRequest*  request, 
                                       NPT_SocketInfo&   info,
                                       NPT_HttpResponse* response);

private:
    NPT_Result GetNextRequest(NPT_HttpRequest*& request, NPT_Timeout timeout);

protected:
    bool                                    m_WaitForever;
    NPT_Lock<NPT_Queue<NPT_HttpRequest> >   m_Requests;
    PLT_HttpTcpConnector*                   m_Connector; //TBD: we need a lock to be able to abort
};

/*----------------------------------------------------------------------
|   PLT_HttpClientTask class
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpClientTask : public PLT_HttpClientSocketTask
{
public:
    PLT_HttpClientTask<T>(const NPT_HttpUrl& url, T* data) : 
        PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET")), 
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
        PLT_HttpClientSocketTask(new NPT_HttpRequest(url, "GET")) {}

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
