/*****************************************************************
|
|   Platinum - HTTP Server Tasks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltHttpServerTask.h"
#include "PltHttp.h"
#include "PltVersion.h"

NPT_SET_LOCAL_LOGGER("platinum.core.http.servertask")

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::PLT_HttpServerSocketTask
+---------------------------------------------------------------------*/
PLT_HttpServerSocketTask::PLT_HttpServerSocketTask(NPT_Socket* socket, 
                                                   bool        stay_alive_forever) :
    m_Socket(socket),
    m_StayAliveForever(stay_alive_forever)
{
    socket->SetReadTimeout(30000);
    socket->SetWriteTimeout(30000);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::~PLT_HttpServerSocketTask
+---------------------------------------------------------------------*/
PLT_HttpServerSocketTask::~PLT_HttpServerSocketTask() 
{
    delete m_Socket;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_HttpServerSocketTask::DoRun()
{
    NPT_BufferedInputStreamReference buffered_input_stream;
    NPT_SocketInfo info;
    NPT_Result     res = NPT_SUCCESS;
    bool           headers_only;

    // create a buffered input stream to parse http request
    // as it comes
    NPT_InputStreamReference input_stream;
    NPT_CHECK_LABEL_SEVERE(GetInputStream(input_stream), done);
    buffered_input_stream = new NPT_BufferedInputStream(input_stream);

    while (!IsAborting(0)) {
        NPT_HttpRequest*  request = NULL;
        NPT_HttpResponse* response = NULL;

        while (!IsAborting(0)) {
            // wait for a request
            res = Read(buffered_input_stream, request, info);
            if (NPT_FAILED(res) || (request == NULL)) break;

            // callback to process request and get back a response
            headers_only = false;
            res = ProcessRequest(*request, info, response, headers_only);
            if (NPT_FAILED(res) || (response == NULL)) break;

            // send back response
            res = Write(response, false, headers_only);
            if (NPT_FAILED(res)) break;

            break;
        }

        // cleanup
        delete request;
        delete response;

        if (!m_StayAliveForever) {
            // if we were to support persistent connections 
            // we would stop only if res would be a failure
            // (like a timeout or a read/write error)
            goto done;
        }
    }

done:
    return;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::GetInputStream(NPT_InputStreamReference& stream)
{
    return m_Socket->GetInputStream(stream);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::GetInfo(NPT_SocketInfo& info)
{
    return m_Socket->GetInfo(info);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::Read
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::Read(NPT_BufferedInputStreamReference& buffered_input_stream, 
                               NPT_HttpRequest*&                 request,
                               NPT_SocketInfo&                   info) 
{
    // extract socket info
    GetInfo(info);

    // parse request
    NPT_CHECK_FINE(NPT_HttpRequest::Parse(*buffered_input_stream, &info.local_address, request));
    if (!request) return NPT_FAILURE;

    // read socket info again to refresh the remote address in case it was a udp socket
    GetInfo(info);

    // create an entity
    NPT_HttpEntity* request_entity = new NPT_HttpEntity(request->GetHeaders());
    request->SetEntity(request_entity);

    // buffer body now if any
    if (request_entity->GetContentLength() > 0) {
        // unbuffer the stream
        buffered_input_stream->SetBufferSize(0);

        NPT_MemoryStream* body_stream = new NPT_MemoryStream();
        NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(
            *buffered_input_stream.AsPointer(), 
            *body_stream, 
            0, 
            request_entity->GetContentLength()));

        // set entity body
        request_entity->SetInputStream((NPT_InputStreamReference)body_stream);

        // rebuffer the stream
        buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::Write
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::Write(NPT_HttpResponse* response, 
                                bool              keep_alive, 
                                bool              headers_only /* = false */) 
{
    // add any headers that may be missing
    NPT_HttpHeaders& headers = response->GetHeaders();
    if (keep_alive) {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "keep-alive");
    } else {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    }

    // set user agent
    if (!headers.GetHeader(NPT_HTTP_HEADER_USER_AGENT)) {
        headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
            "Platinum/" PLT_PLATINUM_VERSION_STRING);
    }

    // get the response entity to set additional headers
    NPT_HttpEntity* entity = response->GetEntity();
    if (entity) {
        // content length
        headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, 
            NPT_String::FromInteger(entity->GetContentLength()));

        // content type
        NPT_String content_type = entity->GetContentType();
        if (!content_type.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_TYPE, content_type);
        }

        // content encoding
        NPT_String content_encoding = entity->GetContentEncoding();
        if (!content_encoding.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_ENCODING, content_encoding);
        }
    } else {
        // force content length to 0 is there is no message body
        headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, "0");
    }

    NPT_LOG_FINE("PLT_HttpServerTask Sending response:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINE, response);

    // get the socket stream to send the request
    NPT_OutputStreamReference output_stream;
    NPT_CHECK_SEVERE(m_Socket->GetOutputStream(output_stream));

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;
    
    // emit the response headers into the header buffer
    response->Emit(header_stream);

    // send the headers
    NPT_CHECK_SEVERE(output_stream->WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send response body if any
    if (!headers_only && entity) {
        NPT_InputStreamReference body_stream;
        entity->GetInputStream(body_stream);

        if (!body_stream.IsNull()) {
            NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(
                *body_stream.AsPointer(), 
                *output_stream.AsPointer(),
                0,
                entity->GetContentLength()));
        }
    }

    // flush the output stream so that everything is sent to the server
    output_stream->Flush();

    return NPT_SUCCESS;
}

