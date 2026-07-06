/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpisodesDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <string>
#include <vector>

using namespace XFILE;

bool CEpisodesDirectory::Resolve(CFileItem& item) const
{
  const CURL url{item.GetDynPath()};
  if (url.GetProtocol() != "episodes")
    return false;
  return true;
}

bool CEpisodesDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  CLog::LogF(LOGDEBUG, "Retrieving all episodes for URL: {}", url.Get());

  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }

  const std::string& path{url.GetHostName()};
  const std::string show{url.GetOption("show")};
  int idShow{-1};
  if (!show.empty())
    idShow = static_cast<int>(StringUtils::ToUint32(show));
  std::vector<CVideoInfoTag> episodes;
  database.GetEpisodesByBasePath(path, episodes, idShow);
  database.Close();
  if (episodes.empty())
  {
    CLog::LogF(LOGERROR, "No episodes found for path: {}", path);
    return false;
  }

  for (const auto& episode : episodes)
  {
    auto item{std::make_shared<CFileItem>(episode)};
    items.Add(item);
  }
  return !items.IsEmpty();
}
