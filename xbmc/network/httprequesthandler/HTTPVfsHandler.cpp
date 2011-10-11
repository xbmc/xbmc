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

#include "HTTPVfsHandler.h"
#include "network/WebServer.h"
#include "URL.h"
#include "filesystem/File.h"

using namespace std;

bool CHTTPVfsHandler::CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version)
{
  return (url.find("/vfs") == 0);
}

#if (MHD_VERSION >= 0x00040001)
int CHTTPVfsHandler::HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, size_t *upload_data_size, void **con_cls)
#else
int CHTTPVfsHandler::HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                            const char *upload_data, unsigned int *upload_data_size, void **con_cls)
#endif
{
  bool ok = false;
  if (url.size() > 5)
  {
    m_path = url.substr(5);

    if (XFILE::CFile::Exists(m_path))
    {
      m_responseCode = MHD_HTTP_OK;
      m_responseType = HTTPFileDownload;
    }
    else
    {
      m_responseCode = MHD_HTTP_NOT_FOUND;
      m_responseType = HTTPError;
    }
  }
  else
  {
    m_responseCode = MHD_HTTP_BAD_REQUEST;
    m_responseType = HTTPError;
  }

  return MHD_YES;
}
