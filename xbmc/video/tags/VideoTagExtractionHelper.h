/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItem;

namespace KODI::VIDEO::TAGS
{
class CVideoTagExtractionHelper
{
public:
  /*!
   \brief Check whether tag extraction is supported for the given item. This is no
   check whether the video denoted by the item actually contains any tags. There is also a Kodi
   setting for enabling video tag support that is taken into consideration by the implementation of
   this function.
   \param item [in] the item denoting a video file.
   \return True if tag support is enabled and extraction is supported for the given file type.
   */
  static bool IsExtractionSupportedFor(const CFileItem& item);

  /*!
   \brief Try to extract embedded art of given type from the given item denoting a video file.
   \param item [in] the item.
   \param artType The type of the art to extract (e.g. "fanart").
   \return The image URL of the embedded art if extraction was successful, false otherwise.
   */
  static std::string ExtractEmbeddedArtFor(const CFileItem& item, const std::string& artType);
};
} // namespace KODI::VIDEO::TAGS
