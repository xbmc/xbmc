/*
 *      Copyright (C) 2013 Team XBMC
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

#include "HTTPSpecialHandler.h"
#include "URL.h"
#include "filesystem/File.h"
#include "network/WebServer.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"

using namespace std;

bool CHTTPSpecialHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.url.find("/special/") == 0 || request.url.find("/log") == 0);
}

int CHTTPSpecialHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  string ext;

  /* our URL is /special */
  if (request.url.substr(0, 19) == "/special/special://")
  {
    m_path = request.url.substr(9);
    ext = URIUtils::GetExtension(request.url);
    StringUtils::ToLower(ext);

    if (!XFILE::CFile::Exists(m_path))
    {
      CLog::Log(LOGERROR, "CHTTPSpecialHandler::HandleHTTPRequest: file %s does not exist", m_path.c_str());
      m_responseCode = MHD_HTTP_NOT_FOUND;
      m_responseType = HTTPError;
      return MHD_YES;
    }
  }
  /* add a shortcut URL /log and /log/ pointing to xbmc.log */
  else if (request.url.compare("/log") == 0 || request.url.compare("/log/") == 0)
  {
    m_path = "special://temp/xbmc.log";
    ext = ".log";
  }
  else
  {
    CLog::Log(LOGDEBUG, "CHTTPSpecialHandler::HandleHTTPRequest: bad request");
    m_responseCode = MHD_HTTP_BAD_REQUEST;
    m_responseType = HTTPError;
    return MHD_YES;
  }

  /* If we have a file that potentially holds user credentials
   * we want to sanitize them.
   * Else have the webserver handle the download */
  if (ext == ".log" || ext == ".xml")
  {
    XFILE::CFile *file = new XFILE::CFile();
    if (file->Open(m_path, READ_CACHED))
    {
      m_response = "";
      char m_buf[1024];
      memset(m_buf, '\0', sizeof(m_buf));
      string line = "";
      string user;
      string pass;
      string replace;

      /* this replaces the following:
       * - username:pass in URIs
       * - <pass>pass</pass> in XMLs
       */
      CRegExp regex1, regex2;
      regex1.RegComp("://(.*):(.*)@");
      regex2.RegComp("<[^<>]*(pass|user)[^<>]*>([^<]+)</.*(pass|user).*>");

      while (file->ReadString(m_buf, 1023))
      {
        line = m_buf;
        if (regex1.RegFind(line))
        {
          user = regex1.GetReplaceString("\\1");
          pass = regex1.GetReplaceString("\\2");
          replace = "://" + user + ":" + pass + "@";
          StringUtils::Replace(line, replace, "://xxx:xxx@");
        }
        if (regex2.RegFind(line))
        {
          replace = regex2.GetReplaceString("\\2");
          if (replace.length() > 0)
            StringUtils::Replace(line, replace, "xxx");
        }
        m_response += line;
        memset(m_buf, '\0', sizeof(m_buf));
      }

      const char *mime = request.webserver->CreateMimeTypeFromExtension(ext.c_str());
      if (mime)
        m_responseHeaderFields.insert(pair<string, string>("Content-Type", mime));

      file->Close();
      delete file;
      m_request.clear();

      m_responseCode = MHD_HTTP_OK;
      m_responseType = HTTPMemoryDownloadNoFreeCopy;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CHTTPSpecialHandler::HandleHTTPRequest: cannot open file %s", m_path.c_str());
      m_responseCode = MHD_HTTP_NOT_FOUND;
      m_responseType = HTTPError;
    }
  }
  else
  {
    m_responseCode = MHD_HTTP_OK;
    m_responseType = HTTPFileDownload;
  }
  return MHD_YES;
}
