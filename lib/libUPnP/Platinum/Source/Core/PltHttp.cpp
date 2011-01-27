/*****************************************************************
|
|   Platinum - HTTP Protocol Helper
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
#include "PltHttp.h"
#include "PltDatagramStream.h"
#include "PltVersion.h"

NPT_SET_LOCAL_LOGGER("platinum.core.http")

/*----------------------------------------------------------------------
|   external references
+---------------------------------------------------------------------*/
extern NPT_String HttpServerHeader;

/*----------------------------------------------------------------------
|   NPT_HttpHeaderFinder
+---------------------------------------------------------------------*/
class NPT_HttpHeaderFinder
{
 public:
    // methods
    NPT_HttpHeaderFinder(const char* name) : m_Name(name) {}
    bool operator()(const NPT_HttpHeader* const & header) const {
		if (header->GetName().Compare(m_Name, true)) {
			return true;
		} else {
			return false;
		}
    }

 private:
    // members
    NPT_String m_Name;
};

/*----------------------------------------------------------------------
|   NPT_HttpHeaderPrinter
+---------------------------------------------------------------------*/
class NPT_HttpHeaderPrinter
{
public:
    // methods
    NPT_HttpHeaderPrinter(NPT_OutputStreamReference& stream) : m_Stream(stream) {}
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
|   NPT_HttpHeaderLogger
+---------------------------------------------------------------------*/
class NPT_HttpHeaderLogger
{
public:
    // methods
    NPT_HttpHeaderLogger(NPT_LoggerReference& logger, int level) : 
      m_Logger(logger), m_Level(level) {}
	NPT_Result operator()(NPT_HttpHeader*& header) const {
        NPT_COMPILER_UNUSED(header);

        NPT_LOG_L2(m_Logger, m_Level, "%s: %s", 
            (const char*)header->GetName(), 
            (const char*)header->GetValue());
        return NPT_SUCCESS;
    }

    NPT_LoggerReference& m_Logger;
    int                  m_Level;
};


/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentType
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentType(NPT_HttpMessage& message, NPT_String& type) 
{ 
    type = "";

    const NPT_String* val = 
        message.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_CONTENT_TYPE);
    NPT_CHECK_POINTER(val);

    type = *val;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentType
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetContentType(NPT_HttpMessage& message, const char* type)   
{
    message.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONTENT_TYPE, type);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentLength
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentLength(NPT_HttpMessage& message, NPT_LargeSize& len) 
{ 
    len = 0;

    const NPT_String* val = 
        message.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_CONTENT_LENGTH);
    NPT_CHECK_POINTER(val);

    return val->ToInteger64(len);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentLength
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetContentLength(NPT_HttpMessage& message, NPT_LargeSize len)   
{
    message.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONTENT_LENGTH, NPT_String::FromIntegerU(len));
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage& message, NPT_String& body)
{
    return SetBody(message, (const char*)body, body.GetLength());
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage& message, const char* body, NPT_Size len)
{
    if (len == 0) {
        return NPT_SUCCESS;
    }

    // dump the body in a memory stream
    NPT_MemoryStreamReference stream(new NPT_MemoryStream);
    stream->Write(body, len);

    // set content length
    PLT_HttpHelper::SetContentLength(message, len);

    NPT_InputStreamReference input = stream;
    return SetBody(message, input, len);
}

/*----------------------------------------------------------------------
|   NPT_HttpMessage::SetBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetBody(NPT_HttpMessage& message, NPT_InputStreamReference& stream, NPT_LargeSize len)
{
    if (len == 0) {
        NPT_CHECK_SEVERE(stream->GetAvailable(len));
    }

    // get the entity
    NPT_HttpEntity* entity = message.GetEntity();
    if (entity == NULL) {
        // no entity yet, create one
        message.SetEntity(entity = new NPT_HttpEntity());
    }

    // set the entity body
    entity->SetInputStream(stream);
    entity->SetContentLength(len);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetBody
+---------------------------------------------------------------------*/
NPT_Result 
PLT_HttpHelper::GetBody(NPT_HttpMessage& message, NPT_String& body) 
{
    NPT_Result res;
    NPT_InputStreamReference stream;

    // get stream
    NPT_HttpEntity* entity = message.GetEntity();
    if (!entity || NPT_FAILED(entity->GetInputStream(stream)) || stream.IsNull()) {
        return NPT_FAILURE;
    }

    // extract body
    NPT_StringOutputStream* output_stream = new NPT_StringOutputStream(&body);
    res = NPT_StreamToStreamCopy(*stream, *output_stream, 0, entity->GetContentLength());
    delete output_stream;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::ParseBody
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ParseBody(NPT_HttpMessage& message, NPT_XmlElementNode*& tree) 
{
    // reset tree
    tree = NULL;

    // read body
    NPT_String body;
    NPT_CHECK_WARNING(GetBody(message, body));

    // parse body
    NPT_XmlParser parser;
    NPT_XmlNode*  node;
    NPT_CHECK_WARNING(parser.Parse(body, node));
    
    tree = node->AsElementNode();
    if (!tree) {
        delete node;
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::IsConnectionKeepAlive
+---------------------------------------------------------------------*/
bool
PLT_HttpHelper::IsConnectionKeepAlive(NPT_HttpMessage& message) 
{
    const NPT_String* connection = 
        message.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_CONNECTION);

    // the DLNA says that all HTTP 1.0 requests should be closed immediately by the server
    // all HTTP 1.1 without a Connection header or without a Connection header saying "Close" should be kept alive
    NPT_String protocol = message.GetProtocol();
    if (!protocol.Compare(NPT_HTTP_PROTOCOL_1_1, true) && (!connection || connection->Compare("close", true))) {
        return true; 
    }

    return false;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::IsBodyStreamSeekable
+---------------------------------------------------------------------*/
bool
PLT_HttpHelper::IsBodyStreamSeekable(NPT_HttpMessage& message)
{
    NPT_HttpEntity* entity = message.GetEntity();
    NPT_InputStreamReference stream;
    if (!entity || NPT_FAILED(entity->GetInputStream(stream)) || stream.IsNull()) return true;

    // try to get current position and seek there
    NPT_Position position;
    if (NPT_FAILED(stream->Tell(position)) || 
        NPT_FAILED(stream->Seek(position))) {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetHost
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetHost(NPT_HttpRequest& request, NPT_String& value)    
{ 
    value = "";

    const NPT_String* val = 
        request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_HOST);
    NPT_CHECK_POINTER(val);

    value = *val;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetHost
+---------------------------------------------------------------------*/
void         
PLT_HttpHelper::SetHost(NPT_HttpRequest& request, const char* host)
{ 
    request.GetHeaders().SetHeader(NPT_HTTP_HEADER_HOST, host); 
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetRange(NPT_HttpRequest& request, 
                         NPT_Position&    start, 
                         NPT_Position&    end)
{
    start = (NPT_Position)-1;
    end = (NPT_Position)-1;    
    
    const NPT_String* range = 
        request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_RANGE);
    NPT_CHECK_POINTER(range);

    char s[32], e[32];
    s[0] = '\0';
    e[0] = '\0';
    int ret = sscanf(*range, "bytes=%31[^-]-%31s", s, e);
    if (ret < 1) {
        return NPT_FAILURE;
    }
    if (s[0] != '\0') {
        NPT_ParseInteger64(s, start);
    }
    if (e[0] != '\0') {
        NPT_ParseInteger64(e, end);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetRange
+---------------------------------------------------------------------*/
void
PLT_HttpHelper::SetRange(NPT_HttpRequest& request, NPT_Position start, NPT_Position end)
{
    NPT_String range = "bytes=";
    if (start != (NPT_Position)-1) {
        range += NPT_String::FromIntegerU(start);
    }
    range += '-';
    if (end != (NPT_Position)-1) {
        range += NPT_String::FromIntegerU(end);
    }
    request.GetHeaders().SetHeader(NPT_HTTP_HEADER_RANGE, range);
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ToLog(NPT_LoggerReference logger, int level, NPT_HttpRequest* request)
{
    NPT_COMPILER_UNUSED(logger);
    NPT_COMPILER_UNUSED(level);

    if (!request) {
        NPT_LOG_L(logger, level, "NULL HTTP Request!");
        return NPT_FAILURE;
    }

    NPT_StringOutputStreamReference stream(new NPT_StringOutputStream);
    NPT_OutputStreamReference output = stream;
    request->GetHeaders().GetHeaders().Apply(NPT_HttpHeaderPrinter(output));

    NPT_LOG_L4(logger, level, "\n%s %s %s\n%s", 
        (const char*)request->GetMethod(), 
        (const char*)request->GetUrl().ToRequestString(true), 
        (const char*)request->GetProtocol(),
        (const char*)stream->GetString());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::GetContentRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::GetContentRange(NPT_HttpResponse& response, 
                                NPT_Position&     start, 
                                NPT_Position&     end, 
                                NPT_LargeSize&    length)
{
    const NPT_String* range = 
        response.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_CONTENT_RANGE);
    NPT_CHECK_POINTER(range);

    start  = (NPT_Position)-1;
    end    = (NPT_Position)-1;
    length = (NPT_LargeSize)-1;

    char s[32], e[32], l[32];
    s[0] = '\0';
    e[0] = '\0';
    l[0] = '\0';
    int ret = sscanf(*range, "bytes %31[^-]-%31s[^/]/%31s", s, e, l);
    if (ret < 3) {
        return NPT_FAILURE;
    }
    if (s[0] != '\0') {
        NPT_ParseInteger64(s, start);
    }
    if (e[0] != '\0') {
        NPT_ParseInteger64(e, end);
    }
    if (l[0] != '\0') {
        NPT_ParseInteger64(l, length);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetContentRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::SetContentRange(NPT_HttpResponse& response, 
                                NPT_Position      start, 
                                NPT_Position      end, 
                                NPT_LargeSize     length)
{
    if (start == (NPT_Position)-1 || end == (NPT_Position)-1 || length == (NPT_Size)-1) {
        NPT_LOG_WARNING_3("Content Range is exactly -1? (start=%d, end=%d, length=%d)", start, end, length);
    }

    NPT_String range = "bytes ";
    range += NPT_String::FromIntegerU(start);
    range += '-';
    range += NPT_String::FromIntegerU(end);
    range += '/';
    range += NPT_String::FromIntegerU(length);
    response.GetHeaders().SetHeader(NPT_HTTP_HEADER_CONTENT_RANGE, range);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_HttpResponse::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::ToLog(NPT_LoggerReference logger, int level, NPT_HttpResponse* response)
{
    NPT_COMPILER_UNUSED(logger);
    NPT_COMPILER_UNUSED(level);

    if (!response) {
        NPT_LOG_L(logger, level, "NULL HTTP Response!");
        return NPT_FAILURE;
    }

    NPT_StringOutputStreamReference stream(new NPT_StringOutputStream);
    NPT_OutputStreamReference output = stream;
    response->GetHeaders().GetHeaders().Apply(NPT_HttpHeaderPrinter(output));

    NPT_LOG_L4(logger, level, "\n%s %d %s\n%s", 
        (const char*)response->GetProtocol(), 
        response->GetStatusCode(), 
        (const char*)response->GetReasonPhrase(),
        (const char*)stream->GetString());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::Connect
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpHelper::Connect(NPT_Socket& connection, NPT_HttpRequest& request, NPT_Timeout timeout)
{
    // get the address of the server
    NPT_IpAddress server_address;
    NPT_CHECK_SEVERE(server_address.ResolveName(request.GetUrl().GetHost(), timeout));
    NPT_SocketAddress address(server_address, request.GetUrl().GetPort());

    // connect to the server
    NPT_LOG_FINER_2("Connecting to %s:%d\n", (const char*)request.GetUrl().GetHost(), request.GetUrl().GetPort());
    NPT_CHECK_SEVERE(connection.Connect(address, timeout));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpHelper::SetBasicAuthorization
+---------------------------------------------------------------------*/
void         
PLT_HttpHelper::SetBasicAuthorization(NPT_HttpRequest& request, 
                                      const char*      login, 
                                      const char*      password)
{ 
	NPT_String encoded;
	NPT_String cred =  NPT_String(login) + ":" + password;

	NPT_Base64::Encode((const NPT_Byte *)cred.GetChars(), cred.GetLength(), encoded);
	request.GetHeaders().SetHeader(NPT_HTTP_HEADER_AUTHORIZATION, NPT_String("Basic " + encoded)); 
}

