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

#include "HTTPPythonInvoker.h"
#include "CompileInfo.h"
#include "utils/StringUtils.h"

CHTTPPythonInvoker::CHTTPPythonInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request)
  : CPythonInvoker(invocationHandler),
    m_request(request),
    m_internalError(false)
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
