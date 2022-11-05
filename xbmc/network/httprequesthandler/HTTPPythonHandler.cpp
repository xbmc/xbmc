/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPPythonHandler.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Webinterface.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPRequestHandlerUtils.h"
#include "network/httprequesthandler/HTTPWebinterfaceHandler.h"
#include "network/httprequesthandler/python/HTTPPythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonWsgiInvoker.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#define MAX_STRING_POST_SIZE 20000

CHTTPPythonHandler::CHTTPPythonHandler()
  : IHTTPRequestHandler(),
    m_scriptPath(),
    m_addon(),
    m_lastModified(),
    m_requestData(),
    m_responseData(),
    m_responseRanges(),
    m_redirectUrl()
{ }

CHTTPPythonHandler::CHTTPPythonHandler(const HTTPRequest &request)
  : IHTTPRequestHandler(request),
    m_scriptPath(),
    m_addon(),
    m_lastModified(),
    m_requestData(),
    m_responseData(),
    m_responseRanges(),
    m_redirectUrl()
{
  m_response.type = HTTPMemoryDownloadNoFreeCopy;

  // get the real path of the script and check if it actually exists
  m_response.status = CHTTPWebinterfaceHandler::ResolveUrl(m_request.pathUrl, m_scriptPath, m_addon);
  // only allow requests to a non-static webinterface addon
  if (m_addon == NULL || m_addon->Type() != ADDON::AddonType::WEB_INTERFACE ||
      std::dynamic_pointer_cast<ADDON::CWebinterface>(m_addon)->GetType() ==
          ADDON::WebinterfaceTypeStatic)
  {
    m_response.type = HTTPError;
    m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;

    return;
  }

  std::shared_ptr<ADDON::CWebinterface> webinterface = std::dynamic_pointer_cast<ADDON::CWebinterface>(m_addon);

  // forward every request to the default entry point
  m_scriptPath = webinterface->LibPath();

  // we need to map any requests to a specific WSGI webinterface to the root path
  std::string baseLocation = webinterface->GetBaseLocation();
  if (!URIUtils::PathHasParent(m_request.pathUrl, baseLocation))
  {
    m_response.type = HTTPRedirect;
    m_response.status = MHD_HTTP_MOVED_PERMANENTLY;
    m_redirectUrl = baseLocation + m_request.pathUrl;
  }

  // no need to try to read the last modified date from a non-existing file
  if (m_response.status != MHD_HTTP_OK)
    return;

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

bool CHTTPPythonHandler::CanHandleRequest(const HTTPRequest &request) const
{
  ADDON::AddonPtr addon;
  std::string path;
  // try to resolve the addon as any python script must be part of a webinterface
  if (!CHTTPWebinterfaceHandler::ResolveAddon(request.pathUrl, addon, path) || addon == NULL ||
      addon->Type() != ADDON::AddonType::WEB_INTERFACE)
    return false;

  // static webinterfaces aren't allowed to run python scripts
  ADDON::CWebinterface* webinterface = static_cast<ADDON::CWebinterface*>(addon.get());
  if (webinterface->GetType() != ADDON::WebinterfaceTypeWsgi)
    return false;

  return true;
}

MHD_RESULT CHTTPPythonHandler::HandleRequest()
{
  if (m_response.type == HTTPError || m_response.type == HTTPRedirect)
    return MHD_YES;

  std::vector<std::string> args;
  args.push_back(m_scriptPath);

  try
  {
    HTTPPythonRequest* pythonRequest = new HTTPPythonRequest();
    pythonRequest->connection = m_request.connection;
    pythonRequest->file = URIUtils::GetFileName(m_request.pathUrl);
    HTTPRequestHandlerUtils::GetRequestHeaderValues(m_request.connection, MHD_GET_ARGUMENT_KIND, pythonRequest->getValues);
    HTTPRequestHandlerUtils::GetRequestHeaderValues(m_request.connection, MHD_HEADER_KIND, pythonRequest->headerValues);
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

    CHTTPPythonInvoker* pythonInvoker =
        new CHTTPPythonWsgiInvoker(&CServiceBroker::GetXBPython(), pythonRequest);
    LanguageInvokerPtr languageInvokerPtr(pythonInvoker);
    int result = CScriptInvocationManager::GetInstance().ExecuteSync(m_scriptPath, languageInvokerPtr, m_addon, args, 30000, false);

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

bool CHTTPPythonHandler::appendPostData(const char *data, size_t size)
{
  if (m_requestData.size() + size > MAX_STRING_POST_SIZE)
  {
    CServiceBroker::GetLogging()
        .GetLogger("CHTTPPythonHandler")
        ->error("Stopped uploading post since it exceeded size limitations ({})",
                MAX_STRING_POST_SIZE);
    return false;
  }

  m_requestData.append(data, size);

  return true;
}
