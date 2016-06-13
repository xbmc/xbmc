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

#include <map>

#include "HTTPImageTransformationHandler.h"
#include "TextureCacheJob.h"
#include "URL.h"
#include "filesystem/ImageFile.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPRequestHandlerUtils.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define TRANSFORMATION_OPTION_WIDTH             "width"
#define TRANSFORMATION_OPTION_HEIGHT            "height"
#define TRANSFORMATION_OPTION_SCALING_ALGORITHM "scaling_algorithm"

static const std::string ImageBasePath = "/image/";

CHTTPImageTransformationHandler::CHTTPImageTransformationHandler()
  : m_url(),
    m_lastModified(),
    m_buffer(NULL),
    m_responseData()
{ }

CHTTPImageTransformationHandler::CHTTPImageTransformationHandler(const HTTPRequest &request)
  : IHTTPRequestHandler(request),
    m_url(),
    m_lastModified(),
    m_buffer(NULL),
    m_responseData()
{
  m_url = m_request.pathUrl.substr(ImageBasePath.size());
  if (m_url.empty())
  {
    m_response.status = MHD_HTTP_BAD_REQUEST;
    m_response.type = HTTPError;
    return;
  }

  XFILE::CImageFile imageFile;
  const CURL pathToUrl(m_url);
  if (!imageFile.Exists(pathToUrl))
  {
    m_response.status = MHD_HTTP_NOT_FOUND;
    m_response.type = HTTPError;
    return;
  }

  m_response.type = HTTPMemoryDownloadNoFreeCopy;
  m_response.status = MHD_HTTP_OK;

  // determine the content type
  std::string ext = URIUtils::GetExtension(pathToUrl.GetHostName());
  StringUtils::ToLower(ext);
  m_response.contentType = CMime::GetMimeType(ext);

  //! @todo determine the maximum age

  // determine the last modified date
  struct __stat64 statBuffer;
  if (imageFile.Stat(pathToUrl, &statBuffer) != 0)
    return;

  struct tm *time;
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

CHTTPImageTransformationHandler::~CHTTPImageTransformationHandler()
{
  m_responseData.clear();
  delete m_buffer;
  m_buffer = NULL;
}

bool CHTTPImageTransformationHandler::CanHandleRequest(const HTTPRequest &request)
{
  if ((request.method != GET && request.method != HEAD) ||
    request.pathUrl.find(ImageBasePath) != 0 || request.pathUrl.size() <= ImageBasePath.size())
    return false;

  // get the transformation options
  std::map<std::string, std::string> options;
  HTTPRequestHandlerUtils::GetRequestHeaderValues(request.connection, MHD_GET_ARGUMENT_KIND, options);

  return (options.find(TRANSFORMATION_OPTION_WIDTH) != options.end() ||
          options.find(TRANSFORMATION_OPTION_HEIGHT) != options.end());
}

int CHTTPImageTransformationHandler::HandleRequest()
{
  if (m_response.type == HTTPError)
    return MHD_YES;

  // nothing else to do if this is a HEAD request
  if (m_request.method == HEAD)
  {
    m_response.status = MHD_HTTP_OK;
    m_response.type = HTTPMemoryDownloadNoFreeNoCopy;

    return MHD_YES;
  }

  // get the transformation options
  std::map<std::string, std::string> options;
  HTTPRequestHandlerUtils::GetRequestHeaderValues(m_request.connection, MHD_GET_ARGUMENT_KIND, options);

  std::vector<std::string> urlOptions;
  std::map<std::string, std::string>::const_iterator option = options.find(TRANSFORMATION_OPTION_WIDTH);
  if (option != options.end())
    urlOptions.push_back(TRANSFORMATION_OPTION_WIDTH "=" + option->second);

  option = options.find(TRANSFORMATION_OPTION_HEIGHT);
  if (option != options.end())
    urlOptions.push_back(TRANSFORMATION_OPTION_HEIGHT "=" + option->second);

  option = options.find(TRANSFORMATION_OPTION_SCALING_ALGORITHM);
  if (option != options.end())
    urlOptions.push_back(TRANSFORMATION_OPTION_SCALING_ALGORITHM "=" + option->second);

  std::string imagePath = m_url;
  if (!urlOptions.empty())
  {
    imagePath += "?";
    imagePath += StringUtils::Join(urlOptions, "&");
  }

  // resize the image into the local buffer
  size_t bufferSize;
  if (!CTextureCacheJob::ResizeTexture(imagePath, m_buffer, bufferSize))
  {
    m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
    m_response.type = HTTPError;

    return MHD_YES;
  }

  // store the size of the image
  m_response.totalLength = bufferSize;

  // nothing else to do if the request is not ranged
  if (!GetRequestedRanges(m_response.totalLength))
  {
    m_responseData.push_back(CHttpResponseRange(m_buffer, 0, m_response.totalLength - 1));
    return MHD_YES;
  }

  for (HttpRanges::const_iterator range = m_request.ranges.Begin(); range != m_request.ranges.End(); ++range)
    m_responseData.push_back(CHttpResponseRange(m_buffer + range->GetFirstPosition(), range->GetFirstPosition(), range->GetLastPosition()));

  return MHD_YES;
}

bool CHTTPImageTransformationHandler::GetLastModifiedDate(CDateTime &lastModified) const
{
  if (!m_lastModified.IsValid())
    return false;

  lastModified = m_lastModified;
  return true;
}
