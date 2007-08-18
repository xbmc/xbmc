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
|   PLT_HttpClientSocketTask::PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::PLT_HttpClientSocketTask(NPT_Socket*       socket,
                                                   NPT_HttpRequest*  request) :
    m_Socket(socket),
    m_Request(request)
{
    m_Socket->SetReadTimeout(30000);
    m_Socket->SetWriteTimeout(30000);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask()
{
    delete m_Socket;
    delete m_Request;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_HttpClientSocketTask::DoRun()
{
    NPT_HttpResponse*         response = NULL;
    NPT_SocketInfo            info;
    NPT_Result                res;
    PLT_HttpClient            client;
    NPT_OutputStreamReference output_stream;
    NPT_InputStreamReference  input_stream;

    // connect socket
    res = client.Connect(m_Socket, *m_Request);
    if (NPT_FAILED(res)) goto cleanup;

    // get the connection stream to send the request
    res = m_Socket->GetOutputStream(output_stream);
    if (NPT_FAILED(res)) goto cleanup;

    NPT_LOG_FINER("PLT_HttpClientSocketTask sending:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, m_Request);

    res = client.SendRequest(output_stream, *m_Request);
    if (NPT_FAILED(res)) goto cleanup;

    // get the input stream to read the response
    res = m_Socket->GetInputStream(input_stream);
    if (NPT_FAILED(res)) goto cleanup;

    res = client.WaitForResponse(input_stream, *m_Request, info, response);
    if (NPT_FAILED(res)) goto cleanup;

    res = m_Socket->GetInfo(info);
    if (NPT_FAILED(res)) goto cleanup;

    NPT_LOG_FINE("PLT_HttpClientSocketTask receiving:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);

cleanup:
    // callback to process response
    ProcessResponse(res, m_Request, info, response);

    // cleanup
    delete response;
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
    if (!response || !(entity = response->GetEntity()) || NPT_FAILED(entity->GetInputStream(body))) {
        return NPT_FAILURE;
    }

    // dump body into memory (if no content-length specified, read until disconnection)
    NPT_MemoryStream output;
    NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(*body, output, 0, entity->GetContentLength()));

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
