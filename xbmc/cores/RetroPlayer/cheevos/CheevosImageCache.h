/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::RETRO
{

class CCheevosImageCache
{
public:
  /*!
   * \brief Evict the oldest cached images when the cache exceeds its size limit
   */
  static void CleanIfNeeded();

  static std::string GetGameIconPath(unsigned int gameId);
  static std::string GetBadgePath(unsigned int achievementId);

  static bool IsCached(const std::string& path);

  /*!
   * \brief Store complete image data, removing the destination if the write fails
   */
  static bool Store(const std::string& path, const std::string& data);
};

} // namespace KODI::RETRO
