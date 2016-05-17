/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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
#include "URL.h"
#include "filesystem/File.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "interfaces/json-rpc/JSONUtils.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPRequestHandlerUtils.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/Variant.h"

#define MAX_HTTP_POST_SIZE 65536

bool CHTTPJsonRpcHandler::CanHandleRequest(const HTTPRequest &request)
{
  return (request.pathUrl.compare("/jsonrpc") == 0);
}

int CHTTPJsonRpcHandler::HandleRequest()
{
  CHTTPClient client;
  bool isRequest = false;
  std::string jsonpCallback;

  // get all query arguments
  std::map<std::string, std::string> arguments;
  HTTPRequestHandlerUtils::GetRequestHeaderValues(m_request.connection, MHD_GET_ARGUMENT_KIND, arguments);

  if (m_request.method == POST)
  {
    std::string contentType = HTTPRequestHandlerUtils::GetRequestHeaderValue(m_request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    // If the content-type of the m_request was specified, it must be application/json-rpc, application/json, or application/jsonrequest
    // http://www.jsonrpc.org/historical/json-rpc-over-http.html
    if (!contentType.empty() && contentType.compare("application/json-rpc") != 0 &&
        contentType.compare("application/json") != 0 && contentType.compare("application/jsonrequest") != 0)
    {
      m_response.type = HTTPError;
      m_response.status = MHD_HTTP_UNSUPPORTED_MEDIA_TYPE;
      return MHD_YES;
    }

    isRequest = true;
  }
  else if (m_request.method == GET)
  {
    std::map<std::string, std::string>::const_iterator argument = arguments.find("request");
    if (argument != arguments.end() && !argument->second.empty())
    {
      m_requestData = argument->second;
      isRequest = true;
    }
  }

  std::map<std::string, std::string>::const_iterator argument = arguments.find("jsonp");
  if (argument != arguments.end() && !argument->second.empty())
    jsonpCallback = argument->second;
  else
  {
    argument = arguments.find("callback");
    if (argument != arguments.end() && !argument->second.empty())
      jsonpCallback = argument->second;
  }

  if (isRequest)
  {
    m_responseData = JSONRPC::CJSONRPC::MethodCall(m_requestData, &m_transportLayer, &client);

    if (!jsonpCallback.empty())
      m_responseData = jsonpCallback + "(" + m_responseData + ");";
  }
  else if (jsonpCallback.empty())
  {
    // get the whole output of JSONRPC.Introspect
    CVariant result;
    JSONRPC::CJSONServiceDescription::Print(result, &m_transportLayer, &client);
    m_responseData = CJSONVariantWriter::Write(result, false);
  }
  else
  {
    m_response.type = HTTPError;
    m_response.status = MHD_HTTP_BAD_REQUEST;

    return MHD_YES;
  }

  m_requestData.clear();

  m_responseRange.SetData(m_responseData.c_str(), m_responseData.size());

  m_response.type = HTTPMemoryDownloadNoFreeCopy;
  m_response.status = MHD_HTTP_OK;
  m_response.contentType = "application/json";
  m_response.totalLength = m_responseData.size();

  return MHD_YES;
}

HttpResponseRanges CHTTPJsonRpcHandler::GetResponseData() const
{
  HttpResponseRanges ranges;
  ranges.push_back(m_responseRange);

  return ranges;
}

#if (MHD_VERSION >= 0x00040001)
bool CHTTPJsonRpcHandler::appendPostData(const char *data, size_t size)
#else
bool CHTTPJsonRpcHandler::appendPostData(const char *data, unsigned int size)
#endif
{
  if (m_requestData.size() + size > MAX_HTTP_POST_SIZE)
  {
    CLog::Log(LOGERROR, "WebServer: Stopped uploading POST data since it exceeded size limitations (%d)", MAX_HTTP_POST_SIZE);
    return false;
  }

  m_requestData.append(data, size);

  return true;
}

bool CHTTPJsonRpcHandler::CHTTPTransportLayer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  if (!XFILE::CFile::Exists(path))
    return false;

  protocol = "http";
  std::string url;
  std::string strPath = path;
  if (StringUtils::StartsWith(strPath, "image://") ||
    (StringUtils::StartsWith(strPath, "special://") && StringUtils::EndsWith(strPath, ".tbn")))
    url = "image/";
  else
    url = "vfs/";
  url += CURL::Encode(strPath);
  details["path"] = url;

  return true;
}

bool CHTTPJsonRpcHandler::CHTTPTransportLayer::Download(const char *path, CVariant &result)
{
  return false;
}

int CHTTPJsonRpcHandler::CHTTPTransportLayer::GetCapabilities()
{
  return JSONRPC::Response | JSONRPC::FileDownloadRedirect;
}

int CHTTPJsonRpcHandler::CHTTPClient::GetPermissionFlags()
{
  return JSONRPC::OPERATION_PERMISSION_ALL;
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
