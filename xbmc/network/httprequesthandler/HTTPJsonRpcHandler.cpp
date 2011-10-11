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

bool CHTTPJsonRpcHandler::CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version)
{
  return (url.find("/jsonrpc") == url.size() - 8);
}

#if (MHD_VERSION >= 0x00040001)
int CHTTPJsonRpcHandler::HandleHTTPRequest(CWebServer *server, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, size_t *upload_data_size, void **con_cls)
#else
int CHTTPJsonRpcHandler::HandleHTTPRequest(CWebServer *server, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, unsigned int *upload_data_size, void **con_cls)
#endif
{
  if (method == POST)
  {
    m_responseType = HTTPNone;

    if ((*con_cls) == NULL)
    {
      *con_cls = new string();

      return MHD_YES;
    }
    if (*upload_data_size) 
    {
      string *post = (string *)(*con_cls);
      if (*upload_data_size + post->size() > MAX_STRING_POST_SIZE)
      {
        CLog::Log(LOGERROR, "WebServer: Stopped uploading post since it exceeded size limitations");
        return MHD_NO;
      }
      else
      {
        post->append(upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
      }
    }
    else
    {
      string *jsoncall = (string *)(*con_cls);

      CHTTPClient client;
      m_response = CJSONRPC::MethodCall(*jsoncall, server, &client);

      m_responseHeaderFields["Content-Type"] = "application/json";
      m_responseType = HTTPMemoryDownloadNoFreeCopy;

      delete jsoncall;
    }
  }
  else
  {
    m_response = PAGE_JSONRPC_INFO;
    m_responseType = HTTPMemoryDownloadNoFreeNoCopy;
  }

  m_responseCode = MHD_HTTP_OK;

  return MHD_YES;
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
