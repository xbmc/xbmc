/*****************************************************************
|
|   Platinum - HTTP Helper
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
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
| licensing@plutinosoft.com
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

/** @file
 HTTP utilities
 */

#ifndef _PLT_HTTP_H_
#define _PLT_HTTP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltVersion.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#if !defined(PLT_HTTP_DEFAULT_USER_AGENT)
#define PLT_HTTP_DEFAULT_USER_AGENT "UPnP/1.0 DLNADOC/1.50 Platinum/" PLT_PLATINUM_SDK_VERSION_STRING
#endif

#if !defined(PLT_HTTP_DEFAULT_SERVER)
#define PLT_HTTP_DEFAULT_SERVER "UPnP/1.0 DLNADOC/1.50 Platinum/" PLT_PLATINUM_SDK_VERSION_STRING
#endif

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
	PLT_DEVICE_UNKNOWN,
	PLT_DEVICE_XBOX_360,
	PLT_DEVICE_XBOX_ONE,
	PLT_DEVICE_PS3,
	PLT_DEVICE_WMP,
    PLT_DEVICE_SONOS,
    PLT_DEVICE_MAC,
    PLT_DEVICE_WINDOWS,
    PLT_DEVICE_VLC
} PLT_DeviceSignature;

/*----------------------------------------------------------------------
|   PLT_HttpHelper
+---------------------------------------------------------------------*/
/**
 The PLT_HttpHelper class is a set of utility functions for manipulating 
 HTTP headers, entities and messages.
 */
class PLT_HttpHelper {
public:
    static bool         IsConnectionKeepAlive(NPT_HttpMessage& message);
    static bool         IsBodyStreamSeekable(NPT_HttpMessage& message);

    static NPT_Result   GetContentType(const NPT_HttpMessage& message, NPT_String& type);
    static NPT_Result   GetContentLength(const NPT_HttpMessage& message, NPT_LargeSize& len);

    static NPT_Result   GetHost(const NPT_HttpRequest& request, NPT_String& value);
    static void         SetHost(NPT_HttpRequest& request, const char* host);
	static PLT_DeviceSignature GetDeviceSignature(const NPT_HttpRequest& request);

    static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_String& text, NPT_HttpEntity** entity = NULL);
    static NPT_Result   SetBody(NPT_HttpMessage& message, const char* text, NPT_HttpEntity** entity = NULL);
    static NPT_Result   SetBody(NPT_HttpMessage& message, const void* body, NPT_LargeSize len, NPT_HttpEntity** entity = NULL);
    static NPT_Result   SetBody(NPT_HttpMessage& message, NPT_InputStreamReference stream, NPT_HttpEntity** entity = NULL);
    static NPT_Result   GetBody(const NPT_HttpMessage& message, NPT_String& body);
    static NPT_Result   ParseBody(const NPT_HttpMessage& message, NPT_XmlElementNode*& xml);

	static void			SetBasicAuthorization(NPT_HttpRequest& request, const char* username, const char* password);
};

/*----------------------------------------------------------------------
|   PLT_HttpRequestContext
+---------------------------------------------------------------------*/
/** 
 The PLT_HttpRequestContext class holds information about the request sent, the
 local & remote ip addresses and ports associated with a connection. It is used
 mostly when processing a HTTP response.
 */
class PLT_HttpRequestContext : public NPT_HttpRequestContext {
public:
    // constructors and destructor
    PLT_HttpRequestContext(const NPT_HttpRequest& request) : 
        m_Request(request) {}
    PLT_HttpRequestContext(const NPT_HttpRequest& request, const NPT_HttpRequestContext& context) :
        NPT_HttpRequestContext(&context.GetLocalAddress(), &context.GetRemoteAddress()),
        m_Request(request) {}
    virtual ~PLT_HttpRequestContext() {}
    
    const NPT_HttpRequest& GetRequest() const { return m_Request; }
	PLT_DeviceSignature GetDeviceSignature() { return PLT_HttpHelper::GetDeviceSignature(m_Request); }
    
private:
    const NPT_HttpRequest& m_Request;
};

/*----------------------------------------------------------------------
|   NPT_HttpHeaderPrinter
+---------------------------------------------------------------------*/
class NPT_HttpHeaderPrinter
{
public:
	// methods
	NPT_HttpHeaderPrinter(NPT_OutputStreamReference& stream) :
	m_Stream(stream) {}
	NPT_Result operator()(NPT_HttpHeader*& header) const {
		m_Stream->WriteString(header->GetName());
		m_Stream->Write(": ", 2);
		m_Stream->WriteString(header->GetValue());
		m_Stream->Write("\r\n", 2, NULL);
		return NPT_SUCCESS;
	}

private:
	// members
	NPT_OutputStreamReference& m_Stream;
};

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_ENABLE_LOGGING)
#define PLT_LOG_HTTP_REQUEST_L(_logger, _level, _prefix, _request) \
do { \
    if (!_request) break; \
	NPT_LOG_GET_LOGGER((_logger))                                       \
	if ((_logger).logger && (_level) >= (_logger).logger->GetLevel()) { \
		NPT_StringOutputStreamReference stream(new NPT_StringOutputStream); \
		NPT_OutputStreamReference output = stream; \
		_request->GetHeaders().GetHeaders().Apply(NPT_HttpHeaderPrinter(output)); \
		NPT_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(NPT_LocalFunctionName),"%s\n%s %s %s\n%s", (const char*)_prefix, (const char*)_request->GetMethod(), (const char*)_request->GetUrl().ToRequestString(true), (const char*)_request->GetProtocol(), (const char*)stream->GetString())); \
	} \
} while (0)

#define PLT_LOG_HTTP_RESPONSE_L(_logger, _level, _prefix, _response) \
do { \
    if (!_response) break; \
	NPT_LOG_GET_LOGGER((_logger))                                       \
	if ((_logger).logger && (_level) >= (_logger).logger->GetLevel()) { \
		NPT_StringOutputStreamReference stream(new NPT_StringOutputStream); \
		NPT_OutputStreamReference output = stream; \
		_response->GetHeaders().GetHeaders().Apply(NPT_HttpHeaderPrinter(output)); \
		NPT_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(NPT_LocalFunctionName),"%s\n%s %d %s\n%s", (const char*)_prefix, (const char*)_response->GetProtocol(), _response->GetStatusCode(), (const char*)_response->GetReasonPhrase(), (const char*)stream->GetString())); \
	} \
} while (0)

#define PLT_LOG_HTTP_REQUEST(_level,_prefix,_request) PLT_LOG_HTTP_REQUEST_L(_NPT_LocalLogger,(_level),(_prefix),(_request))

#define PLT_LOG_HTTP_RESPONSE(_level,_prefix,_response) PLT_LOG_HTTP_RESPONSE_L(_NPT_LocalLogger,(_level),(_prefix),(_response))

#else /* NPT_CONFIG_ENABLE_LOGGING */
#define PLT_LOG_HTTP_REQUEST_L(_logger, _level, _prefix, _request)
#define PLT_LOG_HTTP_RESPONSE_L(_logger, _level, _prefix, _response)
#define PLT_LOG_HTTP_REQUEST(_level,_prefix,_request)
#define PLT_LOG_HTTP_RESPONSE(_level,_prefix,_response)
#endif /* NPT_CONFIG_ENABLE_LOGGING */

/*----------------------------------------------------------------------
|   PLT_HttpRequestHandler
+---------------------------------------------------------------------*/
/**
 The PLT_HttpRequestHandler class delegates the handling of the response of a
 received HTTP request by a HTTP Server.
 */
class PLT_HttpRequestHandler : public NPT_HttpRequestHandler
{
public:
    PLT_HttpRequestHandler(NPT_HttpRequestHandler* delegate) : 
        m_Delegate(delegate) {}
    ~PLT_HttpRequestHandler() override {}

    // NPT_HttpRequestHandler methods
    NPT_Result SetupResponse(NPT_HttpRequest&              request, 
                             const NPT_HttpRequestContext& context,
                             NPT_HttpResponse&             response) override {
        return m_Delegate->SetupResponse(request, context, response);
    }

private:
    NPT_HttpRequestHandler* m_Delegate;
};

#endif /* _PLT_HTTP_H_ */
