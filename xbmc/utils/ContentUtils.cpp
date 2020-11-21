/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentUtils.h"

#include "FileItem.h"
#include "video/VideoInfoTag.h"

namespace
{
const bool HasPreferredArtType(const CFileItem& item)
{
  return item.HasVideoInfoTag() && (item.GetVideoInfoTag()->m_type == MediaTypeMovie ||
                                    item.GetVideoInfoTag()->m_type == MediaTypeTvShow ||
                                    item.GetVideoInfoTag()->m_type == MediaTypeSeason ||
                                    item.GetVideoInfoTag()->m_type == MediaTypeVideoCollection);
}

std::string GetPreferredArtType(const MediaType& type)
{
  if (type == MediaTypeMovie || type == MediaTypeTvShow || type == MediaTypeSeason ||
      type == MediaTypeVideoCollection)
  {
    return "poster";
  }
  return "thumb";
}
} // namespace

const std::string ContentUtils::GetPreferredArtImage(const CFileItem& item)
{
  if (HasPreferredArtType(item))
  {
    auto preferredArtType = GetPreferredArtType(item.GetVideoInfoTag()->m_type);
    if (item.HasArt(preferredArtType))
    {
      return item.GetArt(preferredArtType);
    }
  }
  return item.GetArt("thumb");
}
