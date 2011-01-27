/*****************************************************************
|
|   Neptune - HTTP Protocol
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptHttp.h"
#include "NptSockets.h"
#include "NptBufferedStreams.h"
#include "NptDebug.h"
#include "NptVersion.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "NptLogging.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.http")

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
NPT_String NPT_HttpClient::m_UserAgentHeader = "Neptune/" NPT_NEPTUNE_VERSION_STRING;
NPT_String NPT_HttpServer::m_ServerHeader    = "Neptune/" NPT_NEPTUNE_VERSION_STRING;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_HTTP_MAX_REDIRECTS = 20;
const char* const NPT_HTTP_DEFAULT_403_HTML = "<html><head><title>403 Forbidden</title></head><body><h1>Forbidden</h1><p>Access to this URL is forbidden.</p></html>";
const char* const NPT_HTTP_DEFAULT_404_HTML = "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></html>";
const char* const NPT_HTTP_DEFAULT_500_HTML = "<html><head><title>500 Internal Error</title></head><body><h1>Internal Error</h1><p>The server encountered an unexpected condition which prevented it from fulfilling the request.</p></html>";

/*----------------------------------------------------------------------
|   NPT_HttpUrl::NPT_HttpUrl
+---------------------------------------------------------------------*/
NPT_HttpUrl::NPT_HttpUrl(const char* url, bool ignore_scheme) :
    NPT_Url(url, ignore_scheme?SCHEME_ID_UNKNOWN:SCHEME_ID_HTTP, NPT_HTTP_DEFAULT_PORT)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpUrl::NPT_HttpUrl
+---------------------------------------------------------------------*/
NPT_HttpUrl::NPT_HttpUrl(const char* host, 
                         NPT_UInt16  port, 
                         const char* path,
                         const char* query,
                         const char* fragment) :
    NPT_Url("http", host, port, path, query, fragment)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpUrl::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_HttpUrl::ToString(bool with_fragment) const
{
    return NPT_Url::ToStringWithDefaultPort(NPT_HTTP_DEFAULT_PORT, with_fragment);
}

/*----------------------------------------------------------------------
|   NPT_HttpHeader::NPT_HttpHeader
+---------------------------------------------------------------------*/
NPT_HttpHeader::NPT_HttpHeader(const char* name, const char* value):
    m_Name(name), 
    m_Value(value)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpHeader::~NPT_HttpHeader
+---------------------------------------------------------------------*/
NPT_HttpHeader::~NPT_HttpHeader()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpHeader::Emit
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeader::Emit(NPT_OutputStream& stream) const
{
    stream.WriteString(m_Name);
    stream.WriteFully(": ", 2);
    stream.WriteString(m_Value);
    stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);
    NPT_LOG_FINEST_2("header %s: %s", m_Name.GetChars(), m_Value.GetChars());
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeader::SetName
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpHeader::SetName(const char* name)
{
    m_Name = name;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeader::~NPT_HttpHeader
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpHeader::SetValue(const char* value)
{
    m_Value = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::NPT_HttpHeaders
+---------------------------------------------------------------------*/
NPT_HttpHeaders::NPT_HttpHeaders()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::~NPT_HttpHeaders
+---------------------------------------------------------------------*/
NPT_HttpHeaders::~NPT_HttpHeaders()
{
    m_Headers.Apply(NPT_ObjectDeleter<NPT_HttpHeader>());
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::Parse
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpHeaders::Parse(NPT_BufferedInputStream& stream)
{
    NPT_String header_name;
    NPT_String header_value;
    bool       header_pending = false;
    NPT_String line;

    while (NPT_SUCCEEDED(stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH))) {
        if (line.GetLength() == 0) {
            // empty line, end of headers
            break;
        }
        if (header_pending && (line[0] == ' ' || line[0] == '\t')) {
            // continuation (folded header)
            header_value.Append(line.GetChars()+1, line.GetLength()-1);
        } else {
            // add the pending header to the list
            if (header_pending) {
                header_value.Trim();
                AddHeader(header_name, header_value);
                header_pending = false;
                NPT_LOG_FINEST_2("header - %s: %s", 
                                 header_name.GetChars(),
                                 header_value.GetChars());
            }

            // find the colon separating the name and the value
            int colon_index = line.Find(':');
            if (colon_index < 1) {
                // invalid syntax, ignore
                continue;
            }
            header_name = line.Left(colon_index);

            // the field value starts at the first non-whitespace
            const char* value = line.GetChars()+colon_index+1;
            while (*value == ' ' || *value == '\t') {
                value++;
            }
            header_value = value;
           
            // the header is pending
            header_pending = true;
        }
    }

    // if we have a header pending, add it now
    if (header_pending) {
        header_value.Trim();
        AddHeader(header_name, header_value);
        header_pending = false;
        NPT_LOG_FINEST_2("header %s: %s", 
                         header_name.GetChars(),
                         header_value.GetChars());
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::Emit
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeaders::Emit(NPT_OutputStream& stream) const
{
    // for each header in the list
    NPT_List<NPT_HttpHeader*>::Iterator header = m_Headers.GetFirstItem();
    while (header) {
        // emit the header
        NPT_CHECK_WARNING((*header)->Emit(stream));
        ++header;
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::GetHeader
+---------------------------------------------------------------------*/
NPT_HttpHeader*
NPT_HttpHeaders::GetHeader(const char* name) const
{
    // check args
    if (name == NULL) return NULL;

    // find a matching header
    NPT_List<NPT_HttpHeader*>::Iterator header = m_Headers.GetFirstItem();
    while (header) {
        if ((*header)->GetName().Compare(name, true) == 0) {
            return *header;
        }
        ++header;
    }

    // not found
    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::AddHeader
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeaders::AddHeader(const char* name, const char* value)
{
    return m_Headers.Add(new NPT_HttpHeader(name, value));
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::SetHeader
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeaders::SetHeader(const char* name, const char* value, bool replace)
{
    NPT_HttpHeader* header = GetHeader(name);
    if (header == NULL) {
        return AddHeader(name, value);
    } else if (replace) {
        return header->SetValue(value);
    } else {
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::GetHeaderValue
+---------------------------------------------------------------------*/
const NPT_String*
NPT_HttpHeaders::GetHeaderValue(const char* name) const
{
    NPT_HttpHeader* header = GetHeader(name);
    if (header == NULL) {
        return NULL;
    } else {
        return &header->GetValue();
    }
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::NPT_HttpEntity() :
    m_ContentLength(0),
    m_HasContentLength(false)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::NPT_HttpEntity(const NPT_HttpHeaders& headers) :
    m_ContentLength(0),
    m_HasContentLength(false)
{
    SetHeaders(headers);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::~NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::~NPT_HttpEntity()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetHeaders
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpEntity::SetHeaders(const NPT_HttpHeaders& headers) 
{
    NPT_HttpHeader* header;
    
    // Content-Length
    header = headers.GetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH);
    if (header != NULL) {
        m_HasContentLength = true;
        NPT_LargeSize length;
        if (NPT_SUCCEEDED(header->GetValue().ToInteger64(length))) {
            m_ContentLength = length;
        } else {
            m_ContentLength = 0;
        }
    }

    // Content-Type
    header = headers.GetHeader(NPT_HTTP_HEADER_CONTENT_TYPE);
    if (header != NULL) {
        m_ContentType = header->GetValue();
    }

    // Content-Encoding
    header = headers.GetHeader(NPT_HTTP_HEADER_CONTENT_ENCODING);
    if (header != NULL) {
        m_ContentEncoding = header->GetValue();
    }

    // Transfer-Encoding
    header = headers.GetHeader(NPT_HTTP_HEADER_TRANSFER_ENCODING);
    if (header != NULL) {
        m_TransferEncoding = header->GetValue();
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::GetInputStream(NPT_InputStreamReference& stream)
{
    // reset output params first
    stream = NULL;

    if (m_InputStream.IsNull()) return NPT_FAILURE;
    
    stream = m_InputStream;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetInputStream(const NPT_InputStreamReference& stream,
                               bool update_content_length /* = false */)
{
    m_InputStream = stream;

    // get the content length from the stream
    if (update_content_length) {
        m_HasContentLength = true;
        m_ContentLength = 0;
        if (!stream.IsNull()) {
            if (NPT_FAILED(stream->GetSize(m_ContentLength))) {
                m_ContentLength = 0;
            }
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetInputStream(const void* data, NPT_Size data_size)
{
    NPT_MemoryStream* memory_stream = new NPT_MemoryStream(data, data_size);
    NPT_InputStreamReference body(memory_stream);
    return SetInputStream(body, true);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetInputStream(const char* string)
{
    if (string == NULL) return NPT_ERROR_INVALID_PARAMETERS;
    NPT_MemoryStream* memory_stream = new NPT_MemoryStream((const void*)string, 
                                                           NPT_StringLength(string));
    NPT_InputStreamReference body(memory_stream);
    return SetInputStream(body, true);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetInputStream(const NPT_String& string)
{
    NPT_MemoryStream* memory_stream = new NPT_MemoryStream((const void*)string.GetChars(), 
                                                           string.GetLength());
    NPT_InputStreamReference body(memory_stream);
    return SetInputStream(body, true);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpEntity::Load(NPT_DataBuffer& buffer)
{
    // check that we have an input stream
    if (m_InputStream.IsNull()) return NPT_ERROR_INVALID_STATE;

    // load the stream into the buffer
    if (m_ContentLength != (NPT_Size)m_ContentLength) return NPT_ERROR_OUT_OF_RANGE;
    return m_InputStream->Load(buffer, (NPT_Size)m_ContentLength);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetContentLength
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetContentLength(NPT_LargeSize length)
{
    m_ContentLength = length;
    m_HasContentLength = true;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetContentType
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetContentType(const char* type)
{
    m_ContentType = type;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetContentEncoding
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetContentEncoding(const char* encoding)
{
    m_ContentEncoding = encoding;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetTransferEncoding
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetTransferEncoding(const char* encoding)
{
    m_TransferEncoding = encoding;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::NPT_HttpMessage
+---------------------------------------------------------------------*/
NPT_HttpMessage::NPT_HttpMessage(const char* protocol) :
    m_Protocol(protocol),
    m_Entity(NULL)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::NPT_HttpMessage
+---------------------------------------------------------------------*/
NPT_HttpMessage::~NPT_HttpMessage()
{
    delete m_Entity;
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::SetEntity
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpMessage::SetEntity(NPT_HttpEntity* entity)
{
    if (entity != m_Entity) {
        delete m_Entity;
        m_Entity = entity;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::ParseHeaders
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpMessage::ParseHeaders(NPT_BufferedInputStream& stream)
{
    return m_Headers.Parse(stream);
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::NPT_HttpRequest
+---------------------------------------------------------------------*/
NPT_HttpRequest::NPT_HttpRequest(const NPT_HttpUrl& url, 
                                 const char*        method, 
                                 const char*        protocol) :
    NPT_HttpMessage(protocol),
    m_Url(url),
    m_Method(method)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::NPT_HttpRequest
+---------------------------------------------------------------------*/
NPT_HttpRequest::NPT_HttpRequest(const char* url, 
                                 const char* method, 
                                 const char* protocol) :
    NPT_HttpMessage(protocol),
    m_Url(url),
    m_Method(method)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::SetUrl
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpRequest::SetUrl(const char* url)
{
    m_Url = url;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::SetUrl
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpRequest::SetUrl(const NPT_HttpUrl& url)
{
    m_Url = url;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpRequest::Parse(NPT_BufferedInputStream& stream, 
                       const NPT_SocketAddress* endpoint,
                       NPT_HttpRequest*&        request)
{
    // default return value
    request = NULL;

    // read the response line
    NPT_String line;
    NPT_CHECK_FINE(stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH));
    NPT_LOG_FINEST_1("http request: %s", line.GetChars());
    
    // check the request line
    int first_space = line.Find(' ');
    if (first_space < 0) return NPT_ERROR_HTTP_INVALID_REQUEST_LINE;
    int second_space = line.Find(' ', first_space+1);
    if (second_space < 0) {
        return NPT_ERROR_HTTP_INVALID_REQUEST_LINE;
    }

    // parse the request line
    NPT_String method   = line.SubString(0, first_space);
    NPT_String uri      = line.SubString(first_space+1, second_space-first_space-1);
    NPT_String protocol = line.SubString(second_space+1);

    // create a request
    bool proxy_style_request = false;
    if (uri.StartsWith("http://", true)) {
        // proxy-style request with absolute URI
        request = new NPT_HttpRequest(uri, method, protocol);
        proxy_style_request = true;
    } else {
        // normal absolute path request
        request = new NPT_HttpRequest("http:", method, protocol);
    }

    // parse headers
    NPT_Result result = request->ParseHeaders(stream);
    if (NPT_FAILED(result)) {
        delete request;
        request = NULL;
        return result;
    }

    // update the URL
    if (!proxy_style_request) {
        request->m_Url.SetScheme("http");
        request->m_Url.SetPathPlus(uri);
        request->m_Url.SetPort(NPT_HTTP_DEFAULT_PORT);

        // check for a Host: header
        NPT_HttpHeader* host_header = request->GetHeaders().GetHeader(NPT_HTTP_HEADER_HOST);
        if (host_header) {
            request->m_Url.SetHost(host_header->GetValue());
        } else {
            // use the endpoint as the host
            if (endpoint) {
                request->m_Url.SetHost(endpoint->ToString());
            } else {
                // use defaults
                request->m_Url.SetHost("localhost");
            }
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::~NPT_HttpRequest
+---------------------------------------------------------------------*/
NPT_HttpRequest::~NPT_HttpRequest()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpRequest::Emit
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpRequest::Emit(NPT_OutputStream& stream, bool use_proxy) const
{
    // write the request line
    stream.WriteString(m_Method);
    stream.WriteFully(" ", 1);
    if (use_proxy) {
        stream.WriteString(m_Url.ToString(false));
    } else {
        stream.WriteString(m_Url.ToRequestString());
    }
    stream.WriteFully(" ", 1);
    stream.WriteString(m_Protocol);
    stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);

    // emit headers
    m_Headers.Emit(stream);

    // finish with an empty line
    stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::NPT_HttpResponse
+---------------------------------------------------------------------*/
NPT_HttpResponse::NPT_HttpResponse(NPT_HttpStatusCode status_code,
                                   const char*        reason_phrase,
                                   const char*        protocol) :
    NPT_HttpMessage(protocol),
    m_StatusCode(status_code),
    m_ReasonPhrase(reason_phrase)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::~NPT_HttpResponse
+---------------------------------------------------------------------*/
NPT_HttpResponse::~NPT_HttpResponse()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::SetStatus
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponse::SetStatus(NPT_HttpStatusCode status_code,
                            const char*        reason_phrase,
                            const char*        protocol)
{    
    m_StatusCode   = status_code;
    m_ReasonPhrase = reason_phrase;
    m_Protocol     = protocol;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::SetProtocol
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponse::SetProtocol(const char* protocol)
{
    m_Protocol = protocol;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::Emit
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponse::Emit(NPT_OutputStream& stream) const
{
    // write the request line
    stream.WriteString(m_Protocol);
    stream.WriteFully(" ", 1);
    stream.WriteString(NPT_String::FromInteger(m_StatusCode));
    stream.WriteFully(" ", 1);
    stream.WriteString(m_ReasonPhrase);
    stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);

    // emit headers
    m_Headers.Emit(stream);

    // finish with an empty line
    stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponse::Parse(NPT_BufferedInputStream& stream, 
                        NPT_HttpResponse*&       response,
                        bool                     reuse_response /* = false */)
{
    // default return value
    if (!reuse_response) response = NULL;

    // read the response line
    NPT_String line;
    NPT_Result res = stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH);
    if (NPT_FAILED(res)) {
        if (res != NPT_ERROR_TIMEOUT && res != NPT_ERROR_EOS) NPT_CHECK_WARNING(res);
        return res;
    }
    
    // check the response line
    int first_space = line.Find(' ');
    if (first_space < 0) return NPT_ERROR_HTTP_INVALID_RESPONSE_LINE;
    int second_space = line.Find(' ', first_space+1);
    if (second_space < 0 || second_space-first_space != 4) {
        return NPT_ERROR_HTTP_INVALID_RESPONSE_LINE;
    }

    // parse the response line
    NPT_String protocol = line.SubString(0, first_space);
    NPT_String status_code = line.SubString(first_space+1, 3);
    NPT_String reason_phrase = line.SubString(first_space+1+3+1, 
                                              line.GetLength()-(first_space+1+3+1));

    // create a response object
    NPT_UInt32 status_code_int = 0;
    status_code.ToInteger(status_code_int);
    if (!reuse_response) {
        response = new NPT_HttpResponse(status_code_int, reason_phrase, protocol);
    } else {
        response->SetStatus(status_code_int, reason_phrase, protocol);
    }

    // parse headers
    NPT_Result result = response->ParseHeaders(stream);
    if (NPT_FAILED(result)) {
        delete response;
        response = NULL;
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_HttpTcpConnector
+---------------------------------------------------------------------*/
class NPT_HttpTcpConnector : public NPT_HttpClient::Connector
{
    virtual NPT_Result Connect(const char*                   hostname, 
                               NPT_UInt16                    port, 
                               NPT_Timeout                   connection_timeout,
                               NPT_Timeout                   io_timeout,
                               NPT_Timeout                   name_resolver_timeout,
                               NPT_InputStreamReference&     input_stream, 
                               NPT_OutputStreamReference&    output_stream);
};

/*----------------------------------------------------------------------
|   NPT_HttpTcpConnector::Connect
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpTcpConnector::Connect(const char*                hostname, 
                              NPT_UInt16                 port, 
                              NPT_Timeout                connection_timeout,
                              NPT_Timeout                io_timeout,
                              NPT_Timeout                name_resolver_timeout,
                              NPT_InputStreamReference&  input_stream, 
                              NPT_OutputStreamReference& output_stream)
{
    // get the address and port to which we need to connect
    NPT_IpAddress address;
    NPT_CHECK(address.ResolveName(hostname, name_resolver_timeout));

    // connect to the server
    NPT_LOG_FINE_2("TCP connector will connect to %s:%d", hostname, port);
    NPT_TcpClientSocket connection;
    connection.SetReadTimeout(io_timeout);
    connection.SetWriteTimeout(io_timeout);
    NPT_SocketAddress socket_address(address, port);
    NPT_CHECK(connection.Connect(socket_address, connection_timeout));

    // get the streams
    NPT_CHECK(connection.GetInputStream(input_stream));
    NPT_CHECK(connection.GetOutputStream(output_stream));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpProxySelector::GetSystemDefault()
+---------------------------------------------------------------------*/
//// temporary place holder implementation
NPT_HttpProxySelector*
NPT_HttpProxySelector::GetSystemDefault()
{
    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_HttpStaticProxySelector
+---------------------------------------------------------------------*/
class NPT_HttpStaticProxySelector : public NPT_HttpProxySelector
{
public:
    // constructor
    NPT_HttpStaticProxySelector(const char* hostname, NPT_UInt16 port);

    // NPT_HttpProxySelector methods
    NPT_Result GetProxyForUrl(const NPT_HttpUrl& url, NPT_HttpProxyAddress& proxy);

private:
    // members
    NPT_HttpProxyAddress m_Proxy;
};

/*----------------------------------------------------------------------
|   NPT_HttpStaticProxySelector::NPT_HttpStaticProxySelector
+---------------------------------------------------------------------*/
NPT_HttpStaticProxySelector::NPT_HttpStaticProxySelector(const char* hostname, NPT_UInt16 port) :
    m_Proxy(hostname, port)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpStaticProxySelector::GetProxyForUrl
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpStaticProxySelector::GetProxyForUrl(const NPT_HttpUrl& /* url */, 
                                            NPT_HttpProxyAddress& proxy)
{
    proxy = m_Proxy;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::NPT_HttpClient
+---------------------------------------------------------------------*/
NPT_HttpClient::NPT_HttpClient(Connector* connector) :
    m_ProxySelector(NPT_HttpProxySelector::GetSystemDefault()),
    m_ProxySelectorIsOwned(false),
    m_Connector(connector)
{
    m_Config.m_FollowRedirect      = true;
    m_Config.m_ConnectionTimeout   = NPT_HTTP_CLIENT_DEFAULT_CONNECTION_TIMEOUT;
    m_Config.m_IoTimeout           = NPT_HTTP_CLIENT_DEFAULT_CONNECTION_TIMEOUT;
    m_Config.m_NameResolverTimeout = NPT_HTTP_CLIENT_DEFAULT_NAME_RESOLVER_TIMEOUT;

    if (connector == NULL) m_Connector = new NPT_HttpTcpConnector();
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::~NPT_HttpClient
+---------------------------------------------------------------------*/
NPT_HttpClient::~NPT_HttpClient()
{
    if (m_ProxySelectorIsOwned) {
        delete m_ProxySelector;
    }
    delete m_Connector;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SetConfig
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::SetConfig(const Config& config)
{
    m_Config = config;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SetProxy
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::SetProxy(const char* hostname, NPT_UInt16 port)
{
    if (m_ProxySelectorIsOwned) {
        delete m_ProxySelector;
        m_ProxySelector = NULL;
        m_ProxySelectorIsOwned = false;
    }

    // only set a proxy selector is hostname is not null, disable the
    // proxy otherwise
    if (hostname != NULL && port != NPT_HTTP_INVALID_PORT) {
        m_ProxySelector = new NPT_HttpStaticProxySelector(hostname, port);
        m_ProxySelectorIsOwned = true;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SetProxy
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpClient::SetProxySelector(NPT_HttpProxySelector* selector)
{
    if (m_ProxySelectorIsOwned) {
        delete m_ProxySelector;
    }
    m_ProxySelector = selector;
    m_ProxySelectorIsOwned = false;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SetConnector
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpClient::SetConnector(Connector* connector)
{
    Connector* old_connector = m_Connector;
    m_Connector = connector;
    if (old_connector != connector) {
        delete old_connector;
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SetTimeouts
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::SetTimeouts(NPT_Timeout connection_timeout, 
                            NPT_Timeout io_timeout,
                            NPT_Timeout name_resolver_timeout)
{
    m_Config.m_ConnectionTimeout   = connection_timeout;
    m_Config.m_IoTimeout           = io_timeout;
    m_Config.m_NameResolverTimeout = name_resolver_timeout;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SendRequestOnce
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::SendRequestOnce(NPT_HttpRequest&   request, 
                                NPT_HttpResponse*& response)
{
    // setup default values
    response = NULL;

    NPT_LOG_FINE_1("requesting URL %s", request.GetUrl().ToString().GetChars());
    
    // get the address and port to which we need to connect
    NPT_HttpProxyAddress proxy;
    bool                 use_proxy = false;
    if (m_ProxySelector) {
        // we have a proxy selector, ask it to select a proxy for this URL
        NPT_Result result = m_ProxySelector->GetProxyForUrl(request.GetUrl(), proxy);
        if (NPT_FAILED(result) && result != NPT_ERROR_HTTP_NO_PROXY) {
            NPT_LOG_WARNING_1("proxy selector failure (%d)", result);
            return result;
        }
        use_proxy = true;
    }

    // decide which host we need to connect to
    const char* server_hostname;
    NPT_UInt16  server_port;
    if (use_proxy) {
        // the proxy is set
        server_hostname = (const char*)proxy.GetHostName();
        server_port = proxy.GetPort();
    } else {
        // no proxy selector, so no proxy: connect directly
        server_hostname = (const char*)request.GetUrl().GetHost();
        server_port = request.GetUrl().GetPort();
    }

    // connect to the server or proxy
    NPT_InputStreamReference  input_stream;
    NPT_OutputStreamReference output_stream;
    NPT_LOG_FINER_2("connecting to %s:%d", server_hostname, server_port);
    NPT_CHECK_WARNING(m_Connector->Connect(server_hostname,
                                           server_port,
                                           m_Config.m_ConnectionTimeout,
                                           m_Config.m_IoTimeout,
                                           m_Config.m_NameResolverTimeout,
                                           input_stream,
                                           output_stream));

	NPT_CHECK_WARNING(WriteRequest(*output_stream.AsPointer(), request, use_proxy));

    return ReadResponse(input_stream,
                        request.GetMethod() == NPT_HTTP_METHOD_GET || 
                        request.GetMethod() == NPT_HTTP_METHOD_POST,
					    response);
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::WriteRequest
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::WriteRequest(NPT_OutputStream& output_stream, 
                             NPT_HttpRequest&  request,
                             bool			   use_proxy /* = false */)
{
    // add any headers that may be missing
    NPT_HttpHeaders& headers = request.GetHeaders();
    if (request.GetProtocol() == NPT_HTTP_PROTOCOL_1_0) {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, 
            "close", false); // set but don't replace
    }
    headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
        m_UserAgentHeader, false); // set but don't replace
    NPT_String host = request.GetUrl().GetHost();
    if (request.GetUrl().GetPort() != NPT_HTTP_DEFAULT_PORT) {
        host += ":";
        host += NPT_String::FromInteger(request.GetUrl().GetPort());
    }
    headers.SetHeader(NPT_HTTP_HEADER_HOST, host, false); // set but don't replace

    // get the request entity to set additional headers
    NPT_InputStreamReference body_stream;
    NPT_HttpEntity* entity = request.GetEntity();
    if (entity && NPT_SUCCEEDED(entity->GetInputStream(body_stream))) {
        if (!body_stream.IsNull()) {
            // set the content length, unless it is unknown (chunked encoding)
            if (entity->GetContentLength() != 0 || 
                (entity->GetTransferEncoding() != NPT_HTTP_TRANSFER_ENCODING_CHUNKED && 
                 entity->HasContentLength())) {
                    headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, 
                        NPT_String::FromInteger(entity->GetContentLength()));
            }
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

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;

    // emit the request headers into the header buffer
    request.Emit(header_stream, use_proxy);

    // send the headers
    NPT_CHECK_WARNING(output_stream.WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send request body (content length = 0 means copy until body returns EOS)
    if (!body_stream.IsNull()) {
        NPT_CHECK_FATAL(NPT_StreamToStreamCopy(*body_stream.AsPointer(), output_stream, 0, entity->GetContentLength()));
    }

    // flush the output stream so that everything is sent to the server
    output_stream.Flush();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SendRequestOnce
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::ReadResponse(NPT_InputStreamReference& input_stream,
                             bool                      expect_entity,
                             NPT_HttpResponse*&        response)
{
    bool reuse_response = false;

    // setup default values
    response = NULL;

    // create a buffered stream for this socket stream
    NPT_BufferedInputStreamReference buffered_input_stream(new NPT_BufferedInputStream(input_stream));

continue_100:
    // parse the response
    NPT_CHECK_WARNING(NPT_HttpResponse::Parse(*buffered_input_stream, response, reuse_response));
    NPT_LOG_FINER_2("got response, code=%d, msg=%s",
                   response->GetStatusCode(),
                   response->GetReasonPhrase().GetChars());

    // any 1xx response means continue
    if (response && response->GetStatusCode()/100 == 1) {
        NPT_LOG_FINE("NPT_HttpClient::SendRequest - received 100 continue\n");
        reuse_response = true;
        goto continue_100;
    }
    
    // unbuffer the stream
    buffered_input_stream->SetBufferSize(0);

    // create an entity if one is expected in the response
    if (expect_entity) {
        NPT_HttpEntity* response_entity = new NPT_HttpEntity(response->GetHeaders());
        
        // check for chunked Transfer-Encoding
        if (response_entity->GetTransferEncoding() == NPT_HTTP_TRANSFER_ENCODING_CHUNKED) {
            response_entity->SetInputStream(NPT_InputStreamReference(new NPT_HttpChunkedInputStream(buffered_input_stream)));
            response_entity->SetTransferEncoding(NULL);
        } else {
            response_entity->SetInputStream((NPT_InputStreamReference)buffered_input_stream);
        }
        response->SetEntity(response_entity);
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpClient::SendRequest
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpClient::SendRequest(NPT_HttpRequest&   request, 
                            NPT_HttpResponse*& response)
{
    NPT_Cardinal watchdog = NPT_HTTP_MAX_REDIRECTS;
    bool         keep_going;
    NPT_Result   result;

    // default value
    response = NULL;
    
    // check that for GET requests there is no entity
    if (request.GetEntity() != NULL &&
        request.GetMethod() == NPT_HTTP_METHOD_GET) {
        return NPT_ERROR_HTTP_INVALID_REQUEST;
    }
    
    do {
        keep_going = false;
        result = SendRequestOnce(request, response);
        if (NPT_FAILED(result)) break;
        if (response && m_Config.m_FollowRedirect &&
            (request.GetMethod() == NPT_HTTP_METHOD_GET ||
             request.GetMethod() == NPT_HTTP_METHOD_HEAD) &&
            (response->GetStatusCode() == 301 ||
             response->GetStatusCode() == 302 ||
             response->GetStatusCode() == 303 ||
             response->GetStatusCode() == 307)) {
            // handle redirect
            NPT_HttpHeader* location = response->GetHeaders().GetHeader(NPT_HTTP_HEADER_LOCATION);
            if (location) {
                // replace the request url
                if (NPT_SUCCEEDED(request.SetUrl(location->GetValue()))) {
                    NPT_LOG_FINE_1("redirecting to %s", location->GetValue().GetChars());
                    keep_going = true;
                    delete response;
                    response = NULL;
                }
            }
        }
    } while (keep_going && watchdog--);

    return result;
}

/*----------------------------------------------------------------------
|   NPT_HttpRequestContext::NPT_HttpRequestContext
+---------------------------------------------------------------------*/
NPT_HttpRequestContext::NPT_HttpRequestContext(const NPT_SocketAddress* local_address,
                                               const NPT_SocketAddress* remote_address)
{
    if (local_address) m_LocalAddress   = *local_address;
    if (remote_address) m_RemoteAddress = *remote_address;
}
                           
/*----------------------------------------------------------------------
|   NPT_HttpServer::NPT_HttpServer
+---------------------------------------------------------------------*/
NPT_HttpServer::NPT_HttpServer(NPT_UInt16 listen_port, 
                               bool       reuse_address /* = true */) :
    m_BoundPort(0)
{
    m_Config.m_ListenAddress     = NPT_IpAddress::Any;
    m_Config.m_ListenPort        = listen_port;
    m_Config.m_IoTimeout         = NPT_HTTP_SERVER_DEFAULT_IO_TIMEOUT;
    m_Config.m_ConnectionTimeout = NPT_HTTP_SERVER_DEFAULT_CONNECTION_TIMEOUT;
    m_Config.m_ReuseAddress      = reuse_address;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::NPT_HttpServer
+---------------------------------------------------------------------*/
NPT_HttpServer::NPT_HttpServer(NPT_IpAddress listen_address, 
                               NPT_UInt16    listen_port, 
                               bool          reuse_address /* = true */) :
    m_BoundPort(0)
{
    m_Config.m_ListenAddress     = listen_address;
    m_Config.m_ListenPort        = listen_port;
    m_Config.m_IoTimeout         = NPT_HTTP_SERVER_DEFAULT_IO_TIMEOUT;
    m_Config.m_ConnectionTimeout = NPT_HTTP_SERVER_DEFAULT_CONNECTION_TIMEOUT;
    m_Config.m_ReuseAddress      = reuse_address;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::~NPT_HttpServer
+---------------------------------------------------------------------*/
NPT_HttpServer::~NPT_HttpServer()
{
    m_RequestHandlers.Apply(NPT_ObjectDeleter<HandlerConfig>());
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::Bind
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::Bind()
{
    // check if we're already bound
    if (m_BoundPort != 0) return NPT_SUCCESS;

    // bind
    NPT_Result result = m_Socket.Bind(
        NPT_SocketAddress(m_Config.m_ListenAddress, m_Config.m_ListenPort), 
        m_Config.m_ReuseAddress);
    if (NPT_FAILED(result)) return result;

    // remember that we're bound
    NPT_SocketInfo info;
    m_Socket.GetInfo(info);
    m_BoundPort = info.local_address.GetPort();
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::SetConfig
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::SetConfig(const Config& config)
{
    m_Config = config;

    // check that we can bind to this listen port
    return Bind();
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::SetListenPort
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::SetListenPort(NPT_UInt16 port, bool reuse_address)
{
    m_Config.m_ListenPort = port;
    m_Config.m_ReuseAddress = reuse_address;
    return Bind();
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::SetTimeouts
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::SetTimeouts(NPT_Timeout connection_timeout, 
                            NPT_Timeout io_timeout)
{
    m_Config.m_ConnectionTimeout = connection_timeout;
    m_Config.m_IoTimeout         = io_timeout;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::Abort
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::Abort()
{
    m_Socket.Disconnect();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::WaitForNewClient
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::WaitForNewClient(NPT_InputStreamReference&  input,
                                 NPT_OutputStreamReference& output,
                                 NPT_HttpRequestContext*    context)
{
    // ensure that we're bound 
    NPT_CHECK(Bind());

    // wait for a connection
    NPT_Socket* client;
    NPT_LOG_FINE_2("waiting for new connection on %s:%d...", 
        (const char*)m_Config.m_ListenAddress.ToString(),
        m_BoundPort);
    NPT_CHECK_WARNING(m_Socket.WaitForNewClient(client, m_Config.m_ConnectionTimeout));
    if (client == NULL) return NPT_ERROR_INTERNAL;

    // get the client info
    if (context) {
        NPT_SocketInfo client_info;
        client->GetInfo(client_info);

        context->SetLocalAddress(client_info.local_address);
        context->SetRemoteAddress(client_info.remote_address);

        NPT_LOG_FINE_2("client connected (%s <- %s)",
                       client_info.local_address.ToString().GetChars(),
                       client_info.remote_address.ToString().GetChars());
    }

    // configure the socket
    client->SetReadTimeout(m_Config.m_IoTimeout);
    client->SetWriteTimeout(m_Config.m_IoTimeout);

    // get the streams
    client->GetInputStream(input);
    client->GetOutputStream(output);

    // we don't need the socket anymore
    delete client;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::Loop
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::Loop()
{
    NPT_InputStreamReference  input;
    NPT_OutputStreamReference output;
    NPT_HttpRequestContext    context;
    NPT_Result                result;
    
    do {
        // wait for a client to connect
        result = WaitForNewClient(input, output, &context);
        NPT_LOG_FINE_2("WaitForNewClient returned %d (%s)", 
                       result,
                       NPT_ResultText(result));
        if (NPT_FAILED(result)) break;

        // send a response
        result = RespondToClient(input, output, context);
        NPT_LOG_FINE_2("ResponToClient returned %d", 
                       result,
                       NPT_ResultText(result));

        // release the stream references so that the socket can be closed
        input = NULL;
        output = NULL;
    } while (NPT_SUCCEEDED(result));
    
    return result;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::HandlerConfig::HandlerConfig
+---------------------------------------------------------------------*/
NPT_HttpServer::HandlerConfig::HandlerConfig(NPT_HttpRequestHandler* handler, 
                                             const char*             path, 
                                             bool                    include_children) :
    m_Handler(handler),
    m_Path(path),
    m_IncludeChildren(include_children)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::HandlerConfig::~HandlerConfig
+---------------------------------------------------------------------*/
NPT_HttpServer::HandlerConfig::~HandlerConfig()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::AddRequestHandler
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpServer::AddRequestHandler(NPT_HttpRequestHandler* handler, 
                                  const char*             path, 
                                  bool                    include_children)
{
    return m_RequestHandlers.Add(new HandlerConfig(handler, path, include_children));
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::FindRequestHandler
+---------------------------------------------------------------------*/
NPT_HttpRequestHandler* 
NPT_HttpServer::FindRequestHandler(NPT_HttpRequest& request)
{
    for (NPT_List<HandlerConfig*>::Iterator it = m_RequestHandlers.GetFirstItem();
         it;
         ++it) {
         HandlerConfig* config = *it;
         if (config->m_IncludeChildren) {
             if (request.GetUrl().GetPath().StartsWith(config->m_Path)) {
                 return config->m_Handler;
             }  
         } else {
             if (request.GetUrl().GetPath() == config->m_Path) {
                 return config->m_Handler;
             }
         }
    }

    // not found
    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::RespondToClient
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpServer::RespondToClient(NPT_InputStreamReference&     input,
                                NPT_OutputStreamReference&    output,
                                const NPT_HttpRequestContext& context)
{
    NPT_HttpRequest*  request;
    NPT_HttpResponse* response = NULL;
    NPT_Result        result   = NPT_ERROR_NO_SUCH_ITEM;

    NPT_HttpResponder responder(input, output);
    NPT_CHECK_WARNING(responder.ParseRequest(request, &context.GetLocalAddress()));
    NPT_LOG_FINE_1("request, path=%s", request->GetUrl().ToRequestString(true).GetChars());
    
    // prepare the response body
    NPT_HttpEntity* body = new NPT_HttpEntity();

    NPT_HttpRequestHandler* handler = FindRequestHandler(*request);
    if (handler) {
        // create a response object
        response = new NPT_HttpResponse(200, "OK", NPT_HTTP_PROTOCOL_1_0);
        response->SetEntity(body);

        // ask the handler to setup the response
        result = handler->SetupResponse(*request, context, *response);
    }
    if (result == NPT_ERROR_NO_SUCH_ITEM || handler == NULL) {
        body->SetInputStream(NPT_HTTP_DEFAULT_404_HTML);
        body->SetContentType("text/html");
        if (response == NULL) {
            response = new NPT_HttpResponse(404, "Not Found", NPT_HTTP_PROTOCOL_1_0);
        } else {
            response->SetStatus(404, "Not Found");
        }
        response->SetEntity(body);
        handler = NULL;
    } else if (result == NPT_ERROR_PERMISSION_DENIED) {
        body->SetInputStream(NPT_HTTP_DEFAULT_403_HTML);
        body->SetContentType("text/html");
        response->SetStatus(403, "Forbidden");
        handler = NULL;
    } else if (NPT_FAILED(result)) {
        body->SetInputStream(NPT_HTTP_DEFAULT_500_HTML);
        body->SetContentType("text/html");
        response->SetStatus(500, "Internal Error");
        handler = NULL;
    }

    // send the response headers
    result = responder.SendResponseHeaders(*response);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_2("SendResponseHeaders failed (%d:%s)", result, NPT_ResultText(result));
        goto end;
    }
    
    // send the body
    if (request->GetMethod() != NPT_HTTP_METHOD_HEAD) {
        if (handler) {
            result = handler->SendResponseBody(context, *response, *output);
        } else {
            NPT_InputStreamReference body_stream;
            body->GetInputStream(body_stream);
            if (!body_stream.IsNull()) {
                result = NPT_StreamToStreamCopy(*body_stream, *output, 0, body->GetContentLength());
                if (NPT_FAILED(result)) {
                    NPT_LOG_INFO_2("NPT_StreamToStreamCopy returned %d (%s)", result, NPT_ResultText(result));
                    goto end;
                }
            }
        }
    }

    // flush
    output->Flush();

end:
    // cleanup
    delete response;
    delete request;

    return result;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::NPT_HttpResponder
+---------------------------------------------------------------------*/
NPT_HttpResponder::NPT_HttpResponder(NPT_InputStreamReference&  input,
                                     NPT_OutputStreamReference& output) :
    m_Input(new NPT_BufferedInputStream(input)),
    m_Output(output)
{
    m_Config.m_IoTimeout = NPT_HTTP_SERVER_DEFAULT_IO_TIMEOUT;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::~NPT_HttpResponder
+---------------------------------------------------------------------*/
NPT_HttpResponder::~NPT_HttpResponder()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::SetConfig
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::SetConfig(const Config& config)
{
    m_Config = config;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::SetTimeout
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::SetTimeout(NPT_Timeout io_timeout)
{
    m_Config.m_IoTimeout = io_timeout;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::ParseRequest
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::ParseRequest(NPT_HttpRequest*&        request,
                                const NPT_SocketAddress* local_address)
{
    // parse the request
    NPT_CHECK_WARNING(NPT_HttpRequest::Parse(*m_Input, local_address, request));

    // unbuffer the stream
    m_Input->SetBufferSize(0);

    // don't create an entity if no body is expected
    if (request->GetMethod() == NPT_HTTP_METHOD_GET ||
        request->GetMethod() == NPT_HTTP_METHOD_HEAD) {
        return NPT_SUCCESS;
    }

    // set the entity info
    NPT_HttpEntity* entity = new NPT_HttpEntity(request->GetHeaders());
    entity->SetInputStream(m_Input);
    request->SetEntity(entity);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::SendResponseHeaders
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::SendResponseHeaders(NPT_HttpResponse& response)
{
    // add default headers
    NPT_HttpHeaders& headers = response.GetHeaders();
    headers.SetHeader(NPT_HTTP_HEADER_SERVER, 
                      NPT_HttpServer::m_ServerHeader, false);
    if (response.GetProtocol() == NPT_HTTP_PROTOCOL_1_0) {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, 
                          "close", false); // set but don't replace
    }

    // add computed headers
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity) {
        // check if an entity stream has been set
        NPT_InputStreamReference input_stream;
        entity->GetInputStream(input_stream);
        if (!input_stream.IsNull()) {
            // set the content length, unless it is unknown (chunked encoding)
            if (entity->GetContentLength() != 0 || 
                (entity->GetTransferEncoding() != NPT_HTTP_TRANSFER_ENCODING_CHUNKED && 
                 entity->HasContentLength())) {
                headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, 
                                  NPT_String::FromInteger(entity->GetContentLength()));
            }
        }

        // content type
        const NPT_String& content_type = entity->GetContentType();
        if (!content_type.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_TYPE, content_type);
        }

        // content encoding
        const NPT_String& content_encoding = entity->GetContentEncoding();
        if (!content_encoding.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_CONTENT_ENCODING, content_encoding);
        }
        
        // transfer encoding
        const NPT_String& transfer_encoding = entity->GetTransferEncoding();
        if (!transfer_encoding.IsEmpty()) {
            headers.SetHeader(NPT_HTTP_HEADER_TRANSFER_ENCODING, transfer_encoding);
        }
    } else {
        // force content length to 0 if there is no message body
		// (necessary for 1.1 or 1.0 with keep-alive connections)
        headers.SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, "0");
    }
    
    // create a memory stream to buffer the response line and headers
    NPT_MemoryStream buffer;

    // emit the response line
    NPT_CHECK_WARNING(response.Emit(buffer));

    // send the buffer
    NPT_CHECK_WARNING(m_Output->WriteFully(buffer.GetData(), buffer.GetDataSize()));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpRequestHandler::SendResponseBody
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpRequestHandler::SendResponseBody(const NPT_HttpRequestContext& /*context*/,
                                         NPT_HttpResponse&             response,
                                         NPT_OutputStream&             output)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_SUCCESS;
    
    NPT_InputStreamReference body_stream;
    entity->GetInputStream(body_stream);
    if (body_stream.IsNull()) return NPT_SUCCESS;
    
    // check for chunked transfer encoding
    NPT_OutputStream* dest = &output;
    if (entity->GetTransferEncoding() == NPT_HTTP_TRANSFER_ENCODING_CHUNKED) {
        dest = new NPT_HttpChunkedOutputStream(output);
    }
    
    // send the body
    NPT_LOG_FINE_1("sending body stream, %lld bytes", entity->GetContentLength());
    NPT_LargeSize bytes_written = 0;
    NPT_Result result = NPT_StreamToStreamCopy(*body_stream, *dest, 0, entity->GetContentLength(), &bytes_written);
    if (NPT_FAILED(result)) {
        NPT_LOG_FINE_3("body stream only partially sent, %lld bytes (%d:%s)", 
                       bytes_written, 
                       result, 
                       NPT_ResultText(result));
    }
    
    // flush
    dest->Flush();
    
    // cleanup
    if (dest != &output) delete dest;
    
    return result;
}

/*----------------------------------------------------------------------
|   NPT_HttpStaticRequestHandler::NPT_HttpStaticRequestHandler
+---------------------------------------------------------------------*/
NPT_HttpStaticRequestHandler::NPT_HttpStaticRequestHandler(const void* data, 
                                                           NPT_Size    size, 
                                                           const char* mime_type,
                                                           bool        copy) :
    m_MimeType(mime_type),
    m_Buffer(data, size, copy)
{}

/*----------------------------------------------------------------------
|   NPT_HttpStaticRequestHandler::NPT_HttpStaticRequestHandler
+---------------------------------------------------------------------*/
NPT_HttpStaticRequestHandler::NPT_HttpStaticRequestHandler(const char* document, 
                                                           const char* mime_type,
                                                           bool        copy) :
    m_MimeType(mime_type),
    m_Buffer(document, NPT_StringLength(document), copy)
{}

/*----------------------------------------------------------------------
|   NPT_HttpStaticRequestHandler::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpStaticRequestHandler::SetupResponse(NPT_HttpRequest&              /*request*/, 
                                            const NPT_HttpRequestContext& /*context*/,
                                            NPT_HttpResponse&             response)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;

    entity->SetContentType(m_MimeType);
    entity->SetInputStream(m_Buffer.GetData(), m_Buffer.GetDataSize());

    return NPT_SUCCESS;
}

const NPT_HttpFileRequestHandler_FileTypeMapEntry 
NPT_HttpFileRequestHandler_DefaultFileTypeMap[43] = {
    {"xml",  "text/xml; charset=\"utf-8\""  },
    {"htm",  "text/html" },
    {"html", "text/html" },
    {"c",    "text/plain"},
    {"css",  "text/css"  },
    {"h",    "text/plain"},
    {"txt",  "text/plain"},
    {"gif",  "image/gif" },
    {"thm",  "image/jpeg"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"jpe",  "image/jpeg"},
    {"jp2",  "image/jp2" },
    {"png",  "image/png" },
    {"bmp",  "image/bmp" },
    {"png",  "image/png"},
    {"tif",  "image/tiff"},
    {"tiff", "image/tiff"},
    {"mpa",  "audio/mpeg"},
    {"mp2",  "audio/mpeg"},
    {"mp3",  "audio/mpeg"},
    {"m4a",  "audio/mp4"},
    {"wma",  "audio/x-ms-wma"},
    {"aif",  "audio/x-aiff"},
    {"aifc", "audio/x-aiff"},
    {"aiff", "audio/x-aiff"},
    {"wav",  "audio/x-wav"},
    {"mpg",  "video/mpeg"},
    {"mpeg", "video/mpeg"},
    {"mp4",  "video/mp4"},
    {"m4v",  "video/mp4"},
    {"mov",  "video/quicktime"},
    {"wmv",  "video/x-ms-wmv"},
    {"asf",  "video/x-ms-asf"},
    {"avi",  "video/x-msvideo"},
    {"divx", "video/x-msvideo"},
    {"xvid", "video/x-msvideo"},
    {"doc",  "application/msword"},
    {"eps",  "application/postscript"},
    {"js",   "application/javascript"},
    {"pdf",  "application/pdf"},
    {"ps",   "application/postscript"},
    {"zip",  "application/zip"}
};

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler::NPT_HttpFileRequestHandler
+---------------------------------------------------------------------*/
NPT_HttpFileRequestHandler::NPT_HttpFileRequestHandler(const char* url_root,
                                                       const char* file_root,
                                                       bool        auto_dir) :
    m_UrlRoot(url_root),
    m_FileRoot(file_root),
    m_DefaultMimeType("text/html"),
    m_UseDefaultFileTypeMap(true),
    m_AutoDir(auto_dir)
{
}

/*----------------------------------------------------------------------
|   helper functions  FIXME: need to move these to a separate module
+---------------------------------------------------------------------*/
static NPT_UInt32
_utf8_decode(const char** str)
{
  NPT_UInt32   result;
  NPT_UInt32   min_value;
  unsigned int bytes_left;

  if (**str == 0) {
      return ~0;
  } else if ((**str & 0x80) == 0x00) {
      result = *(*str)++;
      bytes_left = 0;
      min_value = 0;
  } else if ((**str & 0xE0) == 0xC0) {
      result = *(*str)++ & 0x1F;
      bytes_left = 1;
      min_value  = 0x80;
  } else if ((**str & 0xF0) == 0xE0) {
      result = *(*str)++ & 0x0F;
      bytes_left = 2;
      min_value  = 0x800;
  } else if ((**str & 0xF8) == 0xF0) {
      result = *(*str)++ & 0x07;
      bytes_left = 3;
      min_value  = 0x10000;
  } else {
      return ~0;
  }

  while (bytes_left--) {
      if (**str == 0 || (**str & 0xC0) != 0x80) return ~0;
      result = (result << 6) | (*(*str)++ & 0x3F);
  }

  if (result < min_value || (result & 0xFFFFF800) == 0xD800 || result > 0x10FFFF) {
      return ~0;
  }

  return result;
}

/*----------------------------------------------------------------------
|   NPT_HtmlEncode
+---------------------------------------------------------------------*/
static NPT_String
NPT_HtmlEncode(const char* str, const char* chars)
{
    NPT_String encoded;

    // check args
    if (str == NULL) return encoded;

    // reserve at least the size of the current uri
    encoded.Reserve(NPT_StringLength(str));

    // process each character
    while (*str) {
        NPT_UInt32 c = _utf8_decode(&str);
        bool encode = false;
        if (c < ' ' || c > '~') {
            encode = true;
        } else {
            const char* match = chars;
            while (*match) {
                if (c == (NPT_UInt32)*match) {
                    encode = true;
                    break;
                }
                ++match;
            }
        }
        if (encode) {
            // encode
            char hex[9];
            encoded += "&#x";
            unsigned int len = 0;
            if (c > 0xFFFF) {
                NPT_ByteToHex((unsigned char)(c>>24), &hex[0], true);
                NPT_ByteToHex((unsigned char)(c>>16), &hex[2], true);
                len = 4;
            }
            NPT_ByteToHex((unsigned char)(c>>8), &hex[len  ], true);
            NPT_ByteToHex((unsigned char)(c   ), &hex[len+2], true);
            hex[len+4] = ';';
            encoded.Append(hex, len+5);
        } else {
            // no encoding required
            encoded += (char)c;
        }
    }
    
    return encoded;
}

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpFileRequestHandler::SetupResponse(NPT_HttpRequest&              request,
                                          const NPT_HttpRequestContext& /* context */,
                                          NPT_HttpResponse&             response)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;

    // check the method
    if (request.GetMethod() != NPT_HTTP_METHOD_GET &&
        request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
        response.SetStatus(405, "Method Not Allowed");
        return NPT_SUCCESS;
    }

    // set some default headers
    response.GetHeaders().SetHeader(NPT_HTTP_HEADER_ACCEPT_RANGES, "bytes");

    // declare HTTP/1.1 if the client asked for it
    if (request.GetProtocol() == NPT_HTTP_PROTOCOL_1_1) {
        response.SetProtocol(NPT_HTTP_PROTOCOL_1_1);
    }
    
    // TODO: we need to normalize the request path

    // check that the request's path is an entry under the url root
    if (!request.GetUrl().GetPath().StartsWith(m_UrlRoot)) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // compute the filename
    NPT_String filename = m_FileRoot;
    NPT_String relative_path = NPT_Url::PercentDecode(request.GetUrl().GetPath().GetChars()+m_UrlRoot.GetLength());
    filename += "/";
    filename += relative_path;
    
    // get info about the file
    NPT_FileInfo info;
    NPT_File::GetInfo(filename, &info);

    // check if this is a directory 
    if (info.m_Type == NPT_FileInfo::FILE_TYPE_DIRECTORY) {
        if (m_AutoDir) {
            // get the dir entries
            NPT_List<NPT_String> entries;
            NPT_File::ListDir(filename, entries);

            NPT_String html;
            html.Reserve(1024+128*entries.GetItemCount());

            NPT_String html_dirname = NPT_HtmlEncode(relative_path, "<>&");
            html += "<hmtl><head><title>Directory Listing for /";
            html += html_dirname;
            html += "</title></head><body>";
            html += "<h2>Directory Listing for /";
            html += html_dirname;
            html += "</h2><hr><ul>\r\n";
            NPT_String url_base_path = NPT_HtmlEncode(request.GetUrl().GetPath(), "<>&\"");
            
            for (NPT_List<NPT_String>::Iterator i = entries.GetFirstItem();
                 i;
                 ++i) {
                 NPT_String url_filename = NPT_HtmlEncode(*i, "<>&");
                 html += "<li><a href=\"";
                 html += url_base_path;
                 if (!url_base_path.EndsWith("/")) html += "/";
                 html += url_filename;
                 html += "\">";
                 html +=url_filename;
                
                 NPT_String full_path = filename;
                 full_path += "/";
                 full_path += *i;
                 NPT_File::GetInfo(full_path, &info);
                 if (info.m_Type == NPT_FileInfo::FILE_TYPE_DIRECTORY) html += "/";
                 
                 html += "</a><br>\r\n";
            }
            html += "</ul></body></html>";

            entity->SetContentType("text/html");
            entity->SetInputStream(html);
            return NPT_SUCCESS;
        } else {
            return NPT_ERROR_PERMISSION_DENIED;
        }
    }
    
    // open the file
    NPT_File file(filename);
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
    if (NPT_FAILED(result)) {
        return NPT_ERROR_NO_SUCH_ITEM;
    }
    NPT_InputStreamReference stream;
    file.GetInputStream(stream);
    
    // check for range requests
    const NPT_String* range_spec = request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_RANGE);
    if (range_spec) {
        // measure the file size
        NPT_LargeSize file_size = 0;
        result = file.GetSize(file_size);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_2("file.GetSize() failed (%d:%s)", result, NPT_ResultText(result));
            return result;
        }
        NPT_LOG_FINE_1("file size=%lld", file_size);
        if (file_size == 0) return NPT_SUCCESS;
        
        if (!range_spec->StartsWith("bytes=")) {
            NPT_LOG_FINE("unknown range spec");
            response.SetStatus(400, "Bad Request");
            return NPT_SUCCESS;
        }
        NPT_String range(range_spec->GetChars()+6);
        if (range.Find(',') >= 0) {
            NPT_LOG_FINE("multi-range requests not supported");
            response.SetStatus(416, "Requested Range Not Satisfiable");
            return NPT_SUCCESS;            
        }
        int sep = range.Find('-');
        NPT_UInt64 range_start  = 0;
        NPT_UInt64 range_end    = 0;
        bool has_start = false;
        bool has_end   = false;
        bool satisfied = false;
        if (sep < 0) {
            NPT_LOG_FINE("invalid syntax");
            response.SetStatus(400, "Bad Request");
            return NPT_SUCCESS;
        } else {
            if ((unsigned int)sep+1 < range.GetLength()) {
                result = NPT_ParseInteger64(range.GetChars()+sep+1, range_end);
                if (NPT_FAILED(result)) {
                    NPT_LOG_FINE("failed to parse range end");
                    return result;
                }
                range.SetLength(sep);
                has_end = true;
            }
            if (sep > 0) {
                result = range.ToInteger64(range_start);
                if (NPT_FAILED(result)) {
                    NPT_LOG_FINE("failed to parse range start");
                    return result;
                }
                has_start = true;
            }
            if (has_start) {
                if (!has_end) range_end = file_size-1; 
            } else {
                if (has_end) {
                    if (range_end <= file_size) {
                        range_start = file_size-range_end;
                        range_end = file_size-1;
                    }
                }
            }
            NPT_LOG_FINE_2("final range: start=%lld, end=%lld", range_start, range_end);
            if (range_start > range_end || range_end >= file_size) {
                NPT_LOG_FINE("out of range");
                satisfied = false;
            } else {
                satisfied = true;
            }
        } 
        if (satisfied && range_start != 0) {
            // seek in the stream
            result = stream->Seek(range_start);
            if (NPT_FAILED(result)) {
                NPT_LOG_WARNING_2("stream.Seek() failed (%d:%s)", result, NPT_ResultText(result));
                satisfied = false;
            }
        }
        if (!satisfied) {
            response.SetStatus(416, "Requested Range Not Satisfiable");
            NPT_String valid_range = "bytes */";
            valid_range += NPT_String::FromInteger(file_size);
            response.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONTENT_RANGE, valid_range.GetChars());
            return NPT_SUCCESS;            
        }
                
        // use a portion of the file stream as a body
        entity->SetInputStream(stream, false);
        entity->SetContentLength(range_end-range_start+1);
        response.SetStatus(206, "Partial Content");
        NPT_String valid_range = "bytes ";
        valid_range += NPT_String::FromInteger(range_start);
        valid_range += "-";
        valid_range += NPT_String::FromInteger(range_end);
        valid_range += "/";
        valid_range += NPT_String::FromInteger(file_size);
        response.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONTENT_RANGE, valid_range.GetChars());
    } else {
        // use the whole file stream as a body
        entity->SetInputStream(stream, true);
    }
    
    // set the response body
    entity->SetContentType(GetContentType(filename));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler::GetContentType
+---------------------------------------------------------------------*/
const char*
NPT_HttpFileRequestHandler::GetContentType(const NPT_String& filename)
{
    int last_dot = filename.ReverseFind('.');
    if (last_dot > 0) {
        NPT_String extension = filename.GetChars()+last_dot+1;
        extension.MakeLowercase();
        
        NPT_LOG_FINE_1("extension=%s", extension.GetChars());
        
        NPT_String* mime_type;
        if (NPT_SUCCEEDED(m_FileTypeMap.Get(extension, mime_type))) {
            NPT_LOG_FINE_1("found mime type in map: %s", mime_type->GetChars());
            return mime_type->GetChars();
        }

        // not found, look in the default map if necessary
        if (m_UseDefaultFileTypeMap) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(NPT_HttpFileRequestHandler_DefaultFileTypeMap); i++) {
                if (extension == NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].extension) {
                    const char* type = NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].mime_type;
                    NPT_LOG_FINE_1("using type from default list: %s", type); 
                    return type;
                }
            }
        }
    }

    NPT_LOG_FINE("using default mime type");
    return m_DefaultMimeType;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::NPT_HttpChunkedInputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedInputStream::NPT_HttpChunkedInputStream(
    NPT_BufferedInputStreamReference& stream) :
    m_Source(stream),
    m_CurrentChunkSize(0),
    m_Eos(false)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::~NPT_HttpChunkedInputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedInputStream::~NPT_HttpChunkedInputStream()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::NPT_HttpChunkedInputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpChunkedInputStream::Read(void*     buffer, 
                                 NPT_Size  bytes_to_read, 
                                 NPT_Size* bytes_read /* = NULL */)
{
    // set the initial state of return values
    if (bytes_read) *bytes_read = 0;

    // check for end of stream
    if (m_Eos) return NPT_ERROR_EOS;
    
    // shortcut
    if (bytes_to_read == 0) return NPT_SUCCESS;
    
    // read next chunk size if needed
    if (m_CurrentChunkSize == 0) {
        // buffered mode
        m_Source->SetBufferSize(4096);

        NPT_String size_line;
        NPT_CHECK(m_Source->ReadLine(size_line));

        // decode size (in hex)
        m_CurrentChunkSize = 0;
        if (size_line.GetLength() < 1) {
            NPT_LOG_WARNING("empty chunk size line");
            return NPT_ERROR_INVALID_FORMAT;
        }
        const char* size_hex = size_line.GetChars();
        while (*size_hex != '\0' &&
               *size_hex != ' '  && 
               *size_hex != ';'  && 
               *size_hex != '\r' && 
               *size_hex != '\n') {
            int nibble = NPT_HexToNibble(*size_hex);
            if (nibble < 0) {
                NPT_LOG_WARNING_1("invalid chunk size format (%s)", size_line.GetChars());
                return NPT_ERROR_INVALID_FORMAT;
            }
            m_CurrentChunkSize = (m_CurrentChunkSize<<4)|nibble;
            ++size_hex;
        }
        NPT_LOG_FINEST_1("start of chunk, size=%d", m_CurrentChunkSize);

        // 0 = end of body
        if (m_CurrentChunkSize == 0) {
            NPT_LOG_FINEST("end of chunked stream, reading trailers");
            
            // read footers until empty line
            NPT_String footer;
            do {
                NPT_CHECK(m_Source->ReadLine(footer));
            } while (!footer.IsEmpty());
            m_Eos = true;
            
            NPT_LOG_FINEST("end of chunked stream, done");
            return NPT_ERROR_EOS;
        }

        // unbuffer source
        m_Source->SetBufferSize(0);
    }

    // read no more than what's left in chunk
    NPT_Size chunk_bytes_read;
    if (bytes_to_read > m_CurrentChunkSize) bytes_to_read = m_CurrentChunkSize;
    NPT_CHECK(m_Source->Read(buffer, bytes_to_read, &chunk_bytes_read));

    // ready to go to next chunk?
    m_CurrentChunkSize -= chunk_bytes_read;
    if (m_CurrentChunkSize == 0) {
        NPT_LOG_FINEST("reading end of chunk");
        
        // when a chunk is finished, a \r\n follows
        char newline[2];
        NPT_CHECK(m_Source->ReadFully(newline, 2));
        if (newline[0] != '\r' || newline[1] != '\n') {
            NPT_LOG_WARNING("invalid end of chunk (expected \\r\\n)");
            return NPT_ERROR_INVALID_FORMAT;
        }
    }

    // update output params
    if (bytes_read) *bytes_read = chunk_bytes_read;
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedInputStream::Seek(NPT_Position /*offset*/)
{
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedInputStream::Tell(NPT_Position& offset)
{
    offset = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedInputStream::GetSize(NPT_LargeSize& size)
{
    return m_Source->GetSize(size);
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedInputStream::GetAvailable(NPT_LargeSize& available)
{
    return m_Source->GetAvailable(available);
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedOutputStream::NPT_HttpChunkedOutputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedOutputStream::NPT_HttpChunkedOutputStream(NPT_OutputStream& stream) :
    m_Stream(stream)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedOutputStream::~NPT_HttpChunkedOutputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedOutputStream::~NPT_HttpChunkedOutputStream()
{
    // zero size chunk followed by CRLF (no trailer)
    m_Stream.WriteFully("0" NPT_HTTP_LINE_TERMINATOR NPT_HTTP_LINE_TERMINATOR, 5);
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedOutputStream::Write(const void* buffer, 
                                   NPT_Size    bytes_to_write, 
                                   NPT_Size*   bytes_written)
{
    // default values
    if (bytes_written) *bytes_written = 0;
    
    // shortcut
    if (bytes_to_write == 0) return NPT_SUCCESS;
    
    // write the chunk header
    char size[16];
    size[15] = '\n';
    size[14] = '\r';
    char* c = &size[14];
    unsigned int char_count = 2;
    unsigned int value = bytes_to_write;
    do {
        unsigned int digit = (unsigned int)(value%16);
        if (digit < 10) {
            *--c = '0'+digit;
        } else {
            *--c = 'A'+digit-10;
        }
        char_count++;
        value /= 16;
    } while(value);
    NPT_Result result = m_Stream.WriteFully(c, char_count);
    if (NPT_FAILED(result)) return result;
    
    // write the chunk data
    result = m_Stream.WriteFully(buffer, bytes_to_write);
    if (NPT_FAILED(result)) return result;
    
    // finish the chunk
    result = m_Stream.WriteFully(NPT_HTTP_LINE_TERMINATOR, 2);
    if (NPT_SUCCEEDED(result) && bytes_written) {
        *bytes_written = bytes_to_write;
    }
    return result;
}
