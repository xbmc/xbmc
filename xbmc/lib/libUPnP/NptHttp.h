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

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_HTTP_DEFAULT_PORT = 80;
const unsigned int NPT_HTTP_INVALID_PORT = 0;

const int NPT_HTTP_PROTOCOL_MAX_LINE_LENGTH = 2048;

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
#define NPT_HTTP_HEADER_LOCATION            "Location"

const int NPT_ERROR_HTTP_INVALID_RESPONSE_LINE = NPT_ERROR_BASE_HTTP - 0;
const int NPT_ERROR_HTTP_INVALID_REQUEST_LINE  = NPT_ERROR_BASE_HTTP - 1;

#define NPT_HTTP_LINE_TERMINATOR "\r\n"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef unsigned int NPT_HttpStatusCode;

/*----------------------------------------------------------------------
|   NPT_HttpProtocol
+---------------------------------------------------------------------*/
class NPT_HttpProtocol
{
public:
    // class methods
    const char* GetSatusCodeString(NPT_HttpStatusCode status_code);
};

/*----------------------------------------------------------------------
|   NPT_HttpUrl
+---------------------------------------------------------------------*/
class NPT_HttpUrl : public NPT_Uri {
public:
    // constructors and destructor
    NPT_HttpUrl() : NPT_Uri("http:") {}
    NPT_HttpUrl(const char* url);
    NPT_HttpUrl(const char* host, NPT_UInt16 port, const char* path);

    // methods
    NPT_UInt16        GetPort() const { return m_Port; }
    const NPT_String& GetHost() const { return m_Host; }
    const NPT_String& GetPath() const { return m_Path; }
    bool              IsValid() const { return m_Port != NPT_HTTP_INVALID_PORT; }
    NPT_Result        SetHost(const char* host);
    NPT_Result        SetPath(const char* path);

private:
    // members
    NPT_String m_Host;
    NPT_UInt16 m_Port;
    NPT_String m_Path;

    // methods
    void RebuildUri();
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
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result Load(NPT_DataBuffer& buffer);
    NPT_Result SetHeaders(const NPT_HttpHeaders& headers);

    // field access
    NPT_Result        SetContentType(const char* type);
    NPT_Result        SetContentEncoding(const char* encoding);
    NPT_Result        SetContentLength(NPT_Size length);
    NPT_Size          GetContentLength()   { return m_ContentLength;   }
    const NPT_String& GetContentType()     { return m_ContentType;     }
    const NPT_String& GetContentEncoding() { return m_ContentEncoding; }

private:
    // members
    NPT_InputStreamReference m_InputStream;
    NPT_Size                 m_ContentLength;
    NPT_String               m_ContentType;
    NPT_String               m_ContentEncoding;
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
    NPT_HttpHeaders& GetHeaders() { 
        return m_Headers;  
    }
    NPT_Result SetEntity(NPT_HttpEntity* entity);
    NPT_HttpEntity* GetEntity() {
        return m_Entity;
    }
    virtual NPT_Result ParseHeaders(NPT_BufferedInputStream& stream,
                                    NPT_Timeout              timeout = NPT_TIMEOUT_INFINITE);
    virtual NPT_Result Emit(NPT_OutputStream& stream) const = 0;

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
                            NPT_HttpRequest*&        request,
                            NPT_Timeout              timeout = NPT_TIMEOUT_INFINITE);

    // constructors and destructor
    NPT_HttpRequest(const NPT_HttpUrl& url,
                    const char*        method,
                    const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    NPT_HttpRequest(const char*        url,
                    const char*        method,
                    const char*        protocol = NPT_HTTP_PROTOCOL_1_0);
    virtual ~NPT_HttpRequest();

    // methods
    const NPT_HttpUrl& GetUrl() { return m_Url; }
    NPT_Result         SetUrl(const char* url);
    const NPT_String&  GetMethod() const { return m_Method; }
    NPT_Result         Emit(NPT_OutputStream& stream) const;
    
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
                            NPT_HttpResponse*&       response,
                            NPT_Timeout              timeout = NPT_TIMEOUT_INFINITE);

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
    NPT_Result         Emit(NPT_OutputStream& stream) const;

protected:
    // members
    NPT_HttpStatusCode m_StatusCode;
    NPT_String         m_ReasonPhrase;
};

/*----------------------------------------------------------------------
|   NPT_HttpClient
+---------------------------------------------------------------------*/
class NPT_HttpClient {
public:
    // constructors and destructor
             NPT_HttpClient();
    virtual ~NPT_HttpClient();

    // methods
    NPT_Result SendRequest(NPT_HttpRequest&   request,
                           NPT_HttpResponse*& response,
                           NPT_Timeout        timeout = NPT_TIMEOUT_INFINITE);

protected:
    // methods
    NPT_Result SendRequestOnce(NPT_HttpRequest&   request,
                               NPT_HttpResponse*& response,
                               NPT_Timeout        timeout = NPT_TIMEOUT_INFINITE);

    // members
    bool m_FollowRedirect;
};

/*----------------------------------------------------------------------
|   NPT_HttpServer
+---------------------------------------------------------------------*/
class NPT_HttpServer {
public:
    // constructors and destructor
    NPT_HttpServer();
    virtual ~NPT_HttpServer();

    // methods
    NPT_Result WaitForRequest(NPT_HttpRequest*&          request,
                              NPT_OutputStreamReference& stream,
                              NPT_Timeout                timeout = NPT_TIMEOUT_INFINITE);
    NPT_Result SendResponse(NPT_HttpResponse& response, NPT_OutputStream& stream);

protected:
    // members
    NPT_UInt16 m_Port;
};

#endif // _NPT_HTTP_H_
