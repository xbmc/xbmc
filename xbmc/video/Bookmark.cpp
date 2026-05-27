/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Bookmark.h"

#include "video/VideoDatabase.h"

CBookmark::CBookmark()
{
  Reset();
}

void CBookmark::Reset()
{
  episodeNumber = 0;
  seasonNumber = 0;
  timeInSeconds = 0.0;
  totalTimeInSeconds = 0.0;
  partNumber = 0;
  type = STANDARD;
}

bool CBookmark::IsSet() const
{
  return totalTimeInSeconds > 0.0;
}

bool CBookmark::IsPartWay() const
{
  return totalTimeInSeconds > 0.0 && timeInSeconds > 0.0;
}

bool CBookmark::HasSavedPlayerState() const
{
  return !playerState.empty();
}

bool CBookmark::GetBookmarksForFile(const std::string& filePath,
                                    VECBOOKMARKS& bookmarks,
                                    std::vector<EType> types)
{
  bookmarks.clear();

  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
    return false;

  for (CBookmark::EType type : types)
    videoDatabase.GetBookMarksForFile(filePath, bookmarks, type, true);

  videoDatabase.Close();
  return true;
}

std::vector<std::chrono::milliseconds> CBookmark::BookmarksToPositions(
    const VECBOOKMARKS& bookmarks)
{
  std::vector<std::chrono::milliseconds> result;
  for (const CBookmark& b : bookmarks)
  {
    if (b.type == CBookmark::STANDARD)
    {
      std::chrono::duration<double> sec(b.timeInSeconds);
      result.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(sec));
    }
  }
  std::ranges::sort(result);

  return result;
}

void CBookmark::AddToPositions(const CBookmark& bookmark,
                               std::vector<std::chrono::milliseconds>& positions)
{
  if (bookmark.type == CBookmark::STANDARD)
  {
    std::chrono::duration<double> sec(bookmark.timeInSeconds);
    positions.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(sec));
    std::ranges::sort(positions); //! @todo use a std::set instead?
  }
}
