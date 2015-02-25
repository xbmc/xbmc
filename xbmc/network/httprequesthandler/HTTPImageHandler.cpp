/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "HTTPImageHandler.h"
#include "URL.h"
#include "filesystem/ImageFile.h"
#include "network/WebServer.h"

CHTTPImageHandler::CHTTPImageHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  std::string file;
  int responseStatus = MHD_HTTP_BAD_REQUEST;

  // resolve the URL into a file path and a HTTP response status
  if (m_request.pathUrl.size() > 7)
  {
    file = m_request.pathUrl.substr(7);

    XFILE::CImageFile imageFile;
    const CURL pathToUrl(file);
    if (imageFile.Exists(pathToUrl))
      responseStatus = MHD_HTTP_OK;
    else
      responseStatus = MHD_HTTP_NOT_FOUND;
  }

  // set the file and the HTTP response status
  SetFile(file, responseStatus);
}

bool CHTTPImageHandler::CanHandleRequest(const HTTPRequest &request)
{
  return request.pathUrl.find("/image/") == 0;
}
