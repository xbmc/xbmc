/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPPythonInvoker.h"

#include "CompileInfo.h"
#include "utils/StringUtils.h"

CHTTPPythonInvoker::CHTTPPythonInvoker(ILanguageInvocationHandler* invocationHandler,
                                       HTTPPythonRequest* request)
  : CPythonInvoker(invocationHandler), m_request(request)
{ }

CHTTPPythonInvoker::~CHTTPPythonInvoker()
{
  delete m_request;
  m_request = NULL;
}

void CHTTPPythonInvoker::onAbort()
{
  if (m_request == NULL)
    return;

  m_internalError = true;
  m_request->responseType = HTTPError;
  m_request->responseStatus = MHD_HTTP_INTERNAL_SERVER_ERROR;
}

void CHTTPPythonInvoker::onError(const std::string& exceptionType /* = "" */, const std::string& exceptionValue /* = "" */, const std::string& exceptionTraceback /* = "" */)
{
  if (m_request == NULL)
    return;

  m_internalError = true;
  m_request->responseType = HTTPMemoryDownloadNoFreeCopy;
  m_request->responseStatus = MHD_HTTP_INTERNAL_SERVER_ERROR;

  std::string output;
  if (!exceptionType.empty())
  {
    output += exceptionType;

    if (!exceptionValue.empty())
      output += ": " + exceptionValue;
    output += "\n";
  }

  if (!exceptionTraceback .empty())
    output += exceptionTraceback;

  // replace all special characters

  StringUtils::Replace(output, "<", "&lt;");
  StringUtils::Replace(output, ">", "&gt;");
  StringUtils::Replace(output, " ", "&nbsp;");
  StringUtils::Replace(output, "\n", "\n<br />");

  if (!exceptionType.empty())
  {
    // now make the type and value bold (needs to be done here because otherwise the < and > would have been replaced
    output = "<b>" + output;
    output.insert(output.find('\n'), "</b>");
  }

  m_request->responseData = "<html><head><title>" + std::string(CCompileInfo::GetAppName()) + ": python error</title></head><body>" + output + "</body></html>";
}
