/*****************************************************************
|
|   Platinum - HTTP Client Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltHttpClientTask.h"

NPT_SET_LOCAL_LOGGER("platinum.core.http.clienttask")

/*----------------------------------------------------------------------
|   PLT_HttpTcpConnector::Connect
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpTcpConnector::Connect(const char*                hostname, 
                              NPT_UInt16                 port, 
                              NPT_Timeout                connection_timeout,
                              NPT_Timeout                io_timeout,
                              NPT_Timeout                name_resolver_timeout,
                              NPT_InputStreamReference&  input_stream, 
                              NPT_OutputStreamReference& output_stream)
{
    if (m_HostName == hostname && m_Port == port) {
        input_stream  = m_InputStream;
        output_stream = m_OutputStream;
        return NPT_SUCCESS;
    }

    // get the address and port to which we need to connect
    NPT_IpAddress address;
    NPT_CHECK(address.ResolveName(hostname, name_resolver_timeout));

    // connect to the server
    NPT_LOG_FINE_2("NPT_HttpTcpConnector::Connect - will connect to %s:%d\n", hostname, port);
    m_Socket = new NPT_TcpClientSocket();
    m_Socket->SetReadTimeout(io_timeout);
    m_Socket->SetWriteTimeout(io_timeout);
    NPT_SocketAddress socket_address(address, port);
    NPT_CHECK(m_Socket->Connect(socket_address, connection_timeout));

    // get and keep the streams
    NPT_CHECK(m_Socket->GetInputStream(m_InputStream));
    NPT_CHECK(m_Socket->GetOutputStream(m_OutputStream));
    NPT_CHECK(m_Socket->GetInfo(m_SocketInfo));

    m_HostName    = hostname;
    m_Port        = port;
    input_stream  = m_InputStream;
    output_stream = m_OutputStream;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::PLT_HttpClientSocketTask(NPT_HttpRequest* request,
                                                   bool wait_forever /* = false */) :
    m_WaitForever(wait_forever),
    m_Connector(NULL)
{
    if (request) m_Requests.Push(request);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask()
{
    // delete any outstanding requests
    NPT_HttpRequest* request;
    while (NPT_SUCCEEDED(m_Requests.Pop(request, false))) {
        delete request;
    }
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::AddRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::AddRequest(NPT_HttpRequest* request)
{
    NPT_AutoLock lock(m_Requests);
    return m_Requests.Push(request);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::GetNextRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::GetNextRequest(NPT_HttpRequest*& request, NPT_Timeout timeout)
{
    NPT_AutoLock lock(m_Requests);
    return m_Requests.Pop(request, timeout);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_HttpClientSocketTask::DoRun()
{
    NPT_HttpClient        client(m_Connector = new PLT_HttpTcpConnector());
    NPT_HttpRequest*      request;
    NPT_SocketInfo        info;
    bool                  reuse_connector = true;

    do {
        // pop next request or wait for one for 100ms
        if (NPT_SUCCEEDED(GetNextRequest(request, 100))) {
            NPT_HttpResponse* response = NULL;
retry:
            if (IsAborting(0)) {
                delete request;
                return;
            }

            // if body is not seekable, don't even try to
            // reuse connector since in case it fails because
            // server closed connection, we won't be able to
            // rewind the body to resend the request
            if (!PLT_HttpHelper::IsBodyStreamSeekable(request)) {
                reuse_connector = false;
            }

            // create a new connector if necessary
            if (!reuse_connector) {
                client.SetConnector(m_Connector = new PLT_HttpTcpConnector());
            }

            // send request
            NPT_Result res = client.SendRequest(*request, response);

            // retry if we reused a previous connector
            if (NPT_FAILED(res) && reuse_connector) {
                reuse_connector = false;

                // server may have closed socket on us
                NPT_HttpEntity* entity = request->GetEntity();
                NPT_InputStreamReference input_stream;

                // rewind request body if any to be able to resend it
                if (entity && entity->GetInputStream(input_stream)) {
                    input_stream->Seek(0);
                }

                goto retry;
            }

            NPT_LOG_FINE("PLT_HttpClientSocketTask receiving:");
            PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);

            // callback to process response
            m_Connector->GetInfo(info);
            ProcessResponse(res, request, info, response);

            // server says connection close, force reopen next request
            if (response) reuse_connector = PLT_HttpHelper::IsConnectionKeepAlive(response);

            // cleanup
            delete response;
            delete request;
        }
    } while (m_WaitForever && !IsAborting(0));
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_HttpClientSocketTask::ProcessResponse(NPT_Result        res, 
                                          NPT_HttpRequest*  request, 
                                          NPT_SocketInfo&   info, 
                                          NPT_HttpResponse* response) 
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);

    NPT_LOG_FINE_1("PLT_HttpClientSocketTask::ProcessResponse (result=%d)", res);
    NPT_CHECK_SEVERE(res);

    NPT_HttpEntity* entity;
    NPT_InputStreamReference body;
    if (!response || 
        !(entity = response->GetEntity()) || 
        NPT_FAILED(entity->GetInputStream(body))) {
        return NPT_FAILURE;
    }

    // dump body into memory (if no content-length specified, read until disconnection)
    NPT_MemoryStream output;
    NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(*body, 
                                            output,
                                            0, 
                                            entity->GetContentLength()));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileHttpClientTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileHttpClientTask::ProcessResponse(NPT_Result        res, 
                                        NPT_HttpRequest*  request, 
                                        NPT_SocketInfo&   info, 
                                        NPT_HttpResponse* response) 
{
    NPT_COMPILER_UNUSED(res);
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);
    NPT_COMPILER_UNUSED(response);

    NPT_LOG_INFO_1("PLT_FileHttpClientTask::ProcessResponse (status=%d)\n", res);
    return NPT_SUCCESS;
}
