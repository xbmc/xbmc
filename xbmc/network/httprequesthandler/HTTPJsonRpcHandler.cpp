/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "HTTPJsonRpcHandler.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "interfaces/json-rpc/JSONUtils.h"
#include "network/WebServer.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"

#define MAX_STRING_POST_SIZE 20000

using namespace std;
using namespace JSONRPC;

bool CHTTPJsonRpcHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.url.compare("/jsonrpc") == 0);
}

int CHTTPJsonRpcHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  CHTTPClient client;
  bool isRequest = false;
  if (request.method == POST)
  {
    string contentType = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    // If the content-type of the request was specified, it must be application/json
    if (!contentType.empty() && contentType.compare("application/json") != 0)
    {
      m_responseType = HTTPError;
      m_responseCode = MHD_HTTP_UNSUPPORTED_MEDIA_TYPE;
      return MHD_YES;
    }

    isRequest = true;
  }
  else if (request.method == GET)
  {
    map<string, string> arguments;
    if (CWebServer::GetRequestHeaderValues(request.connection, MHD_GET_ARGUMENT_KIND, arguments) > 0)
    {
      map<string, string>::const_iterator argument = arguments.find("request");
      if (argument != arguments.end() && !argument->second.empty())
      {
        m_request = argument->second;
        isRequest = true;
      }
    }
  }

  if (isRequest)
    m_response = CJSONRPC::MethodCall(m_request, request.webserver, &client);
  else
  {
    // get the whole output of JSONRPC.Introspect
    CVariant result;
    CJSONServiceDescription::Print(result, request.webserver, &client);
    m_response = CJSONVariantWriter::Write(result, false);
  }

  m_responseHeaderFields.insert(pair<string, string>("Content-Type", "application/json"));

  m_request.clear();
  
  m_responseType = HTTPMemoryDownloadNoFreeCopy;
  m_responseCode = MHD_HTTP_OK;

  return MHD_YES;
}

#if (MHD_VERSION >= 0x00040001)
bool CHTTPJsonRpcHandler::appendPostData(const char *data, size_t size)
#else
bool CHTTPJsonRpcHandler::appendPostData(const char *data, unsigned int size)
#endif
{
  if (m_request.size() + size > MAX_STRING_POST_SIZE)
  {
    CLog::Log(LOGERROR, "WebServer: Stopped uploading post since it exceeded size limitations");
    return false;
  }

  m_request.append(data, size);

  return true;
}

int CHTTPJsonRpcHandler::CHTTPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CHTTPJsonRpcHandler::CHTTPClient::GetAnnouncementFlags()
{
  // Does not support broadcast
  return 0;
}

bool CHTTPJsonRpcHandler::CHTTPClient::SetAnnouncementFlags(int flags)
{
  return false;
}
