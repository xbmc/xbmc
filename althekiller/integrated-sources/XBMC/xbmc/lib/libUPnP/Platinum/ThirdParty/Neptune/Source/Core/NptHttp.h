/*****************************************************************
|
|   Neptune - HTTP Protocol
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_HTTP_H_
#define _NPT_HTTP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUri.h"
#include "NptTypes.h"
#include "NptList.h"
#include "NptBufferedStreams.h"
#include "NptSockets.h"
#include "NptMap.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_HTTP_DEFAULT_PORT = 80;
const unsigned int NPT_HTTP_INVALID_PORT = 0;

const NPT_Timeout NPT_HTTP_CLIENT_DEFAULT_CONNECTION_TIMEOUT    = 30000;
const NPT_Timeout NPT_HTTP_CLIENT_DEFAULT_IO_TIMEOUT            = 30000;
const NPT_Timeout NPT_HTTP_CLIENT_DEFAULT_NAME_RESOLVER_TIMEOUT = 60000;

const NPT_Timeout NPT_HTTP_SERVER_DEFAULT_CONNECTION_TIMEOUT    = NPT_TIMEOUT_INFINITE;
const NPT_Timeout NPT_HTTP_SERVER_DEFAULT_IO_TIMEOUT            = 60000;

const int NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH  = 8192;
const int NPT_HTTP_PROTOCOL_MAX_HEADER_COUNT = 100;

#define NPT_HTTP_PROTOCOL_1_0   "HTTP/1.0"
#define NPT_HTTP_PROTOCOL_1_1   "HTTP/1.1"
#define NPT_HTTP_METHOD_GET     "GET"
#define NPT_HTTP_METHOD_HEAD    "HEAD"
#define NPT_HTTP_METHOD_POST    "POST"

#define NPT_HTTP_HEADER_HOST                "Host"
#define NPT_HTTP_HEADER_CONNECTION          "Connection"
#define NPT_HTTP_HEADER_USER_AGENT          "User-Agent"
#define NPT_HTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define NPT_HTTP_HEADER_CONTENT_TYPE        "Content-Type"
#define NPT_HTTP_HEADER_CONTENT_ENCODING    "Content-Encoding"
#define NPT_HTTP_HEADER_TRANSFER_ENCODING   "Transfer-Encoding"
#define NPT_HTTP_HEADER_LOCATION            "Location"
#define NPT_HTTP_HEADER_RANGE               "Range"
#define NPT_HTTP_HEADER_CONTENT_RANGE       "Content-Range"
#define NPT_HTTP_HEADER_COOKIE              "Cookie"

const int NPT_ERROR_HTTP_INVALID_RESPONSE_LINE = NPT_ERROR_BASE_HTTP - 0;
const int NPT_ERROR_HTTP_INVALID_REQUEST_LINE  = NPT_ERROR_BASE_HTTP - 1;
const int NPT_ERROR_HTTP_NO_PROXY              = NPT_ERROR_BASE_HTTP - 2;

#define NPT_HTTP_LINE_TERMINATOR "\r\n"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef unsigned int NPT_HttpStatusCode;
typedef NPT_UrlQuery NPT_HttpUrlQuery; // for backward compatibility

/*----------------------------------------------------------------------
|   NPT_HttpUrl
+---------------------------------------------------------------------*/
class NPT_HttpUrl : public NPT_Url {
public:
    // constructors
    NPT_HttpUrl() {}
    NPT_HttpUrl(const char* host, 
                NPT_UInt16  port, 
                const char* path,
                const char* query = NULL,
                const char* fragment = NULL);
    NPT_HttpUrl(const char* url, bool ignore_scheme = false);

    // methods
    virtual NPT_String ToString(bool with_fragment = true) const;
};

/*----------------------------------------------------------------------
|   NPT_HttpProtocol
+---------------------------------------------------------------------*/
class NPT_HttpProtocol
{
public:
    // class methods
    const char* GetStatusCodeString(NPT_HttpStatusCode status_code);
};

/*----------------------------------------------------------------------
|   NPT_HttpHeader
+---------------------------------------------------------------------*/
class NPT_HttpHeader {
public:
    // constructors and destructor
    NPT_HttpHeader(const char* name, const char* value);
    ~NPT_HttpHeader();

    // methods
    NPT_Result        Emit(NPT_OutputStream& stream) const;
    const NPT_String& GetName()  const { return m_Name;  }
    const NPT_String& GetValue() const { return m_Value; }
    NPT_Result        SetName(const char* name);
    NPT_Result        SetValue(const char* value);

private:
    // members
    NPT_String m_Name;
    NPT_String m_Value;
};

/*----------------------------------------------------------------------
|   NPT_HttpHeaders
+---------------------------------------------------------------------*/
class NPT_HttpHeaders {
public:
    // constructors and destructor
     NPT_HttpHeaders();
    ~NPT_HttpHeaders();

    // methods
    NPT_Result Emit(NPT_OutputStream& stream) const;
    NPT_List<NPT_HttpHeader*>& GetHeaders() { return m_Headers; }
    NPT_HttpHeader* GetHeader(const char* name) const;
    NPT_Result SetHeader(const char* name, const char* value);
    NPT_Result GetHeaderValue(const char* name, NPT_String& value);
    NPT_Result AddHeader(const char* name, const char* value);

private:
    // members
    NPT_List<NPT_HttpHeader*> m_Headers;
};

/*----------------------------------------------------------------------
|   NPT_HttpEntity
+---------------------------------------------------------------------*/
class NPT_HttpEntity {
public:
    // constructors and destructor
             NPT_HttpEntity();
             NPT_HttpEntity(const NPT_HttpHeaders& headers);
    virtual ~NPT_HttpEntity();

    // methods
    NPT_Result SetInputStream(const NPT_InputStreamReference& stream,
                              bool update_content_length = false);
    NPT_Result SetInputStream(const void* data, NPT_Size size);
    NPT_Result SetInputStream(const NPT_String& string);
    NPT_Result SetInputStream(const char* string);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result Load(NPT_DataBuffer& buffer);

    // field access
    NPT_Result        SetContentType(const char* type);
    NPT_Result        SetContentEncoding(const char* encoding);
    NPT_Result        SetContentLength(NPT_Size length);
    NPT_Size          GetContentLength()   { return m_ContentLength;   }
    const NPT_String& GetContentType()     { return m_ContentType;     }
    const NPT_String& GetContentEncoding() { return m_ContentEncoding; }
    const NPT_String& GetTransferEncoding(){ return m_TransferEncoding; }

private:
    // members
    NPT_InputStreamReference m_InputStream;
    NPT_Size                 m_ContentLength;
    NPT_String               m_ContentType;
    NPT_String               m_ContentEncoding;
    NPT_String               m_TransferEncoding;
};

/*----------------------------------------------------------------------
|   NPT_HttpMessage
+---------------------------------------------------------------------*/
class NPT_HttpMessage {
public:
    // constructors and destructor
    virtual ~NPT_HttpMessage();

    // methods
    const NPT_String& GetProtocol() const { 
        return m_Protocol; 
    }
    NPT_Result SetProtocol(const char* protocol) {
        m_Protocol = protocol;
        return NPT_SUCCESS;
    }
    NPT_HttpHeaders& GetHeaders() { 
        return m_Headers;  
    }
    NPT_Result SetEntity(NPT_HttpEntity* entity);
    NPT_HttpEntity* GetEntity() {
        return m_Entity;
    }
    virtual NPT_Result ParseHeaders(NPT_BufferedInputStream& stream);

protected:
    // constructors
    NPT_HttpMessage(const char* protocol);

    // members
    NPT_String      m_Protocol;
    NPT_HttpHeaders m_Headers;
    NPT_HttpEntity* m_Entity;
};

/*----------------------------------------------------------------------
|   NPT_HttpRequest
+---------------------------------------------------------------------*/
class NPT_HttpRequest : public NPT_HttpMessage {
public:
    // class methods
    static NPT_Result Parse(NPT_BufferedInputStream& stream, 
                            const NPT_SocketAddress* endpoint,
                            NPT_HttpRequest*&        request);

    // constructors and destructor
    NPT_HttpRequest(const NPT_HttpUrl& url,
                    const char*        method,
                    const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    NPT_HttpRequest(const char*        url,
                    const char*        method,
                    const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    virtual ~NPT_HttpRequest();

    // methods
    const NPT_HttpUrl& GetUrl() const { return m_Url; }
    NPT_HttpUrl&       GetUrl()       { return m_Url; }
    NPT_Result         SetUrl(const char* url);
    NPT_Result         SetUrl(const NPT_HttpUrl& url);
    const NPT_String&  GetMethod() const { return m_Method; }
    virtual NPT_Result Emit(NPT_OutputStream& stream, bool use_proxy=false) const;
    
protected:
    // members
    NPT_HttpUrl m_Url;
    NPT_String  m_Method;
};

/*----------------------------------------------------------------------
|   NPT_HttpResponse
+---------------------------------------------------------------------*/
class NPT_HttpResponse : public NPT_HttpMessage {
public:
    // class methods
    static NPT_Result Parse(NPT_BufferedInputStream& stream, 
                            NPT_HttpResponse*&       response);

    // constructors and destructor
             NPT_HttpResponse(NPT_HttpStatusCode status_code,
                              const char*        reason_phrase,
                              const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    virtual ~NPT_HttpResponse();

    // methods
    NPT_Result         SetStatus(NPT_HttpStatusCode status_code,
                                 const char*        reason_phrase,
                                 const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    NPT_HttpStatusCode GetStatusCode()   { return m_StatusCode;   }
    NPT_String&        GetReasonPhrase() { return m_ReasonPhrase; }
    virtual NPT_Result Emit(NPT_OutputStream& stream) const;

protected:
    // members
    NPT_HttpStatusCode m_StatusCode;
    NPT_String         m_ReasonPhrase;
};

/*----------------------------------------------------------------------
|   NPT_HttpProxyAddress
+---------------------------------------------------------------------*/
class NPT_HttpProxyAddress
{
public:
    NPT_HttpProxyAddress() : m_Port(NPT_HTTP_INVALID_PORT) {}
    NPT_HttpProxyAddress(const char* hostname, NPT_UInt16 port) :
        m_HostName(hostname), m_Port(port) {}

    const NPT_String& GetHostName() const { return m_HostName; } 
    void              SetHostName(const char* hostname) { m_HostName = hostname; }
    NPT_UInt16        GetPort() const { return m_Port; }
    void              SetPort(NPT_UInt16 port) { m_Port = port; }

private:
    NPT_String m_HostName;
    NPT_UInt16 m_Port;
};

/*----------------------------------------------------------------------
|   NPT_HttpProxySelector
+---------------------------------------------------------------------*/
class NPT_HttpProxySelector
{
public:
    // class methods
    static NPT_HttpProxySelector* GetSystemDefault();

    // methods
    virtual ~NPT_HttpProxySelector() {};
    virtual NPT_Result GetProxyForUrl(const NPT_HttpUrl& url, NPT_HttpProxyAddress& proxy) = 0;
};

/*----------------------------------------------------------------------
|   NPT_HttpClient
+---------------------------------------------------------------------*/
class NPT_HttpClient {
public:
    // types
    struct Config {
        NPT_Timeout m_ConnectionTimeout;
        NPT_Timeout m_IoTimeout;
        NPT_Timeout m_NameResolverTimeout;
        bool        m_FollowRedirect;
    };

    class Connector {
    public:
        virtual ~Connector() {}

        virtual NPT_Result Connect(const char*                hostname, 
                                   NPT_UInt16                 port, 
                                   NPT_Timeout                connection_timeout,
                                   NPT_Timeout                io_timeout,
                                   NPT_Timeout                name_resolver_timeout,
                                   NPT_InputStreamReference&  input_stream,
                                   NPT_OutputStreamReference& output_stream) = 0;

    };

    /**
     * @param connector Pointer to a Connector instance, or NULL to use 
     * the default (TCP) connector.
     */
    NPT_HttpClient(Connector* connector = NULL);

    virtual ~NPT_HttpClient();

    // methods
    NPT_Result SendRequest(NPT_HttpRequest&   request,
                           NPT_HttpResponse*& response);
    NPT_Result SetConfig(const Config& config);
    NPT_Result SetProxy(const char* hostname, NPT_UInt16 port);
    NPT_Result SetProxySelector(NPT_HttpProxySelector* selector);
    NPT_Result SetConnector(Connector* connector);
    NPT_Result SetTimeouts(NPT_Timeout connection_timeout,
                           NPT_Timeout io_timeout,
                           NPT_Timeout name_resolver_timeout);

protected:
    // methods
    NPT_Result SendRequestOnce(NPT_HttpRequest&   request,
                               NPT_HttpResponse*& response);

    // members
    Config                 m_Config;
    NPT_HttpProxySelector* m_ProxySelector;
    bool                   m_ProxySelectorIsOwned;
    Connector*             m_Connector;
};

/*----------------------------------------------------------------------
|   NPT_HttpRequestHandler
+---------------------------------------------------------------------*/
class NPT_HttpRequestHandler 
{
public:
    // destructor
    virtual ~NPT_HttpRequestHandler() {}

    // methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&  request,
                                     NPT_HttpResponse& response,
                                     NPT_SocketInfo&   client_info) = 0;
};

/*----------------------------------------------------------------------
|   NPT_HttpStaticRequestHandler
+---------------------------------------------------------------------*/
class NPT_HttpStaticRequestHandler : public NPT_HttpRequestHandler
{
public:
    // constructors
    NPT_HttpStaticRequestHandler(const char* document, 
                                 const char* mime_type,
                                 bool        copy = true);
    NPT_HttpStaticRequestHandler(const void* data,
                                 NPT_Size    size,
                                 const char* mime_type,
                                 bool        copy = true);

    // NPT_HttpRequetsHandler methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&  request, 
                                     NPT_HttpResponse& response,
                                     NPT_SocketInfo&   client_info);

private:
    NPT_String     m_MimeType;
    NPT_DataBuffer m_Buffer;
};

/*----------------------------------------------------------------------
|   NPT_HttpFileRequestHandler
+---------------------------------------------------------------------*/
class NPT_HttpFileRequestHandler : public NPT_HttpRequestHandler
{
public:
    // constructors
    NPT_HttpFileRequestHandler(const char* url_root,
                               const char* file_root);

    // NPT_HttpRequetsHandler methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&  request, 
                                     NPT_HttpResponse& response,
                                     NPT_SocketInfo&   client_info);

    // accessors
    NPT_Map<NPT_String,NPT_String>& GetFileTypeMap() { return m_FileTypeMap; }
    void SetDefaultMimeType(const char* mime_type) {
        m_DefaultMimeType = mime_type;
    }
    void SetUseDefaultFileTypeMap(bool use_default) {
        m_UseDefaultFileTypeMap = use_default;
    }

protected:
    // methods
    const char* GetContentType(const NPT_String& filename);

private:
    NPT_String                      m_UrlRoot;
    NPT_String                      m_FileRoot;
    NPT_Map<NPT_String, NPT_String> m_FileTypeMap;
    NPT_String                      m_DefaultMimeType;
    bool                            m_UseDefaultFileTypeMap;
};

/*----------------------------------------------------------------------
|   NPT_HttpServer
+---------------------------------------------------------------------*/
class NPT_HttpServer {
public:
    // types
    struct Config {
        NPT_Timeout   m_ConnectionTimeout;
        NPT_Timeout   m_IoTimeout;
        NPT_IpAddress m_ListenAddress;
        NPT_UInt16    m_ListenPort;
        bool          m_ReuseAddress;
    };

    // constructors and destructor
    NPT_HttpServer(NPT_UInt16 listen_port = NPT_HTTP_DEFAULT_PORT,
                   bool       reuse_address = true);
    virtual ~NPT_HttpServer();

    // methods
    NPT_Result SetConfig(const Config& config);
    NPT_Result SetListenPort(NPT_UInt16 port, bool reuse_address = true);
    NPT_Result SetTimeouts(NPT_Timeout connection_timeout, NPT_Timeout io_timeout);
    void       Abort();
    NPT_Result WaitForNewClient(NPT_InputStreamReference&  input,
                                NPT_OutputStreamReference& output,
                                NPT_SocketInfo&            client_info);
    
    /**
     *  Add a request handler. The ownership of the handler is NOT transfered to this object,
     *  so the caller is responsible for the lifetime management of the handler object.
     */
    NPT_Result AddRequestHandler(NPT_HttpRequestHandler* handler, const char* path, bool include_children = false);
    NPT_HttpRequestHandler* FindRequestHandler(NPT_HttpRequest& request);

    /**
     * Parse the request from a new client, form a response, and send it back. 
     */
    NPT_Result RespondToClient(NPT_InputStreamReference&  input,
                               NPT_OutputStreamReference& output,
                               NPT_SocketInfo&            client_info);

protected:
    // types
    struct HandlerConfig {
        HandlerConfig(NPT_HttpRequestHandler* handler,
                      const char*             path,
                      bool                    include_children);
        ~HandlerConfig();

        // methods
        bool WillHandle(NPT_HttpRequest& request);

        // members
        NPT_HttpRequestHandler* m_Handler;
        NPT_String              m_Path;
        bool                    m_IncludeChildren;
    };

    // methods
    NPT_Result Bind();

    // members
    NPT_TcpServerSocket      m_Socket;
    NPT_UInt16               m_BoundPort;
    Config                   m_Config;
    NPT_List<HandlerConfig*> m_RequestHandlers;
};

/*----------------------------------------------------------------------
|   NPT_HttpResponder
+---------------------------------------------------------------------*/
class NPT_HttpResponder {
public:
    // types
    struct Config {
        NPT_Timeout m_IoTimeout;
    };

    // constructors and destructor
    NPT_HttpResponder(NPT_InputStreamReference&  input,
                      NPT_OutputStreamReference& output);
    virtual ~NPT_HttpResponder();

    // methods
    NPT_Result SetConfig(const Config& config);
    NPT_Result SetTimeout(NPT_Timeout io_timeout);
    NPT_Result ParseRequest(NPT_HttpRequest*&  request,
                            NPT_SocketAddress* local_address = NULL);
    NPT_Result SendResponse(NPT_HttpResponse& response,
                            bool              headers_only = false);

protected:
    // members
    Config                           m_Config;
    NPT_BufferedInputStreamReference m_Input;
    NPT_OutputStreamReference        m_Output;
};

/*----------------------------------------------------------------------
|   NPT_HttpChunkedDecoderInputStream
+---------------------------------------------------------------------*/
class NPT_HttpChunkedDecoderInputStream : public NPT_InputStream
{
public:
    // constructors and destructor
    NPT_HttpChunkedDecoderInputStream(NPT_BufferedInputStreamReference& stream);
    virtual ~NPT_HttpChunkedDecoderInputStream();

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read = NULL);
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);
    NPT_Result GetSize(NPT_Size& size);
    NPT_Result GetAvailable(NPT_Size& available);

protected:
    // members
    NPT_BufferedInputStreamReference m_Source;
    NPT_Size m_ChunkSize;
    bool     m_InChunk;
};

#endif // _NPT_HTTP_H_

