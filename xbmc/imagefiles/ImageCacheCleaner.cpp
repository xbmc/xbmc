/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageCacheCleaner.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureDatabase.h"
#include "addons/AddonDatabase.h"
#include "music/MusicDatabase.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

namespace IMAGE_FILES
{
std::optional<IMAGE_FILES::CImageCacheCleaner> CImageCacheCleaner::Create()
{
  auto result = CImageCacheCleaner();
  if (result.m_valid)
    return std::move(result);
  return {};
}

CImageCacheCleaner::CImageCacheCleaner()
{
  bool valid = true;
  m_textureDB = std::make_unique<CTextureDatabase>();
  if (!m_textureDB->Open())
  {
    valid = false;
    CLog::LogF(LOGWARNING, "failed to initialize image cache cleaner: failed to open texture DB");
  }

  m_videoDB = std::make_unique<CVideoDatabase>();
  if (!m_videoDB->Open())
  {
    valid = false;
    CLog::LogF(LOGWARNING, "failed to initialize image cache cleaner: failed to open video DB");
  }

  m_musicDB = std::make_unique<CMusicDatabase>();
  if (!m_musicDB->Open())
  {
    valid = false;
    CLog::LogF(LOGWARNING, "failed to initialize image cache cleaner: failed to open music DB");
  }

  m_addonDB = std::make_unique<ADDON::CAddonDatabase>();
  if (!m_addonDB->Open())
  {
    valid = false;
    CLog::LogF(LOGWARNING, "failed to initialize image cache cleaner: failed to open add-on DB");
  }
  m_valid = valid;
}

CImageCacheCleaner::~CImageCacheCleaner()
{
  if (m_addonDB)
    m_addonDB->Close();
  if (m_musicDB)
    m_musicDB->Close();
  if (m_videoDB)
    m_videoDB->Close();
  if (m_textureDB)
    m_textureDB->Close();
}

CleanerResult CImageCacheCleaner::ScanOldestCache(unsigned int imageLimit)
{
  CLog::LogF(LOGDEBUG, "begin process to clean image cache");

  auto images = m_textureDB->GetOldestCachedImages(imageLimit);
  if (images.empty())
  {
    CLog::LogF(LOGDEBUG, "found no old cached images to process");
    return CleanerResult{0, {}, {}};
  }
  unsigned int processedCount = images.size();
  CLog::LogF(LOGDEBUG, "found {} old cached images to process", processedCount);

  auto usedImages = m_videoDB->GetUsedImages(images);

  auto nextUsedImages = m_musicDB->GetUsedImages(images);
  usedImages.insert(usedImages.end(), std::make_move_iterator(nextUsedImages.begin()),
                    std::make_move_iterator(nextUsedImages.end()));

  nextUsedImages = m_addonDB->GetUsedImages(images);
  usedImages.insert(usedImages.end(), std::make_move_iterator(nextUsedImages.begin()),
                    std::make_move_iterator(nextUsedImages.end()));

  images.erase(std::remove_if(images.begin(), images.end(),
                              [&usedImages](const std::string& image) {
                                return std::find(usedImages.cbegin(), usedImages.cend(), image) !=
                                       usedImages.cend();
                              }),
               images.end());

  m_textureDB->SetKeepCachedImages(usedImages);

  unsigned int keptCount = usedImages.size();
  CLog::LogF(LOGDEBUG, "cleaning {} unused images from cache, keeping {}", images.size(),
             keptCount);

  return CleanerResult{processedCount, keptCount, std::move(images)};
}
} // namespace IMAGE_FILES
