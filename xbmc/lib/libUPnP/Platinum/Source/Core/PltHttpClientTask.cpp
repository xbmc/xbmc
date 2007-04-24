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

    PLT_Log(PLT_LOG_LEVEL_4, "PLT_HttpClientSocketTask sending:\n");
    PLT_HttpHelper::ToLog(m_Request, PLT_LOG_LEVEL_3);

    res = client.SendRequest(output_stream, *m_Request);
    if (NPT_FAILED(res)) goto cleanup;

    // get the input stream to read the response
    res = m_Socket->GetInputStream(input_stream);
    if (NPT_FAILED(res)) goto cleanup;

    res = client.WaitForResponse(input_stream, *m_Request, info, response);
    if (NPT_FAILED(res)) goto cleanup;

    res = m_Socket->GetInfo(info);
    if (NPT_FAILED(res)) goto cleanup;

    PLT_Log(PLT_LOG_LEVEL_4, "PLT_HttpClientSocketTask receiving:\n");
    PLT_HttpHelper::ToLog(response, PLT_LOG_LEVEL_3);


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

    PLT_Log(PLT_LOG_LEVEL_3, "PLT_HttpClientSocketTask::ProcessResponse (result=%d)\n", res);
    NPT_CHECK(res);

    NPT_HttpEntity* entity;
    NPT_InputStreamReference body;
    if (!response || !(entity = response->GetEntity()) || NPT_FAILED(entity->GetInputStream(body))) {
        return NPT_FAILURE;
    }

    // dump body into memory (if no content-length specified, read until disconnection)
    NPT_MemoryStream output;
    NPT_CHECK(NPT_StreamToStreamCopy(*body, output, 0, entity->GetContentLength()));

    return NPT_SUCCESS;
}
//
///*----------------------------------------------------------------------
//|   PLT_HttpClientSocketTask::AddRequest
//+---------------------------------------------------------------------*/
//NPT_Result
//PLT_HttpClientSocketTask::AddRequest(NPT_HttpRequest*           request, 
//                                     bool                       buffer_response_body,
//                                     PLT_HttpContextReference*  context_result /* = NULL */)
//{
//    //PLT_HttpHelper::SetHost(request, m_Host + ":" + NPT_String::FromInteger(m_RemoteAddr.GetPort()));
//    PLT_HttpContextReference context(new PLT_HttpContext(request, buffer_response_body));
//
//    NPT_String protocol = context->m_Request->GetProtocol();
//    if (m_KeepAlive) {
//        if (protocol.Compare("HTTP/1.0") == 0) {
//            // in HTTP 1.0, we need to explicitly tell the server to keep
//            // the connection alive
//            context->m_Request->GetHeaders().SetHeader("Connection", "Keep-Alive");
//        }
//    } else {
//        // in HTTP 1.1, we need to explicitly tell the server to close
//        // the connection when done
//        // Just to be sure, do it even for HTTP 1.0
//        context->m_Request->GetHeaders().SetHeader("Connection", "Close");
//    }
//
//    // if the request was passed with a body, set the Content-Length
//    // if not already set
//    NPT_Size len;
//    if (NPT_FAILED(PLT_HttpHelper::GetContentLength(context->m_Request, len))) {
//        NPT_InputStreamReference stream;
//        NPT_HttpEntity* entity = context->m_Request->GetEntity();
//        if (entity) {
//            if (NPT_SUCCEEDED(entity->GetInputStream(stream)) && !stream.IsNull()) {
//                if (NPT_SUCCEEDED(stream->GetSize(len))) {
//                    PLT_HttpHelper::SetContentLength(context->m_Request, len);
//                }
//            }
//        }
//    }
//
//    m_Contexts.Add(context);
//    if (context_result) {
//        *context_result = context;
//    }
//    return NPT_SUCCESS;
//}

