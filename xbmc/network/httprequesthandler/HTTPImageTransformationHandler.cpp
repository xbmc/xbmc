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

#include <map>
#include <sstream>

#include "HTTPImageTransformationHandler.h"
#include "URL.h"
#include "filesystem/File.h"
#include "network/WebServer.h"
#include "pictures/Picture.h"

using namespace std;

CHTTPImageTransformationHandler::CHTTPImageTransformationHandler()
  : m_size(0),
    m_data(NULL)
{ }

bool CHTTPImageTransformationHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.method == GET &&
          request.url.find("/transform/image/") == 0 && request.url.size() > 17);
}

int CHTTPImageTransformationHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  CStdString path = request.url.substr(17);
  CURL::Decode(path);
  if (!XFILE::CFile::Exists(path))
  {
    m_responseCode = MHD_HTTP_NOT_FOUND;
    m_responseType = HTTPError;

    return MHD_YES;
  }

  map<string, string> arguments;
  CWebServer::GetRequestHeaderValues(request.connection, MHD_GET_ARGUMENT_KIND, arguments);

  std::string strWidth = arguments["width"];
  std::string strHeight = arguments["height"];
  std::string strQuality = arguments["quality"];
  std::string format = arguments["format"];
    
  int width = -1, height = -1, quality = 65;
		  
	if (!strWidth.empty())
	{
		istringstream convert(strWidth);
		if (!(convert >> width) || width <= 0)
		  width = -1;
	}
	if (!strHeight.empty())
	{
		istringstream convert(strHeight);
		if (!(convert >> height) || height <= 0)
		  height = -1;
	}
	if (!strQuality.empty())
	{
		istringstream convert(strQuality);
		if (!(convert >> quality) || quality <= 0)
	      quality = 65;
    else if (quality > 100)
      quality = 100;
	}
  if (format.empty() ||
      (format.compare("jpg") != 0 && format.compare("png") != 0 && format.compare("bmp") != 0))
		format = "jpg";

  long size = 0;
  CPicture::EncodeImageToBuffer(path, m_data, size, width, height, format.c_str(), quality);

	if (size > 0 && m_data != NULL)
	{
    m_size = (size_t)size;
    m_responseCode = MHD_HTTP_OK;
    m_responseType = HTTPMemoryDownloadFreeCopy;
	}
  else
  {
		m_responseCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
    m_responseType = HTTPError;

    if (m_data != NULL)
      free (m_data);
  }

  return MHD_YES;
}
