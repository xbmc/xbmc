/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "HTTPVfsHandler.h"
#include "MediaSource.h"
#include "URL.h"
#include "filesystem/File.h"
#include "network/WebServer.h"
#include "settings/MediaSourceSettings.h"
#include "utils/URIUtils.h"

CHTTPVfsHandler::CHTTPVfsHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  std::string file;
  int responseStatus = MHD_HTTP_BAD_REQUEST;

  if (m_request.pathUrl.size() > 5)
  {
    file = m_request.pathUrl.substr(5);

    if (XFILE::CFile::Exists(file))
    {
      bool accessible = false;
      if (file.substr(0, 8) == "image://")
        accessible = true;
      else
      {
        std::string sourceTypes[] = { "video", "music", "pictures" };
        unsigned int size = sizeof(sourceTypes) / sizeof(std::string);

        std::string realPath = URIUtils::GetRealPath(file);
        // for rar:// and zip:// paths we need to extract the path to the archive instead of using the VFS path
        while (URIUtils::IsInArchive(realPath))
          realPath = CURL(realPath).GetHostName();

        VECSOURCES *sources = NULL;
        for (unsigned int index = 0; index < size && !accessible; index++)
        {
          sources = CMediaSourceSettings::Get().GetSources(sourceTypes[index]);
          if (sources == NULL)
            continue;

          for (VECSOURCES::const_iterator source = sources->begin(); source != sources->end() && !accessible; ++source)
          {
            // don't allow access to locked / disabled sharing sources
            if (source->m_iHasLock == 2 || !source->m_allowSharing)
              continue;

            for (std::vector<std::string>::const_iterator path = source->vecPaths.begin(); path != source->vecPaths.end(); ++path)
            {
              std::string realSourcePath = URIUtils::GetRealPath(*path);
              if (URIUtils::IsInPath(realPath, realSourcePath))
              {
                accessible = true;
                break;
              }
            }
          }
        }
      }

      if (accessible)
        responseStatus = MHD_HTTP_OK;
      // the file exists but not in one of the defined sources so we deny access to it
      else
        responseStatus = MHD_HTTP_UNAUTHORIZED;
    }
    else
      responseStatus = MHD_HTTP_NOT_FOUND;
  }

  // set the file and the HTTP response status
  SetFile(file, responseStatus);
}

bool CHTTPVfsHandler::CanHandleRequest(const HTTPRequest &request)
{
  return request.pathUrl.find("/vfs") == 0;
}
