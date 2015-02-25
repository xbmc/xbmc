/*
 *      Copyright (C) 2015 Team XBMC
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

#include "HTTPPythonHandler.h"
#include "URL.h"
#include "addons/Webinterface.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "filesystem/File.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPWebinterfaceHandler.h"
#include "network/httprequesthandler/python/HTTPModPythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonInvoker.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define MAX_STRING_POST_SIZE 20000

CHTTPPythonHandler::CHTTPPythonHandler()
  : IHTTPRequestHandler(),
    m_scriptPath(),
    m_addon(),
    m_type(),
    m_lastModified(),
    m_requestData(),
    m_responseData(),
    m_responseRanges()
{ }

CHTTPPythonHandler::CHTTPPythonHandler(const HTTPRequest &request)
  : IHTTPRequestHandler(request),
    m_scriptPath(),
    m_addon(),
    m_type(),
    m_lastModified(),
    m_requestData(),
    m_responseData(),
    m_responseRanges()
{
  // get the real path of the script and check if it actually exists
  m_response.status = CHTTPWebinterfaceHandler::ResolveUrl(m_request.pathUrl, m_scriptPath, m_addon);
  // only allow requests to existing files and to python scripts belonging to a non-static webinterface addon
  if (m_response.status != MHD_HTTP_OK ||
      m_addon == NULL || m_addon->Type() != ADDON::ADDON_WEB_INTERFACE ||
      std::dynamic_pointer_cast<ADDON::CWebinterface>(m_addon)->GetType() == ADDON::WebinterfaceTypeStatic)
  {
    m_response.type = HTTPError;
    if (m_response.status == MHD_HTTP_FOUND)
      m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;

    return;
  }

  m_type = std::dynamic_pointer_cast<ADDON::CWebinterface>(m_addon)->GetType();

  m_response.type = HTTPMemoryDownloadNoFreeCopy;
  m_response.status = MHD_HTTP_OK;
  
  // determine the last modified date
  const CURL pathToUrl(m_scriptPath);
  struct __stat64 statBuffer;
  if (XFILE::CFile::Stat(pathToUrl, &statBuffer) != 0)
    return;

  struct tm* time;
#ifdef HAVE_LOCALTIME_R
  struct tm result = {};
  time = localtime_r((time_t*)&statBuffer.st_mtime, &result);
#else
  time = localtime((time_t *)&statBuffer.st_mtime);
#endif
  if (time == NULL)
    return;

  m_lastModified = *time;
}

bool CHTTPPythonHandler::CanHandleRequest(const HTTPRequest &request)
{
  ADDON::AddonPtr addon;
  std::string path;
  // try to resolve the addon as any python script must be part of a webinterface
  if (!CHTTPWebinterfaceHandler::ResolveAddon(request.pathUrl, addon, path) ||
      addon == NULL || addon->Type() != ADDON::ADDON_WEB_INTERFACE)
    return false;

  // static webinterfaces aren't allowed to run python scripts
  ADDON::CWebinterface* webinterface = static_cast<ADDON::CWebinterface*>(addon.get());
  if (webinterface->GetType() == ADDON::WebinterfaceTypeStatic)
    return false;

  // if the root path has been requested we will automatically load the default entry point
  if (URIUtils::PathEquals(path, addon->Path(), true))
    return true;

  // we only handle python scripts
  if (!StringUtils::EqualsNoCase(URIUtils::GetExtension(path), ".py"))
    return false;

  return true;
}

int CHTTPPythonHandler::HandleRequest()
{
  if (m_response.type == HTTPError)
    return MHD_YES;

  std::vector<std::string> args;
  args.push_back(m_scriptPath);

  try
  {
    HTTPPythonRequest* pythonRequest = new HTTPPythonRequest();
    pythonRequest->connection = m_request.connection;
    pythonRequest->file = URIUtils::GetFileName(m_request.pathUrl);
    CWebServer::GetRequestHeaderValues(m_request.connection, MHD_GET_ARGUMENT_KIND, pythonRequest->getValues);
    CWebServer::GetRequestHeaderValues(m_request.connection, MHD_HEADER_KIND, pythonRequest->headerValues);
    pythonRequest->method = m_request.method;
    pythonRequest->postValues = m_postFields;
    pythonRequest->requestContent = m_requestData;
    pythonRequest->responseType = HTTPNone;
    pythonRequest->responseLength = 0;
    pythonRequest->responseStatus = MHD_HTTP_OK;
    pythonRequest->url = m_request.pathUrlFull;
    pythonRequest->path = m_request.pathUrl;
    pythonRequest->version = m_request.version;
    pythonRequest->requestTime = CDateTime::GetCurrentDateTime();
    pythonRequest->lastModifiedTime = m_lastModified;

    std::string hostname;
    uint16_t port;
    if (GetHostnameAndPort(hostname, port))
    {
      pythonRequest->hostname = hostname;
      pythonRequest->port = port;
    }

    CHTTPPythonInvoker* pythonInvoker = NULL;
    if (m_type == ADDON::WebinterfaceTypeModPython)
      pythonInvoker = new CHTTPModPythonInvoker(&g_pythonParser, pythonRequest);
    else
    {
      CLog::Log(LOGWARNING, "WebServer: unable to run python script at %s with an unknown invoker", m_scriptPath.c_str());

      m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
      m_response.type = HTTPError;

      return MHD_YES;
    }

    LanguageInvokerPtr languageInvokerPtr(pythonInvoker);
    int result = CScriptInvocationManager::Get().ExecuteSync(m_scriptPath, languageInvokerPtr, m_addon, args, 30000, false);

    // check if the script couldn't be started
    if (result < 0)
    {
      m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
      m_response.type = HTTPError;

      return MHD_YES;
    }
    // check if the script exited with an error
    if (result > 0)
    {
      // check if the script didn't finish in time
      if (result == ETIMEDOUT)
        m_response.status = MHD_HTTP_REQUEST_TIMEOUT;
      else
        m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
      m_response.type = HTTPError;

      return MHD_YES;
    }

    HTTPPythonRequest* pythonFinalizedRequest = pythonInvoker->GetRequest();
    if (pythonFinalizedRequest == NULL)
    {
      m_response.type = HTTPError;
      m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
      return MHD_YES;
    }

    m_response.type = pythonFinalizedRequest->responseType;
    m_response.status = pythonFinalizedRequest->responseStatus;
    if (m_response.status < MHD_HTTP_BAD_REQUEST)
    {
      if (m_response.type == HTTPNone)
        m_response.type = HTTPMemoryDownloadNoFreeCopy;
      m_response.headers = pythonFinalizedRequest->responseHeaders;

      if (pythonFinalizedRequest->lastModifiedTime.IsValid())
        m_lastModified = pythonFinalizedRequest->lastModifiedTime;
    }
    else
    {
      if (m_response.type == HTTPNone)
        m_response.type = HTTPError;
      m_response.headers = pythonFinalizedRequest->responseHeadersError;
    }

    m_responseData = pythonFinalizedRequest->responseData;
    if (pythonFinalizedRequest->responseLength > 0 && pythonFinalizedRequest->responseLength <= m_responseData.size())
      m_response.totalLength = pythonFinalizedRequest->responseLength;
    else
      m_response.totalLength = m_responseData.size();

    CHttpResponseRange responseRange(m_responseData.c_str(), m_responseData.size());
    m_responseRanges.push_back(responseRange);

    if (!pythonFinalizedRequest->responseContentType.empty())
      m_response.contentType = pythonFinalizedRequest->responseContentType;
  }
  catch (...)
  {
    m_response.type = HTTPError;
    m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
  }

  return MHD_YES;
}

bool CHTTPPythonHandler::GetLastModifiedDate(CDateTime &lastModified) const
{
  if (!m_lastModified.IsValid())
    return false;

  lastModified = m_lastModified;
  return true;
}

#if (MHD_VERSION >= 0x00040001)
bool CHTTPPythonHandler::appendPostData(const char *data, size_t size)
#else
bool CHTTPPythonHandler::appendPostData(const char *data, unsigned int size)
#endif
{
  if (m_requestData.size() + size > MAX_STRING_POST_SIZE)
  {
    CLog::Log(LOGERROR, "WebServer: Stopped uploading post since it exceeded size limitations");
    return false;
  }

  m_requestData.append(data, size);

  return true;
}
