/*****************************************************************
|
|   Neptune - HTTP Protocol
|
|   (c) 2001-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
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
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_HTTP_MAX_REDIRECTS = 20;

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
|   NPT_HttpHeaders::Emit
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeaders::Emit(NPT_OutputStream& stream) const
{
    // for each header in the list
    NPT_List<NPT_HttpHeader*>::Iterator header = m_Headers.GetFirstItem();
    while (header) {
        // emit the header
        NPT_CHECK((*header)->Emit(stream));
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
NPT_HttpHeaders::SetHeader(const char* name, const char* value)
{
    NPT_HttpHeader* header = GetHeader(name);
    if (header == NULL) {
        return AddHeader(name, value);
    } else {
        return header->SetValue(value);
    }
}

/*----------------------------------------------------------------------
|   NPT_HttpHeaders::GetHeaderValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpHeaders::GetHeaderValue(const char* name, NPT_String& value)
{
    NPT_HttpHeader* header = GetHeader(name);
    if (header == NULL) {
        return NPT_ERROR_NO_SUCH_ITEM;
    } else {
        value = header->GetValue();
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::NPT_HttpEntity() :
    m_ContentLength(0)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::NPT_HttpEntity(const NPT_HttpHeaders& headers) :
    m_ContentLength(0)
{
    NPT_HttpHeader* header;
    
    // Content-Length
    header = headers.GetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH);
    if (header != NULL) {
        long length;
        if (NPT_SUCCEEDED(header->GetValue().ToInteger(length))) {
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
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::~NPT_HttpEntity
+---------------------------------------------------------------------*/
NPT_HttpEntity::~NPT_HttpEntity()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::GetInputStream(NPT_InputStreamReference& stream)
{
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
    return m_InputStream->Load(buffer, m_ContentLength);
}

/*----------------------------------------------------------------------
|   NPT_HttpEntity::SetContentLength
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpEntity::SetContentLength(NPT_Size length)
{
    m_ContentLength = length;
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
    delete m_Entity;
    m_Entity = entity;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::ParseHeaders
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpMessage::ParseHeaders(NPT_BufferedInputStream& stream)
{
    NPT_String header_name;
    NPT_String header_value;
    bool       header_pending = false;
    NPT_String line;

    while (NPT_SUCCEEDED(stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH))) {
        if (line.GetLength() == 0) {
            // empty line, end of headers
            if (header_pending) {
                header_value.Trim();
                m_Headers.AddHeader(header_name, header_value);
            }
            break;
        }
        char first_char = line[0];
        if ((first_char == ' ' || first_char == '\t') && header_pending) {
            header_value.Append(line.GetChars()+1, line.GetLength()-1);
        } else {
            // add the pending header to the list
            if (header_pending) {
                header_value.Trim();
                m_Headers.AddHeader(header_name, header_value);
                header_pending = false;
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

    return NPT_SUCCESS;
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
    NPT_CHECK(stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH));

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
    m_Protocol = protocol;
    m_StatusCode = status_code;
    m_ReasonPhrase = reason_phrase;
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
                        NPT_HttpResponse*&       response)
{
    // default return value
    response = NULL;

    // read the response line
    NPT_String line;
    NPT_CHECK(stream.ReadLine(line, NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH));
    
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
    long status_code_int = 0;
    status_code.ToInteger(status_code_int);
    response = new NPT_HttpResponse(status_code_int, reason_phrase, protocol);

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
    NPT_LOG_FINE_2("NPT_HttpTcpConnector::Connect - will connect to %s:%d\n", hostname, port);
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
    delete m_Connector;
    m_Connector = connector;
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

    // get the address and port to which we need to connect
    NPT_HttpProxyAddress proxy;
    bool                 use_proxy = false;
    if (m_ProxySelector) {
        // we have a proxy selector, ask it to select a proxy for this URL
        NPT_Result result = m_ProxySelector->GetProxyForUrl(request.GetUrl(), proxy);
        if (NPT_FAILED(result) && result != NPT_ERROR_HTTP_NO_PROXY) {
            NPT_LOG_WARNING_1("NPT_HttpClient::SendRequestOnce - proxy selector failure (%d)", result);
            return result;
        }
        use_proxy = true;
    }

    // decide which host we need to connect to
    const char* server_hostname;
    NPT_UInt16    server_port;
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
    NPT_CHECK_WARNING(m_Connector->Connect(server_hostname,
                                           server_port,
                                           m_Config.m_ConnectionTimeout,
                                           m_Config.m_IoTimeout,
                                           m_Config.m_NameResolverTimeout,
                                           input_stream,
                                           output_stream));

    // add any headers that may be missing
    NPT_HttpHeaders& headers = request.GetHeaders();
    if (!headers.GetHeader(NPT_HTTP_HEADER_CONNECTION)) {
        headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");
    }
    if (!headers.GetHeader(NPT_HTTP_HEADER_USER_AGENT)) {
        headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
                          "Neptune/" NPT_NEPTUNE_VERSION_STRING);
    }
    NPT_String host = request.GetUrl().GetHost();
    if (request.GetUrl().GetPort() != NPT_HTTP_DEFAULT_PORT) {
        host += ":";
        host += NPT_String::FromInteger(request.GetUrl().GetPort());
    }
    headers.SetHeader(NPT_HTTP_HEADER_HOST, host);

    // get the request entity to set additional headers
    NPT_InputStreamReference body_stream;
    NPT_HttpEntity* entity = request.GetEntity();
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

    // create a memory stream to buffer the headers
    NPT_MemoryStream header_stream;

    // emit the request headers into the header buffer
    request.Emit(header_stream, use_proxy);

    // send the headers
    NPT_CHECK(output_stream->WriteFully(header_stream.GetData(), header_stream.GetDataSize()));

    // send request body
    if (!body_stream.IsNull() && entity->GetContentLength()) {
        NPT_CHECK(NPT_StreamToStreamCopy(*body_stream.AsPointer(), *output_stream.AsPointer(), 0, entity->GetContentLength()));
    }

    // flush the output stream so that everything is sent to the server
    output_stream->Flush();

    // create a buffered stream for this socket stream
    NPT_BufferedInputStreamReference buffered_input_stream(new NPT_BufferedInputStream(input_stream));

    // parse the response
    NPT_CHECK(NPT_HttpResponse::Parse(*buffered_input_stream, response));

    // unbuffer the stream
    buffered_input_stream->SetBufferSize(0);

    // create an entity if one is expected in the response
    if (request.GetMethod() == NPT_HTTP_METHOD_GET || request.GetMethod() == NPT_HTTP_METHOD_POST) {
        NPT_HttpEntity* response_entity = new NPT_HttpEntity(response->GetHeaders());
        // Transfer-Encoding: chunked ?
        if (response_entity->GetTransferEncoding() == "chunked") {
            NPT_InputStreamReference body_stream(new NPT_HttpChunkedDecoderInputStream(buffered_input_stream));
            response_entity->SetInputStream((NPT_InputStreamReference)body_stream);
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
                    NPT_LOG_FINE_1("NPT_HttpClient::SendRequest - redirecting to %s\n", location->GetValue().GetChars());
                    keep_going = true;
                    delete response;
                    response = NULL;
                }
            }
        } else if (response && response->GetStatusCode() == 100) {
            NPT_LOG_FINE("NPT_HttpClient::SendRequest - received 100 continue\n");
            keep_going = true;
            delete response;
            response = NULL;
        }
    } while (keep_going && watchdog--);

    return result;
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
        NPT_SocketAddress(NPT_IpAddress::Any, m_Config.m_ListenPort), 
        m_Config.m_ReuseAddress);
    if (NPT_FAILED(result)) return result;

    // remember that we're bound
    m_BoundPort = m_Config.m_ListenPort;
    
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
void
NPT_HttpServer::Abort()
{
    m_Socket.Disconnect();
}

/*----------------------------------------------------------------------
|   NPT_HttpServer::WaitForNewClient
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpServer::WaitForNewClient(NPT_InputStreamReference&  input,
                                 NPT_OutputStreamReference& output,
                                 NPT_SocketInfo&            client_info)
{
    // ensure that we're bound 
    NPT_CHECK(Bind());

    // wait for a connection
    NPT_Socket*         client;
    NPT_LOG_FINE_1("NPT_HttpServer::WaitForRequest - waiting for connection on port %d...\n", m_Config.m_ListenPort);
    NPT_CHECK(m_Socket.WaitForNewClient(client, m_Config.m_ConnectionTimeout));
    if (client == NULL) return NPT_ERROR_INTERNAL;

    // get the client info
    client->GetInfo(client_info);

    NPT_LOG_FINE_2("NPT_HttpServer::WaitForRequest - client connected (%s)\n",
                client_info.local_address.ToString().GetChars(),
                client_info.remote_address.ToString().GetChars());

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
NPT_HttpServer::RespondToClient(NPT_InputStreamReference&  input,
                                NPT_OutputStreamReference& output,
                                NPT_SocketInfo&            client_info)
{
    NPT_HttpRequest*  request;
    NPT_HttpResponse* response;
    NPT_Result        result = NPT_SUCCESS;

    NPT_HttpResponder responder(input, output);
    NPT_CHECK(responder.ParseRequest(request, &(client_info.local_address)));

    NPT_HttpRequestHandler* handler = FindRequestHandler(*request);
    if (handler == NULL) {
        response = new NPT_HttpResponse(404, "Not Found", NPT_HTTP_PROTOCOL_1_0);
    } else {
        // create a response object
        response = new NPT_HttpResponse(200, "OK", NPT_HTTP_PROTOCOL_1_0);

        // prepare the response
        response->SetEntity(new NPT_HttpEntity());

        // ask the handler to setup the response
        if (NPT_FAILED(handler->SetupResponse(*request, *response, client_info))) {
            response->SetStatus(500, "Internal Server Error");
        }
    }

    // send the response
    result = responder.SendResponse(*response, request->GetMethod()==NPT_HTTP_METHOD_HEAD);

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
|   NPT_HttpResponder::SetTimeout
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::ParseRequest(NPT_HttpRequest*&  request,
                                NPT_SocketAddress* local_address /* = NULL */)
{
    // parse the request
    NPT_CHECK(NPT_HttpRequest::Parse(*m_Input, local_address, request));

    // unbuffer the stream
    m_Input->SetBufferSize(0);

    // set the entity info
    NPT_HttpEntity* entity = new NPT_HttpEntity(request->GetHeaders());
    entity->SetInputStream(m_Input);
    request->SetEntity(entity);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponder::SendResponse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpResponder::SendResponse(NPT_HttpResponse& response, 
                                bool              headers_only)
{
    // add default headers
    NPT_HttpHeaders& headers = response.GetHeaders();
    headers.SetHeader(NPT_HTTP_HEADER_CONNECTION, "close");

    // add computed headers
    NPT_HttpEntity* entity = response.GetEntity();
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

    // create a memory stream to buffer the response line and headers
    NPT_MemoryStream buffer;

    // emit the response line
    NPT_CHECK(response.Emit(buffer));

    // send the buffer
    NPT_CHECK(m_Output->WriteFully(buffer.GetData(), buffer.GetDataSize()));

    // send the body
    if (!headers_only && entity) {
        NPT_InputStreamReference body_stream;
        entity->GetInputStream(body_stream);
        if (!body_stream.IsNull()) return NPT_StreamToStreamCopy(*body_stream, *m_Output);
    }

    // flush
    m_Output->Flush();

    return NPT_SUCCESS;
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
NPT_HttpStaticRequestHandler::SetupResponse(NPT_HttpRequest&  /*request*/, 
                                            NPT_HttpResponse& response,
                                            NPT_SocketInfo&   /*client_info*/)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;

    entity->SetContentType(m_MimeType);
    entity->SetInputStream(m_Buffer.GetData(), m_Buffer.GetDataSize());

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler_DefaultFileTypeMap
+---------------------------------------------------------------------*/
struct NPT_HttpFileRequestHandler_DefaultFileTypeMapEntry {
    const char* extension;
    const char* mime_type;
};
static const NPT_HttpFileRequestHandler_DefaultFileTypeMapEntry 
NPT_HttpFileRequestHandler_DefaultFileTypeMap[] = {
    {"xml", "text/html"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"gif",  "image/gif"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"jpe",  "image/jpeg"},
    {"png",  "image/png"},
    {"bmp",  "image/bmp"},
    {"css",  "text/css"},
    {"js",   "application/javascript"}
};

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler::NPT_HttpFileRequestHandler
+---------------------------------------------------------------------*/
NPT_HttpFileRequestHandler::NPT_HttpFileRequestHandler(const char* url_root,
                                                       const char* file_root) :
    m_UrlRoot(url_root),
    m_FileRoot(file_root),
    m_DefaultMimeType("text/html"),
    m_UseDefaultFileTypeMap(true)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpFileRequestHandler::SetupResponse(NPT_HttpRequest&  request, 
                                          NPT_HttpResponse& response,
                                          NPT_SocketInfo&   /*client_info*/)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;

    // check the method
    if (request.GetMethod() != NPT_HTTP_METHOD_GET &&
        request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
        response.SetStatus(405, "Method Not Allowed");
        return NPT_SUCCESS;
    }

    // TODO: we need to normalize the request path

    // check that the request's path is an entry under the url root
    if (!request.GetUrl().GetPath().StartsWith(m_UrlRoot)) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // compute the filename
    NPT_String filename = m_FileRoot;
    filename += request.GetUrl().GetPath().GetChars()+m_UrlRoot.GetLength();
    
    // open the file
    NPT_File file(filename);
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
    if (NPT_FAILED(result)) {
        response.SetStatus(404, "Not Found");
        return NPT_SUCCESS;
    }
    NPT_InputStreamReference stream;
    file.GetInputStream(stream);
    entity->SetContentType(GetContentType(filename));
    entity->SetInputStream(stream, true);

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
        NPT_String extension = filename.GetChars()+filename.GetLength()-last_dot;
        extension.MakeLowercase();
        NPT_String* mime_type;
        if (NPT_SUCCEEDED(m_FileTypeMap.Get(extension, mime_type))) {
            return mime_type->GetChars();
        }

        // not found, look in the default map if necessary
        if (m_UseDefaultFileTypeMap) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(NPT_HttpFileRequestHandler_DefaultFileTypeMap); i++) {
                if (extension == NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].extension) {
                    return NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].mime_type;
                }
            }
        }
    }

    return m_DefaultMimeType;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::NPT_HttpChunkedDecoderInputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedDecoderInputStream::NPT_HttpChunkedDecoderInputStream(
    NPT_BufferedInputStreamReference& stream) :
    m_Source(stream),
    m_ChunkSize(0),
    m_InChunk(false)
{
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::~NPT_HttpChunkedDecoderInputStream
+---------------------------------------------------------------------*/
NPT_HttpChunkedDecoderInputStream::~NPT_HttpChunkedDecoderInputStream()
{
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::NPT_HttpChunkedDecoderInputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_HttpChunkedDecoderInputStream::Read(void*     buffer, 
                                        NPT_Size  bytes_to_read, 
                                        NPT_Size* bytes_read /* = NULL */)
{
    // update output params first
    if (bytes_read) *bytes_read = 0;

    // read next chunk size
    if (!m_InChunk) {
        // buffered mode
        m_Source->SetBufferSize(4096);

        NPT_String size_line;
        NPT_CHECK(m_Source->ReadLine(size_line));

        // decode line
        int delimiter = size_line.Find(';');
        if (delimiter) size_line = size_line.Left(delimiter);
        long size;
        NPT_CHECK(NPT_ParseInteger(size_line, size));

        // 0 = end of body
        if (size == 0) {
            // read footers until empty line
            NPT_String footer;
            do {
                NPT_CHECK(m_Source->ReadLine(footer));
                if (footer.IsEmpty()) return NPT_ERROR_EOS;
            } while (1);
        }

        // read chunk now
        m_ChunkSize = size;
        m_InChunk = false;

        // unbuffer source
        m_Source->SetBufferSize(0);
    }

    // read no more than what's left in chunk
    NPT_Size tmp;
    bytes_to_read = (m_ChunkSize>bytes_to_read)?bytes_to_read:m_ChunkSize;
    NPT_CHECK(m_Source->Read(buffer, bytes_to_read, &tmp));

    // ready to go to next chunk?
    m_ChunkSize -= tmp;
    if (m_ChunkSize == 0) {
        m_InChunk = true;

        // when a chunk is finished, a \r\n follows
        NPT_String tmp;
        NPT_CHECK(m_Source->ReadLine(tmp));
    }

    // update output params
    if (bytes_read) *bytes_read = tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedDecoderInputStream::Seek(NPT_Position /*offset*/)
{
    // not implemented yet
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedDecoderInputStream::Tell(NPT_Position& offset)
{
    // not implemented yet
    offset = 0;
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedDecoderInputStream::GetSize(NPT_Size& size)
{
    return m_Source->GetSize(size);
}

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_HttpChunkedDecoderInputStream::GetAvailable(NPT_Size& available)
{
    return m_Source->GetAvailable(available);
}
