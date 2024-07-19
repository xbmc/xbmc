/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class CTextureDatabase;
class CVideoDatabase;
class CMusicDatabase;
namespace ADDON
{
class CAddonDatabase;
}

namespace IMAGE_FILES
{
struct CleanerResult
{
  unsigned int processedCount;
  unsigned int keptCount;
  std::vector<std::string> imagesToClean;
};

/*!
 * @brief Clean old unused images from the image cache.
 */
class CImageCacheCleaner
{
public:
  static std::optional<IMAGE_FILES::CImageCacheCleaner> Create();
  ~CImageCacheCleaner();
  CImageCacheCleaner(const CImageCacheCleaner&) = delete;
  CImageCacheCleaner& operator=(const CImageCacheCleaner&) = delete;
  CImageCacheCleaner(CImageCacheCleaner&&) = default;
  CImageCacheCleaner& operator=(CImageCacheCleaner&&) = default;

  CleanerResult ScanOldestCache(unsigned int imageLimit);

private:
  CImageCacheCleaner();
  bool m_valid;

  std::unique_ptr<CTextureDatabase> m_textureDB;
  std::unique_ptr<CVideoDatabase> m_videoDB;
  std::unique_ptr<CMusicDatabase> m_musicDB;
  std::unique_ptr<ADDON::CAddonDatabase> m_addonDB;
};
} // namespace IMAGE_FILES
