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

#ifndef HTTPREQUESTHANDLER_HTTPVFSHANDLER_H_INCLUDED
#define HTTPREQUESTHANDLER_HTTPVFSHANDLER_H_INCLUDED
#include "HTTPVfsHandler.h"
#endif

#ifndef HTTPREQUESTHANDLER_MEDIASOURCE_H_INCLUDED
#define HTTPREQUESTHANDLER_MEDIASOURCE_H_INCLUDED
#include "MediaSource.h"
#endif

#ifndef HTTPREQUESTHANDLER_URL_H_INCLUDED
#define HTTPREQUESTHANDLER_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef HTTPREQUESTHANDLER_FILESYSTEM_FILE_H_INCLUDED
#define HTTPREQUESTHANDLER_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef HTTPREQUESTHANDLER_NETWORK_WEBSERVER_H_INCLUDED
#define HTTPREQUESTHANDLER_NETWORK_WEBSERVER_H_INCLUDED
#include "network/WebServer.h"
#endif

#ifndef HTTPREQUESTHANDLER_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#define HTTPREQUESTHANDLER_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#include "settings/MediaSourceSettings.h"
#endif

#ifndef HTTPREQUESTHANDLER_UTILS_URIUTILS_H_INCLUDED
#define HTTPREQUESTHANDLER_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif


using namespace std;

bool CHTTPVfsHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return (request.url.find("/vfs") == 0);
}

int CHTTPVfsHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  if (request.url.size() > 5)
  {
    m_path = request.url.substr(5);

    if (XFILE::CFile::Exists(m_path))
    {
      bool accessible = false;
      if (m_path.substr(0, 8) == "image://")
        accessible = true;
      else
      {
        string sourceTypes[] = { "video", "music", "pictures" };
        unsigned int size = sizeof(sourceTypes) / sizeof(string);

        string realPath = URIUtils::GetRealPath(m_path);
        // for rar:// and zip:// paths we need to extract the path to the archive
        // instead of using the VFS path
        while (URIUtils::IsInArchive(realPath))
          realPath = CURL(realPath).GetHostName();

        VECSOURCES *sources = NULL;
        for (unsigned int index = 0; index < size && !accessible; index++)
        {
          sources = CMediaSourceSettings::Get().GetSources(sourceTypes[index]);
          if (sources == NULL)
            continue;

          for (VECSOURCES::const_iterator source = sources->begin(); source != sources->end() && !accessible; source++)
          {
            // don't allow access to locked / disabled sharing sources
            if (source->m_iHasLock == 2 || !source->m_allowSharing)
              continue;

            for (vector<CStdString>::const_iterator path = source->vecPaths.begin(); path != source->vecPaths.end(); path++)
            {
              string realSourcePath = URIUtils::GetRealPath(*path);
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
      {
        m_responseCode = MHD_HTTP_OK;
        m_responseType = HTTPFileDownload;
      }
      // the file exists but not in one of the defined sources so we deny access to it
      else
      {
        m_responseCode = MHD_HTTP_UNAUTHORIZED;
        m_responseType = HTTPError;
      }
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
