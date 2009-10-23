/*****************************************************************
|
|   Platinum - HTTP Server Tasks
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
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
|   external references
+---------------------------------------------------------------------*/
extern NPT_String HttpServerHeader;

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::PLT_HttpServerSocketTask
+---------------------------------------------------------------------*/
PLT_HttpServerSocketTask::PLT_HttpServerSocketTask(NPT_Socket* socket, 
                                                   bool        stay_alive_forever) :
    m_Socket(socket),
    m_StayAliveForever(stay_alive_forever)
{
    // needed for PS3 that is some case will request data every 35secs and 
    // won't like it if server disconnected too early
    socket->SetReadTimeout(60000);
    socket->SetWriteTimeout(60000);
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::~PLT_HttpServerSocketTask
+---------------------------------------------------------------------*/
PLT_HttpServerSocketTask::~PLT_HttpServerSocketTask() 
{
    if (m_Socket) delete m_Socket;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_HttpServerSocketTask::DoRun()
{
    NPT_BufferedInputStreamReference buffered_input_stream;
    NPT_HttpRequestContext           context;
    NPT_Result                       res = NPT_SUCCESS;
    bool                             headers_only;
    bool                             keep_alive = false;

    // create a buffered input stream 
    // to parse http request
    NPT_InputStreamReference input_stream;
    NPT_CHECK_LABEL_SEVERE(GetInputStream(input_stream), done);
    NPT_CHECK_POINTER_LABEL_FATAL(input_stream.AsPointer(), done);
    buffered_input_stream = new NPT_BufferedInputStream(input_stream);

    while (!IsAborting(0)) {
        NPT_HttpRequest*  request = NULL;
        NPT_HttpResponse* response = NULL;

        // reset keep-alive in case of failure to read
        keep_alive = false;

        // wait for a request
        res = Read(buffered_input_stream, request, &context);
        if (NPT_FAILED(res) || (request == NULL)) goto cleanup;

        // callback to process request and get back a response
        headers_only = false;
        res = ProcessRequest(*request, context, response, headers_only);
        if (NPT_FAILED(res) || (response == NULL)) goto cleanup;

        // send back response
        keep_alive = PLT_HttpHelper::IsConnectionKeepAlive(*request);
        res = Write(response, keep_alive, headers_only);

        // on write error, don't keep connection alive
        if (NPT_FAILED(res)) keep_alive = false;

cleanup:
        // cleanup
        delete request;
        delete response;

        if (!keep_alive && !m_StayAliveForever) {
            goto done;
        }
    }

done:
    delete m_Socket;
    m_Socket = NULL;
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
                               NPT_HttpRequestContext*           context) 
{
    NPT_SocketInfo info;
    GetInfo(info);

    if (context) {
        // extract socket info
        context->SetLocalAddress(info.local_address);
        context->SetRemoteAddress(info.remote_address);
    }

    // put back in buffered mode
    buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);

    // parse request
    NPT_Result res = NPT_HttpRequest::Parse(*buffered_input_stream, &info.local_address, request);
    if (NPT_FAILED(res) || !request) {
        // only log if not timeout
        res = NPT_FAILED(res)?res:NPT_FAILURE;
        if (res != NPT_ERROR_TIMEOUT && res != NPT_ERROR_EOS) NPT_CHECK_WARNING(res);
        return res;
    }

    // read socket info again to refresh the remote address in case it was a udp socket
    GetInfo(info);
    if (context) {
        context->SetLocalAddress(info.local_address);
        context->SetRemoteAddress(info.remote_address);
    }

    // return right away if no body is expected
    if (request->GetMethod() == NPT_HTTP_METHOD_GET || 
        request->GetMethod() == NPT_HTTP_METHOD_HEAD) 
        return NPT_SUCCESS;

    // create an entity
    NPT_HttpEntity* request_entity = new NPT_HttpEntity(request->GetHeaders());
    request->SetEntity(request_entity);

    NPT_MemoryStream* body_stream = new NPT_MemoryStream();
    request_entity->SetInputStream((NPT_InputStreamReference)body_stream);

    // unbuffer the stream
    buffered_input_stream->SetBufferSize(0);

    // check for chunked Transfer-Encoding
    if (request_entity->GetTransferEncoding() == "chunked") {
        NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(
            *NPT_InputStreamReference(new NPT_HttpChunkedInputStream(buffered_input_stream)).AsPointer(), 
            *body_stream));

        request_entity->SetTransferEncoding(NULL);
    } else if (request_entity->GetContentLength()) {
        // a request with a body must always have a content length if not chunked
        NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(
            *buffered_input_stream.AsPointer(), 
            *body_stream, 
            0, 
            request_entity->GetContentLength()));
    } else {
        request->SetEntity(NULL);
    }

    // rebuffer the stream
    buffered_input_stream->SetBufferSize(NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpServerSocketTask::Write
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpServerSocketTask::Write(NPT_HttpResponse* response, 
                                bool&             keep_alive, 
                                bool              headers_only /* = false */) 
{
    // add any headers that may be missing
    NPT_HttpHeaders& headers = response->GetHeaders();
    const NPT_String* value  = headers.GetHeaderValue(NPT_HTTP_HEADER_CONNECTION);
    if (value) {
        // override keep_alive header if value passed not ok
        if (!keep_alive) {
            headers.SetHeader(NPT_HTTP_HEADER_CONNECTION,  "close"); // override
        } else {
            keep_alive =  value->Compare("keep-alive") == 0; // keep-alive ok but return what headers say
        }
    } else {
        // FIXME: we really should put Connection: keep-alive header if request was 1.0 only ?
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, keep_alive?"keep-alive":"close");
    }

    // set user agent
    headers.SetHeader(NPT_HTTP_HEADER_SERVER, 
                      NPT_HttpServer::m_ServerHeader, false); // set but don't replace
                      
    // get the response entity to set additional headers
    NPT_HttpEntity* entity = response->GetEntity();
    
    NPT_InputStreamReference body_stream;
    if (entity && 
        NPT_SUCCEEDED(entity->GetInputStream(body_stream)) && 
        !body_stream.IsNull()) {
        if (entity->HasContentLength()) {
            // content length
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, 
                NPT_String::FromIntegerU(entity->GetContentLength()));
        }

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
    }

    NPT_LOG_FINER("PLT_HttpServerTask Sending response:");
    PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

    // get the socket stream to send the request
    NPT_OutputStreamReference output_stream;
    NPT_CHECK_WARNING(m_Socket->GetOutputStream(output_stream));

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;
    
    // emit the response headers into the header buffer
    response->Emit(header_stream);

    // send the headers
    NPT_CHECK_WARNING(output_stream->WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send response body if any
    if (!headers_only && !body_stream.IsNull()) {
        NPT_CHECK_WARNING(NPT_StreamToStreamCopy(
            *body_stream.AsPointer(), 
            *output_stream.AsPointer(),
            0,
            entity->GetContentLength()));
    }

    // flush the output stream so that everything is sent to the server
    output_stream->Flush();

    return NPT_SUCCESS;
}

