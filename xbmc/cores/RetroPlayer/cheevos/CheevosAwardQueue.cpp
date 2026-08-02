/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CheevosAwardQueue.h"

#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>

using namespace KODI::RETRO;

namespace
{
constexpr auto RA_AWARD_QUEUE_DIRECTORY = "special://profile/cache/retroachievements/";
constexpr auto RA_AWARD_QUEUE_FILE = "special://profile/cache/retroachievements/pending_awards.txt";
constexpr auto RA_AWARD_QUEUE_TEMP_FILE =
    "special://profile/cache/retroachievements/pending_awards.tmp";
constexpr std::size_t RA_AWARD_QUEUE_MAX_BYTES = 64 * 1024;
constexpr unsigned int RA_AWARD_QUEUE_MAX_ENTRIES = 50;

std::mutex g_awardQueueMutex;

bool WriteComplete(XFILE::CFile& file, const std::string& data)
{
  const ssize_t bytesWritten = file.Write(data.data(), data.size());
  return bytesWritten >= 0 && static_cast<std::size_t>(bytesWritten) == data.size();
}

bool LoadAwardQueue(std::vector<uint8_t>& data)
{
  if (!XFILE::CFile::Exists(RA_AWARD_QUEUE_FILE))
  {
    if (!XFILE::CFile::Exists(RA_AWARD_QUEUE_TEMP_FILE))
      return false;

    if (!XFILE::CFile::Rename(RA_AWARD_QUEUE_TEMP_FILE, RA_AWARD_QUEUE_FILE))
    {
      CLog::Log(LOGERROR, "CCheevos: failed to recover temporary award queue");
      return false;
    }
  }

  XFILE::CFile queueFile;
  if (!queueFile.Open(CURL(RA_AWARD_QUEUE_FILE)))
    return false;

  const int64_t queueSize = queueFile.GetLength();
  if (queueSize < 0)
  {
    CLog::Log(LOGERROR, "CCheevos: failed to determine award queue size");
    return false;
  }

  if (static_cast<uint64_t>(queueSize) > RA_AWARD_QUEUE_MAX_BYTES)
  {
    CLog::Log(LOGERROR, "CCheevos: award queue exceeds {} byte limit", RA_AWARD_QUEUE_MAX_BYTES);
    return false;
  }

  data.resize(static_cast<std::size_t>(queueSize));
  std::size_t totalBytesRead = 0;
  while (totalBytesRead < data.size())
  {
    const ssize_t bytesRead =
        queueFile.Read(data.data() + totalBytesRead, data.size() - totalBytesRead);
    if (bytesRead <= 0)
    {
      data.clear();
      CLog::Log(LOGERROR, "CCheevos: failed to read complete award queue");
      return false;
    }
    totalBytesRead += static_cast<std::size_t>(bytesRead);
  }

  return true;
}

bool StoreAwardQueue(const std::string& data)
{
  XFILE::CFile::Delete(RA_AWARD_QUEUE_TEMP_FILE);

  XFILE::CFile outFile;
  if (!outFile.OpenForWrite(CURL(RA_AWARD_QUEUE_TEMP_FILE), true))
  {
    CLog::Log(LOGERROR, "CCheevos: failed to open temporary award queue");
    return false;
  }

  if (!WriteComplete(outFile, data))
  {
    outFile.Close();
    XFILE::CFile::Delete(RA_AWARD_QUEUE_TEMP_FILE);
    CLog::Log(LOGERROR, "CCheevos: failed to write complete award queue");
    return false;
  }
  outFile.Close();

  if (XFILE::CFile::Exists(RA_AWARD_QUEUE_FILE) && !XFILE::CFile::Delete(RA_AWARD_QUEUE_FILE))
  {
    XFILE::CFile::Delete(RA_AWARD_QUEUE_TEMP_FILE);
    CLog::Log(LOGERROR, "CCheevos: failed to replace award queue");
    return false;
  }

  if (!XFILE::CFile::Rename(RA_AWARD_QUEUE_TEMP_FILE, RA_AWARD_QUEUE_FILE))
  {
    // Keep the complete temporary file so LoadAwardQueue can recover it.
    CLog::Log(LOGERROR, "CCheevos: failed to install updated award queue");
    return false;
  }

  return true;
}
} // namespace

void CCheevosAwardQueue::Flush()
{
  std::lock_guard<std::mutex> queueLock(g_awardQueueMutex);

  std::vector<uint8_t> data;
  if (!LoadAwardQueue(data) || data.empty())
    return;

  const std::string queueData(data.begin(), data.end());
  std::vector<std::string> urls = StringUtils::Split(queueData, "\n");

  std::vector<std::string> remaining;
  for (const auto& url : urls)
  {
    if (url.empty())
      continue;

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", CSysInfo::GetUserAgent());
    std::string response;
    if (curl.Get(url, response))
      CLog::Log(LOGINFO, "CCheevos: flushed queued award: {}", url);
    else
    {
      CLog::Log(LOGWARNING, "CCheevos: queued award still failing, keeping: {}", url);
      remaining.push_back(url);
    }
  }

  // Rewrite queue with only the still-failing ones
  if (remaining.empty())
  {
    if (!XFILE::CFile::Delete(RA_AWARD_QUEUE_FILE))
      CLog::Log(LOGERROR, "CCheevos: failed to remove empty award queue");
  }
  else
  {
    const std::string newData = StringUtils::Join(remaining, "\n") + "\n";
    StoreAwardQueue(newData);
  }
}

void CCheevosAwardQueue::Queue(const std::string& url)
{
  std::lock_guard<std::mutex> queueLock(g_awardQueueMutex);

  std::vector<uint8_t> existingData;
  if ((XFILE::CFile::Exists(RA_AWARD_QUEUE_FILE) ||
       XFILE::CFile::Exists(RA_AWARD_QUEUE_TEMP_FILE)) &&
      !LoadAwardQueue(existingData))
  {
    return;
  }

  if (!existingData.empty())
  {
    const std::string existing(existingData.begin(), existingData.end());
    const std::size_t lineCount =
        static_cast<std::size_t>(std::count(existing.begin(), existing.end(), '\n'));
    if (lineCount >= RA_AWARD_QUEUE_MAX_ENTRIES)
    {
      CLog::Log(LOGWARNING, "CCheevos: award queue full ({} entries), dropping: {}", lineCount,
                url);
      return;
    }
  }

  XFILE::CDirectory::Create(RA_AWARD_QUEUE_DIRECTORY);
  const std::string line = url + "\n";
  if (existingData.size() + line.size() > RA_AWARD_QUEUE_MAX_BYTES)
  {
    CLog::Log(LOGWARNING, "CCheevos: award queue size limit reached, dropping: {}", url);
    return;
  }

  std::string updatedQueue(existingData.begin(), existingData.end());
  updatedQueue += line;
  if (StoreAwardQueue(updatedQueue))
  {
    CLog::Log(LOGWARNING, "CCheevos: award queued for retry: {}", url);
  }
}
