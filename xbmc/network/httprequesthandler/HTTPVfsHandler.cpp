/*
 *  Copyright (C) 2011-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPVfsHandler.h"

#include "MediaSource.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "media/MediaLockState.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

CHTTPVfsHandler::CHTTPVfsHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  std::string file;
  int responseStatus = MHD_HTTP_BAD_REQUEST;

  if (m_request.pathUrl.size() > 5)
  {
    file = m_request.pathUrl.substr(5);

    if (CFileUtils::Exists(file))
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

        // Check manually configured sources
        VECSOURCES *sources = NULL;
        for (unsigned int index = 0; index < size && !accessible; index++)
        {
          sources = CMediaSourceSettings::GetInstance().GetSources(sourceTypes[index]);
          if (sources == NULL)
            continue;

          for (const auto& source : *sources)
          {
            if (accessible)
              break;

            // don't allow access to locked / disabled sharing sources
            if (source.m_iHasLock == LOCK_STATE_LOCKED || !source.m_allowSharing)
              continue;

            for (const auto& path : source.vecPaths)
            {
              std::string realSourcePath = URIUtils::GetRealPath(path);
              if (URIUtils::PathHasParent(realPath, realSourcePath, true))
              {
                accessible = true;
                break;
              }
            }
          }
        }

        // Check auto-mounted sources
        if (!accessible)
        {
          bool isSource;
          VECSOURCES removableSources;
          CServiceBroker::GetMediaManager().GetRemovableDrives(removableSources);
          int sourceIndex = CUtil::GetMatchingSource(realPath, removableSources, isSource);
          if (sourceIndex >= 0 && sourceIndex < static_cast<int>(removableSources.size()) &&
              removableSources.at(sourceIndex).m_iHasLock != LOCK_STATE_LOCKED &&
              removableSources.at(sourceIndex).m_allowSharing)
            accessible = true;
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

bool CHTTPVfsHandler::CanHandleRequest(const HTTPRequest &request) const
{
  return request.pathUrl.find("/vfs") == 0;
}
