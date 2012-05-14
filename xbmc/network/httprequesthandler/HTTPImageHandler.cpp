/*
 *      Copyright (C) 2012 Team XBMC
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

#include "HTTPImageHandler.h"
#include "network/WebServer.h"
#include "URL.h"
#include "filesystem/ImageFile.h"

using namespace std;

bool CHTTPImageHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.url.find("/image/") == 0);
}

int CHTTPImageHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  if (request.url.size() > 7)
  {
    m_path = request.url.substr(7);

    XFILE::CImageFile imageFile;
    if (imageFile.Exists(m_path) ||
       // temporary workaround for music images until they are integrated into CTextureCache and therefore CImageFile
       (m_path.Left(10) == "special://" && m_path.Right(4) == ".tbn" && XFILE::CFile::Exists(m_path)))
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
