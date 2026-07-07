/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI::VIDEO
{

/*!
 * \brief Raw data collected from the DB for a given path during TV show content resolution.
 */
struct TVShowEpisodePathResult
{
  //! True when episode_view contains a row whose strPath exactly matches the queried path.
  bool episodesInThisPath{false};

  //! Distinct strPath values from episode_view whose parent path idPath matches the queried path.
  std::vector<std::string> candidatePaths{};
};

/*!
 * \brief Determine the content label for a TV show path from pre-fetched DB results.
 *
 * \param scraperSetOnThisPath  True if the scraper was configured directly on \p strPath
 *                              (not inherited from a parent path).
 * \param isShowNameFolder      True if \p strPath is the base folder for a show
 *                              ie. /TV Shows/Show Name (2002)/ and not /TV Shows/Show Name (2002)/Season 1/
 *                              (derived from SScanSettings::parent_name for the scraper)
 * \param result                Pre-fetched query results for the path.
 * \return One of "episodes", "seasons", or "tvshows".
 */
std::string DetermineContentForTVShows(bool scraperSetOnThisPath,
                                       bool isShowNameFolder,
                                       const TVShowEpisodePathResult& result);

} // namespace KODI::VIDEO
