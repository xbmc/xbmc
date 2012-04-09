/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "HTTPJsonRpcHandler.h"
#include "network/WebServer.h"
#include "utils/log.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONUtils.h"

#define MAX_STRING_POST_SIZE 20000
#define PAGE_JSONRPC_INFO   "<html><head><title>JSONRPC</title></head><body>JSONRPC active and working</body></html>"

using namespace std;
using namespace JSONRPC;

bool CHTTPJsonRpcHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.url.compare("/jsonrpc") == 0);
}

int CHTTPJsonRpcHandler::HandleHTTPRequest(const HTTPRequest &request)
{
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

    CHTTPClient client;
    m_response = CJSONRPC::MethodCall(m_request, request.webserver, &client);

    m_responseHeaderFields.insert(pair<string, string>("Content-Type", "application/json"));

    m_request.clear();
  }
  else
    m_response = PAGE_JSONRPC_INFO;
  
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
