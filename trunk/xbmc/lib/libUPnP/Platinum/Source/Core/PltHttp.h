/*****************************************************************
|
|   Platinum - HTTP Helper
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
    static bool         IsConnectionKeepAlive(NPT_HttpMessage& message);
    static bool         IsBodyStreamSeekable(NPT_HttpMessage& message);

    static NPT_Result   ToLog(NPT_LoggerReference logger, int level, NPT_HttpRequest* request);
    static NPT_Result   ToLog(NPT_LoggerReference logger, int level, NPT_HttpResponse* response);

    static NPT_Result   GetContentType(NPT_HttpMessage& message, NPT_String& type);
    static     void     SetContentType(NPT_HttpMessage& message, const char* type);
    static NPT_Result   GetContentLength(NPT_HttpMessage& message, NPT_LargeSize& len);
    static void         SetContentLength(NPT_HttpMessage& message, NPT_LargeSize len);

    static NPT_Result   GetHost(NPT_HttpRequest& request, NPT_String& value);
    static void         SetHost(NPT_HttpRequest& request, const char* host);
    static NPT_Result   GetRange(NPT_HttpRequest& request, NPT_Position& start, NPT_Position& end);
    static void         SetRange(NPT_HttpRequest& request, NPT_Position start, NPT_Position end = (NPT_Position)-1);

    static NPT_Result   GetContentRange(NPT_HttpResponse& response, NPT_Position& start, NPT_Position& end, NPT_LargeSize& length);
    static NPT_Result   SetContentRange(NPT_HttpResponse& response, NPT_Position start, NPT_Position end, NPT_LargeSize length);

    static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_String& body);
    static NPT_Result   SetBody(NPT_HttpMessage& message, const char* body, NPT_Size len);
    static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_InputStreamReference& stream, NPT_LargeSize len = 0);


    static NPT_Result   GetBody(NPT_HttpMessage& message, NPT_String& body);
    static NPT_Result   ParseBody(NPT_HttpMessage& message, NPT_XmlElementNode*& xml);

    static NPT_Result   Connect(NPT_Socket&      connection,
                                NPT_HttpRequest& request,
                                NPT_Timeout      timeout = NPT_TIMEOUT_INFINITE);
	static void			SetBasicAuthorization(NPT_HttpRequest& request, const char* login, const char* password);
};

/*----------------------------------------------------------------------
|   PLT_HttpRequestContext
+---------------------------------------------------------------------*/
class PLT_HttpRequestContext : public NPT_HttpRequestContext {
public:
    // constructors and destructor
    PLT_HttpRequestContext(NPT_HttpRequest& request) : 
        m_Request(request) {}
    PLT_HttpRequestContext(NPT_HttpRequest& request, const NPT_HttpRequestContext& context) :
        NPT_HttpRequestContext(&context.GetLocalAddress(), &context.GetRemoteAddress()),
        m_Request(request) {}
    virtual ~PLT_HttpRequestContext() {}
    
    NPT_HttpRequest& GetRequest() const { return m_Request; }
    
private:
    NPT_HttpRequest& m_Request;
};

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_ENABLE_LOGGING)
#define PLT_LOG_HTTP_MESSAGE_L(_logger, _level, _msg) \
    PLT_HttpHelper::ToLog((_logger), (_level), (_msg))
#define PLT_LOG_HTTP_MESSAGE(_level, _msg) \
	PLT_HttpHelper::ToLog((_NPT_LocalLogger), (_level), (_msg))

#else /* NPT_CONFIG_ENABLE_LOGGING */
#define PLT_LOG_HTTP_MESSAGE_L(_logger, _level, _msg)
#define PLT_LOG_HTTP_MESSAGE(_level, _msg)
#endif /* NPT_CONFIG_ENABLE_LOGGING */

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
    NPT_Result SetupResponse(NPT_HttpRequest&              request, 
                             const NPT_HttpRequestContext& context,
                             NPT_HttpResponse&             response) {
        return m_Data->ProcessHttpRequest(request, context, response);
    }

private:
    T* m_Data;
};


#endif /* _PLT_HTTP_H_ */
