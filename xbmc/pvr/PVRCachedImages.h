/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace PVR
{

struct PVRImagePattern
{
  PVRImagePattern(const std::string& _owner, const std::string& _path) : owner(_owner), path(_path)
  {
  }

  std::string owner;
  std::string path;
};

class CPVRCachedImages
{
public:
  /*!
   * @brief Erase stale texture db entries and image files.
   * @param urlPatterns The URL patterns to fetch from texture database.
   * @param urlsToCheck The URLs to check for still being present in the texture db.
   * @param clearTextureForPath Whether to clear the path in texture database.
   * @return number of cleaned up images.
   */
  static int Cleanup(const std::vector<PVRImagePattern>& urlPatterns,
                     const std::vector<std::string>& urlsToCheck,
                     bool clearTextureForPath = false);
};

} // namespace PVR
