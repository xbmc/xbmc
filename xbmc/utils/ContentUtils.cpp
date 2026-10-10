/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentUtils.h"

#include "FileItem.h"
#include "utils/ArtTypes.h"
#include "utils/StringUtils.h"
#include "video/Bookmark.h"
#include "video/VideoInfoTag.h"

namespace
{
bool PrefersPoster(const CFileItem& item)
{
  if (!item.HasVideoInfoTag())
    return false;

  switch (item.GetVideoInfoTag()->GetMediaType())
  {
    case KODI::MEDIA::TYPE::MOVIE:
    case KODI::MEDIA::TYPE::TV_SHOW:
    case KODI::MEDIA::TYPE::SEASON:
    case KODI::MEDIA::TYPE::VIDEO_COLLECTION:
      return true;
    default:
      return false;
  }
}
} // namespace

const std::string ContentUtils::GetPreferredArtImage(const CFileItem& item)
{
  if (PrefersPoster(item) && item.HasArt(KODI::ART::TYPE::POSTER))
    return item.GetArt(KODI::ART::TYPE::POSTER);
  return item.GetArt(KODI::ART::TYPE::THUMB);
}

std::unique_ptr<CFileItem> ContentUtils::GeneratePlayableTrailerItem(const CFileItem& item,
                                                                     const std::string& label)
{
  std::unique_ptr<CFileItem> trailerItem = std::make_unique<CFileItem>();
  trailerItem->SetPath(item.GetVideoInfoTag()->m_strTrailer);
  CVideoInfoTag* videoInfoTag = trailerItem->GetVideoInfoTag();
  *videoInfoTag = *item.GetVideoInfoTag();
  videoInfoTag->m_streamDetails.Reset();
  videoInfoTag->SetFileNameAndPath(item.GetVideoInfoTag()->m_strTrailer);
  videoInfoTag->m_strFile.clear();
  videoInfoTag->m_strPath.clear();
  CBookmark resumePoint;
  resumePoint.type = CBookmark::RESUME;
  videoInfoTag->SetResumePoint(resumePoint);
  videoInfoTag->m_iBookmarkId = -1;
  // Assign a new bookmark rather than Reset(), which doesn't clear the saved player state
  CBookmark epBookmark;
  epBookmark.type = CBookmark::EPISODE;
  videoInfoTag->m_EpBookmark = epBookmark;
  videoInfoTag->ResetPlayCount();
  videoInfoTag->m_lastPlayed.Reset();
  videoInfoTag->m_strTitle = StringUtils::Format("{} ({})", videoInfoTag->m_strTitle, label);
  trailerItem->SetArt(item.GetArt());
  videoInfoTag->m_iDbId = -1;
  videoInfoTag->m_iFileId = -1;
  return trailerItem;
}
