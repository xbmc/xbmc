/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CheevosImageCache.h"

#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdint>
#include <mutex>

using namespace KODI::RETRO;

namespace
{
constexpr auto RA_IMAGE_CACHE_DIRECTORY = "special://profile/cache/retroachievements/icons/";
constexpr uint64_t RA_IMAGE_CACHE_MAX_BYTES = 50 * 1024 * 1024;
constexpr uint64_t RA_IMAGE_CACHE_TARGET_BYTES = 40 * 1024 * 1024;

std::mutex g_imageCacheMutex;

bool WriteComplete(XFILE::CFile& file, const std::string& data)
{
  std::size_t totalBytesWritten = 0;
  while (totalBytesWritten < data.size())
  {
    const ssize_t bytesWritten =
        file.Write(data.data() + totalBytesWritten, data.size() - totalBytesWritten);
    if (bytesWritten <= 0)
      return false;

    totalBytesWritten += static_cast<std::size_t>(bytesWritten);
  }

  return true;
}
} // namespace

void CCheevosImageCache::CleanIfNeeded()
{
  std::lock_guard<std::mutex> lock(g_imageCacheMutex);

  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(RA_IMAGE_CACHE_DIRECTORY, items, "",
                                       XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    return;
  }

  uint64_t totalSize = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const int64_t fileSize = items[i]->GetSize();
    if (fileSize > 0)
      totalSize += static_cast<uint64_t>(fileSize);
  }

  if (totalSize <= RA_IMAGE_CACHE_MAX_BYTES)
    return;

  CLog::Log(LOGINFO, "CCheevos: image cache size {}MB exceeds limit, evicting oldest files",
            totalSize / (1024 * 1024));

  items.Sort(SortBy::DATE, SortOrder::ASCENDING);

  for (int i = 0; i < items.Size() && totalSize > RA_IMAGE_CACHE_TARGET_BYTES; ++i)
  {
    const int64_t fileSize = items[i]->GetSize();
    if (XFILE::CFile::Delete(items[i]->GetPath()))
    {
      if (fileSize > 0)
        totalSize -= static_cast<uint64_t>(fileSize);
      CLog::Log(LOGDEBUG, "CCheevos: evicted cache file: {}", items[i]->GetPath());
    }
  }

  CLog::Log(LOGINFO, "CCheevos: image cache cleaned, new size {}MB", totalSize / (1024 * 1024));
}

std::string CCheevosImageCache::GetGameIconPath(unsigned int gameId)
{
  return std::string(RA_IMAGE_CACHE_DIRECTORY) + StringUtils::Format("game_{}.png", gameId);
}

std::string CCheevosImageCache::GetBadgePath(unsigned int achievementId)
{
  return std::string(RA_IMAGE_CACHE_DIRECTORY) + StringUtils::Format("badge_{}.png", achievementId);
}

bool CCheevosImageCache::IsCached(const std::string& path)
{
  std::lock_guard<std::mutex> lock(g_imageCacheMutex);
  return XFILE::CFile::Exists(path);
}

bool CCheevosImageCache::Store(const std::string& path, const std::string& data)
{
  if (data.empty())
    return false;

  std::lock_guard<std::mutex> lock(g_imageCacheMutex);

  XFILE::CDirectory::Create(RA_IMAGE_CACHE_DIRECTORY);

  XFILE::CFile outFile;
  if (!outFile.OpenForWrite(path, true))
    return false;

  const bool writeSucceeded = WriteComplete(outFile, data);
  outFile.Close();
  if (!writeSucceeded)
    XFILE::CFile::Delete(path);

  return writeSucceeded;
}
