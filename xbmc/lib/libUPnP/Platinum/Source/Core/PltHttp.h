/*****************************************************************
|
|   Platinum - HTTP Helper
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_HTTP_H_
#define _PLT_HTTP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_HttpHelper
+---------------------------------------------------------------------*/
class PLT_HttpHelper {
 public:
    static bool         IsConnectionKeepAlive(NPT_HttpMessage* message);
    static bool         IsBodyStreamSeekable(NPT_HttpMessage* message);

    static NPT_Result   ToLog(NPT_LoggerReference logger, int level, NPT_HttpRequest* request);
    static NPT_Result   ToLog(NPT_LoggerReference logger, int level, NPT_HttpResponse* response);

    static NPT_Result   GetContentType(NPT_HttpMessage* message, NPT_String& type);
    static     void     SetContentType(NPT_HttpMessage* message, const char* type);
    static NPT_Result   GetContentLength(NPT_HttpMessage* message, NPT_Size& len);
    static void         SetContentLength(NPT_HttpMessage* message, NPT_Size len);

    static NPT_Result   GetHost(NPT_HttpRequest* request, NPT_String& value);
    static void         SetHost(NPT_HttpRequest* request, const char* host);
    static NPT_Result   GetRange(NPT_HttpRequest* request, NPT_Integer& start, NPT_Integer& end);
    static void         SetRange(NPT_HttpRequest* request, NPT_Integer start, NPT_Integer end = -1);

    static NPT_Result   GetContentRange(NPT_HttpResponse* response, NPT_Integer& start, NPT_Integer& end, NPT_Integer& length);
    static NPT_Result   SetContentRange(NPT_HttpResponse* response, NPT_Integer start, NPT_Integer end, NPT_Integer length);

    static NPT_Result   SetBody(NPT_HttpMessage* message, NPT_String& body);
    static NPT_Result   SetBody(NPT_HttpMessage* message, const char* body, NPT_Size len);
    static NPT_Result   SetBody(NPT_HttpMessage* message, NPT_InputStreamReference& stream, NPT_Size len = 0);


    static NPT_Result   GetBody(NPT_HttpMessage* message, NPT_String& body);
    static NPT_Result   ParseBody(NPT_HttpMessage* message, NPT_XmlElementNode*& xml);
};

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_ENABLE_LOGGING)
#define PLT_LOG_HTTP_MESSAGE_L(_logger, _level, _msg) \
    PLT_HttpHelper::ToLog(_logger, _level, _msg)
#define PLT_LOG_HTTP_MESSAGE(_level, _msg) \
    PLT_LOG_HTTP_MESSAGE_L(_NPT_LocalLogger, _level, _msg)

#else /* NPT_CONFIG_ENABLE_LOGGING */
#define PLT_LOG_HTTP_MESSAGE_L(_logger, _level, _msg)
#define PLT_LOG_HTTP_MESSAGE(_level, _msg)
#endif /* NPT_CONFIG_ENABLE_LOGGING */

/*----------------------------------------------------------------------
|   PLT_HttpClient
+---------------------------------------------------------------------*/
class PLT_HttpClient {
public:
    // constructors and destructor
    PLT_HttpClient() {}
    virtual ~PLT_HttpClient() {}

    // methods
    NPT_Result Connect(NPT_Socket*       connection,
                       NPT_HttpRequest&  request,
                       NPT_Timeout       timeout = NPT_TIMEOUT_INFINITE);

    NPT_Result SendRequest(NPT_OutputStreamReference& output_stream, 
                           NPT_HttpRequest&           request,
                           NPT_Timeout                timeout = NPT_TIMEOUT_INFINITE);

    NPT_Result WaitForResponse(NPT_InputStreamReference& input_stream,
                               NPT_HttpRequest&          request, 
                               NPT_SocketInfo&           info, 
                               NPT_HttpResponse*&        response);

protected:
    // members
};

/*----------------------------------------------------------------------
|   PLT_HttpRequestHandler
+---------------------------------------------------------------------*/
template <class T>
class PLT_HttpRequestHandler : public NPT_HttpRequestHandler
{
public:
    PLT_HttpRequestHandler<T>(T* data) : m_Data(data) {}
    virtual ~PLT_HttpRequestHandler<T>() {}

    // NPT_HttpRequestHandler methods
    NPT_Result SetupResponse(NPT_HttpRequest&  request, 
                             NPT_HttpResponse& response, 
                             NPT_SocketInfo&   client_info) {
        return m_Data->ProcessHttpRequest(request, response, client_info);
    }

private:
    T* m_Data;
};


#endif /* _PLT_HTTP_H_ */
