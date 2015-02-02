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

#include "system.h"
#include "HTTPFileHandler.h"
#include "filesystem/File.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

CHTTPFileHandler::CHTTPFileHandler()
  : IHTTPRequestHandler(),
    m_url(),
    m_canHandleRanges(true),
    m_canBeCached(true),
    m_lastModified()
{ }

CHTTPFileHandler::CHTTPFileHandler(const HTTPRequest &request)
  : IHTTPRequestHandler(request),
    m_url(),
    m_canHandleRanges(true),
    m_canBeCached(true),
    m_lastModified()
{ }

int CHTTPFileHandler::HandleRequest()
{
  return !m_url.empty() ? MHD_YES : MHD_NO;
}

bool CHTTPFileHandler::GetLastModifiedDate(CDateTime &lastModified) const
{
  if (!m_lastModified.IsValid())
    return false;

  lastModified = m_lastModified;
  return true;
}

void CHTTPFileHandler::SetFile(const std::string& file, int responseStatus)
{
  m_url = file;
  m_response.status = responseStatus;
  if (m_url.empty())
    return;

  // translate the response status into the response type
  if (m_response.status == MHD_HTTP_OK)
    m_response.type = HTTPFileDownload;
  else if (m_response.status == MHD_HTTP_FOUND)
      m_response.type = HTTPRedirect;
  else
    m_response.type = HTTPError;

  // try to determine some additional information if the file can be downloaded
  if (m_response.type == HTTPFileDownload)
  {
    // determine the content type
    std::string ext = URIUtils::GetExtension(m_url);
    StringUtils::ToLower(ext);
    m_response.contentType = CMime::GetMimeType(ext);

    // determine the last modified date
    XFILE::CFile fileObj;
    if (!fileObj.Open(m_url, READ_NO_CACHE))
    {
      m_response.type = HTTPError;
      m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }
    else
    {
      struct __stat64 statBuffer;
      if (fileObj.Stat(&statBuffer) == 0)
      {
        struct tm *time;
#ifdef HAVE_LOCALTIME_R
        struct tm result = { };
        time = localtime_r((time_t*)&statBuffer.st_mtime, &result);
#else
        time = localtime((time_t *)&statBuffer.st_mtime);
#endif
        if (time != NULL)
          m_lastModified = *time;
      }
    }
  }

  // disable ranges and caching if the file can't be downloaded
  if (m_response.type != HTTPFileDownload)
  {
    m_canHandleRanges = false;
    m_canBeCached = false;
  }

  // disable caching if the last modified date couldn't be read
  if (!m_lastModified.IsValid())
    m_canBeCached = false;
}
