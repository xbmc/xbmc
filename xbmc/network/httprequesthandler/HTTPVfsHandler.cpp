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
#include "utils/Screenshot.h"
#include "utils/URIUtils.h"

#include <string_view>
#include <vector>

using KODI::MEDIA::MediaSection;

namespace
{
constexpr std::string_view VFS_ROOT{"/vfs/"};

//! Whether \p realPath lies in a source that allows sharing, configured or mounted
bool IsInSharedSource(const std::string& realPath)
{
  for (const MediaSection section :
       {MediaSection::VIDEO, MediaSection::MUSIC, MediaSection::PICTURES})
  {
    for (const auto& source : CMediaSourceSettings::GetInstance().GetSources(section))
    {
      if (source.GetLockInfo().IsLocked() || !source.m_allowSharing)
        continue;

      for (const auto& path : source.vecPaths)
      {
        if (URIUtils::PathHasParent(realPath, URIUtils::GetRealPath(path), true))
          return true;
      }
    }
  }

  std::vector<CMediaSource> removableSources;
  CServiceBroker::GetMediaManager().GetRemovableDrives(removableSources);
  bool isSource;
  const int sourceIndex{CUtil::GetMatchingSource(realPath, removableSources, isSource)};
  return sourceIndex >= 0 && sourceIndex < static_cast<int>(removableSources.size()) &&
         !removableSources[sourceIndex].GetLockInfo().IsLocked() &&
         removableSources[sourceIndex].m_allowSharing;
}

//! Whether \p file may be served: an image, a screenshot, or a file in a shared source
bool IsAccessible(const std::string& file)
{
  if (URIUtils::IsProtocol(file, "image") || CScreenShot::IsScreenshotPath(file))
    return true;

  std::string realPath = URIUtils::GetRealPath(file);
  // for rar:// and zip:// paths we need to extract the path to the archive instead of using the VFS path
  while (URIUtils::IsInArchive(realPath))
    realPath = CURL(realPath).GetHostName();

  return IsInSharedSource(realPath);
}
} // namespace

CHTTPVfsHandler::CHTTPVfsHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  std::string file;
  int responseStatus = MHD_HTTP_BAD_REQUEST;

  if (m_request.pathUrl.size() > VFS_ROOT.size())
  {
    file = m_request.pathUrl.substr(VFS_ROOT.size());

    if (!CFileUtils::Exists(file))
      responseStatus = MHD_HTTP_NOT_FOUND;
    else if (IsAccessible(file))
      responseStatus = MHD_HTTP_OK;
    // the file exists but not in one of the defined sources so we deny access to it
    else
      responseStatus = MHD_HTTP_UNAUTHORIZED;
  }

  // set the file and the HTTP response status
  SetFile(file, responseStatus);
}

bool CHTTPVfsHandler::CanHandleRequest(const HTTPRequest &request) const
{
  return request.pathUrl.starts_with(VFS_ROOT);
}
