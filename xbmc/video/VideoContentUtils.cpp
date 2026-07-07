/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoContentUtils.h"

#include "URL.h"
#include "utils/URIUtils.h"

namespace KODI::VIDEO
{

// For TV shows scraped with 'Single TV show in folder OFF' - pointing to folder /TV Shows/ or subfolder
// For TV shows scraped with 'Single TV show in folder ON' - pointing to folder /Show (2002)/ or subfolder

std::string DetermineContentForTVShows(bool scraperSetOnThisPath,
                                       bool isShowNameFolder,
                                       const TVShowEpisodePathResult& result)
{
  if (result.episodesInThisPath)
    return "episodes";

  // If episodes in archives the strPath in episode_view points inside
  // the archive (e.g. rar://host/archive.rar/episode.mkv).
  // When episodes exist in the root of the archive we treat the folder
  // containing the archive as containing episodes (this is handled in GetDirectory())
  for (const auto& path : result.candidatePaths)
  {
    const CURL url(path);
    if ((url.GetFileName().empty() && URIUtils::IsArchive(url)) || url.IsBlurayPath())
      return "episodes"; // episode in root of archive
  }

  // The scraper was attached directly to this path and single-show mode is not active
  // then the folder is the TV Show source root
  return scraperSetOnThisPath && !isShowNameFolder ? "tvshows" : "seasons";
}

} // namespace KODI::VIDEO
