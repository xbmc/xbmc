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
#include "NptVersion.h"
#include "NptHttp.h"
#include "PltLog.h"
#include "PltHttpServerTask.h"
#include "PltHttp.h"
#include "PltVersion.h"

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
    NPT_SocketInfo info;
    NPT_Result     res = NPT_SUCCESS;
    NPT_BufferedInputStreamReference buffered_input_stream;

    // create a buffered input stream to parse http request
    // as it comes
    NPT_InputStreamReference input_stream;
    NPT_CHECK_LABEL(GetInputStream(input_stream), done);
    buffered_input_stream = new NPT_BufferedInputStream(input_stream);

    while (!IsAborting(0)) {
        NPT_HttpRequest*  request = NULL;
        NPT_HttpResponse* response = NULL;

        while (!IsAborting(0)) {
            // wait for a request
            res = Read(buffered_input_stream, request, info);
            if (NPT_FAILED(res) || (request == NULL)) break;

            // callback to process request and get back a response
            res = ProcessRequest(request, info, response);
            if (NPT_FAILED(res) || (response == NULL)) break;

            // send back response
            res = Write(response, false);
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
    // FIXME: we should not pass NULL for the socketaddress which is used if no HOST is found in headers
    NPT_CHECK(NPT_HttpRequest::Parse(*buffered_input_stream, NULL, request));
    if (!request) return NPT_FAILURE;

    // create an entity if one is expected in the response
    NPT_HttpHeaders& headers = request->GetHeaders();
    NPT_HttpHeader* header = headers.GetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH);
    unsigned long content_length;
    if (header && NPT_SUCCEEDED(header->GetValue().ToInteger(content_length)) && content_length) {
    //if (request->GetMethod() == NPT_HTTP_METHOD_POST) {
        NPT_HttpEntity* request_entity = new NPT_HttpEntity(headers);
        request->SetEntity(request_entity);

        // read body if any
        if (request_entity->GetContentLength() > 0) {
            // unbuffer the stream
            buffered_input_stream->SetBufferSize(0);

            NPT_MemoryStream* body_stream = new NPT_MemoryStream();
            NPT_CHECK(NPT_StreamToStreamCopy(
                *buffered_input_stream.AsPointer(), 
                *body_stream, 
                0, 
                request_entity->GetContentLength()));

            // set entity body
            request_entity->SetInputStream((NPT_InputStreamReference)body_stream);

            // rebuffer the stream
            buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);
        }
    }

    // extract socket info
    return GetInfo(info);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::Write
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::Write(NPT_HttpResponse* response, bool keep_alive) 
{
    // add any headers that may be missing
    NPT_HttpHeaders& headers = response->GetHeaders();
    if (keep_alive) {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "keep-alive");
    } else {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    }
    if (!headers.GetHeader(NPT_HTTP_HEADER_USER_AGENT)) {
        headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
            "Platinum/" PLT_PLATINUM_VERSION_STRING);
    }

    // get the response entity to set additional headers
    NPT_InputStreamReference body_stream;
    NPT_HttpEntity* entity = response->GetEntity();
    if (entity && NPT_SUCCEEDED(entity->GetInputStream(body_stream))) {
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

    PLT_Log(PLT_LOG_LEVEL_4, "PLT_HttpServerTask Sending response:\r\n");
    PLT_HttpHelper::ToLog(response, PLT_LOG_LEVEL_4);

    // get the socket stream to send the request
    NPT_OutputStreamReference output_stream;
    NPT_CHECK(m_Socket->GetOutputStream(output_stream));

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;

    // emit the response headers into the header buffer
    response->Emit(header_stream);

    // send the headers
    NPT_CHECK(output_stream->WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send response body
    if (!body_stream.IsNull()) {
        NPT_CHECK(NPT_StreamToStreamCopy(*body_stream.AsPointer(), 
            *output_stream.AsPointer(),
            0,
            entity->GetContentLength()));
    }

    // flush the output stream so that everything is sent to the server
    output_stream->Flush();

    return NPT_SUCCESS;
}

