/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCache.h"

#include "ServiceBroker.h"
#include "TextureCacheJob.h"
#include "URL.h"
#include "commons/ilog.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/File.h"
#include "filesystem/IFileTypes.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageCacheCleaner.h"
#include "imagefiles/ImageFileURL.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/Crc32.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <chrono>
#include <exception>
#include <mutex>
#include <optional>
#include <string.h>

using namespace XFILE;
using namespace std::chrono_literals;

CTextureCache::CTextureCache()
  : CJobQueue(false, 1, CJob::PRIORITY_LOW_PAUSABLE), m_cleanTimer{[this]() { CleanTimer(); }}
{
}

CTextureCache::~CTextureCache() = default;

void CTextureCache::Initialize()
{
  m_cleanTimer.Start(60s);
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  if (!m_database.IsOpen())
    m_database.Open();
}

void CTextureCache::Deinitialize()
{
  CancelJobs();

  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  m_database.Close();
}

bool CTextureCache::IsCachedImage(const std::string &url) const
{
  if (url.empty())
    return false;

  if (!CURL::IsFullPath(url))
    return true;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return URIUtils::PathHasParent(url, "special://skin", true) ||
      URIUtils::PathHasParent(url, "special://temp", true) ||
      URIUtils::PathHasParent(url, "resource://", true) ||
      URIUtils::PathHasParent(url, "androidapp://", true)   ||
      URIUtils::PathHasParent(url, profileManager->GetThumbnailsFolder(), true);
}

bool CTextureCache::HasCachedImage(const std::string &url)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(url, details));
  return (!cachedImage.empty() && cachedImage != url);
}

std::string CTextureCache::GetCachedImage(const std::string &image, CTextureDetails &details, bool trackUsage)
{
  std::string url = IMAGE_FILES::ToCacheKey(image);
  if (url.empty())
    return "";
  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  if (GetCachedTexture(url, details))
  {
    if (details.file.empty())
      return {};

    if (trackUsage)
      IncrementUseCount(details);
    return GetCachedPath(details.file);
  }
  return "";
}

std::string CTextureCache::CheckCachedImage(const std::string &url, bool &needsRecaching)
{
  CTextureDetails details;
  std::string path(GetCachedImage(url, details, true));
  needsRecaching = !details.hash.empty();
  if (!path.empty())
    return path;
  return "";
}

void CTextureCache::BackgroundCacheImage(const std::string &url)
{
  if (url.empty())
    return;

  CTextureDetails details;
  std::string path(GetCachedImage(url, details));
  if (!path.empty() && details.hash.empty())
    return; // image is already cached and doesn't need to be checked further

  path = IMAGE_FILES::ToCacheKey(url);
  if (path.empty())
    return;

  // needs (re)caching
  AddJob(new CTextureCacheJob(path, details.hash));
}

bool CTextureCache::StartCacheImage(const std::string& image)
{
  std::unique_lock<CCriticalSection> lock(m_processingSection);
  std::set<std::string>::iterator i = m_processinglist.find(image);
  if (i == m_processinglist.end())
  {
    m_processinglist.insert(image);
    return true;
  }
  return false;
}

std::string CTextureCache::CacheImage(
    const std::string& image,
    std::unique_ptr<CTexture>* texture /*= nullptr*/,
    CTextureDetails* details /*= nullptr*/,
    unsigned int idealWidth /*= 0*/,
    unsigned int idealHeight /*= 0*/,
    CAspectRatio::AspectRatio aspectRatio /*= CAspectRatio::CENTER*/)
{
  std::string url = IMAGE_FILES::ToCacheKey(image);
  if (url.empty())
    return "";

  std::unique_lock<CCriticalSection> lock(m_processingSection);
  if (m_processinglist.find(url) == m_processinglist.end())
  {
    m_processinglist.insert(url);
    lock.unlock();
    // cache the texture directly
    CTextureCacheJob job(url);
    bool success = job.CacheTexture(texture);
    OnCachingComplete(success, &job);
    if (success && details)
      *details = job.m_details;
    return success ? GetCachedPath(job.m_details.file) : "";
  }
  lock.unlock();

  // wait for currently processing job to end.
  while (true)
  {
    m_completeEvent.Wait(1000ms);
    {
      std::unique_lock<CCriticalSection> lock(m_processingSection);
      if (m_processinglist.find(url) == m_processinglist.end())
        break;
    }
  }
  CTextureDetails tempDetails;
  if (!details)
    details = &tempDetails;

  std::string cachedpath = GetCachedImage(url, *details, true);
  if (!cachedpath.empty())
  {
    if (texture)
      *texture = CTexture::LoadFromFile(cachedpath, idealWidth, idealHeight, aspectRatio);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CTextureCache::{} - Return NULL texture because cache is not ready",
              __FUNCTION__);
  }

  return cachedpath;
}

bool CTextureCache::CacheImage(const std::string &image, CTextureDetails &details)
{
  std::string path = GetCachedImage(image, details);
  if (path.empty()) // not cached
    path = CacheImage(image, NULL, &details);

  return !path.empty();
}

void CTextureCache::ClearCachedImage(const std::string& image, bool deleteSource /*= false */)
{
  //! @todo This can be removed when the texture cache covers everything.
  const std::string url = IMAGE_FILES::ToCacheKey(image);
  std::string path = deleteSource ? url : "";
  std::string cachedFile;
  if (ClearCachedTexture(url, cachedFile))
    path = GetCachedPath(cachedFile);
  if (CFile::Exists(path))
    CFile::Delete(path);
  path = URIUtils::ReplaceExtension(path, ".dds");
  if (CFile::Exists(path))
    CFile::Delete(path);
}

bool CTextureCache::ClearCachedImage(int id)
{
  std::string cachedFile;
  if (ClearCachedTexture(id, cachedFile))
  {
    cachedFile = GetCachedPath(cachedFile);
    if (CFile::Exists(cachedFile))
      CFile::Delete(cachedFile);
    cachedFile = URIUtils::ReplaceExtension(cachedFile, ".dds");
    if (CFile::Exists(cachedFile))
      CFile::Delete(cachedFile);
    return true;
  }
  return false;
}

bool CTextureCache::GetCachedTexture(const std::string &url, CTextureDetails &details)
{
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  return m_database.GetCachedTexture(url, details);
}

bool CTextureCache::AddCachedTexture(const std::string &url, const CTextureDetails &details)
{
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  return m_database.AddCachedTexture(url, details);
}

void CTextureCache::IncrementUseCount(const CTextureDetails &details)
{
  static const size_t count_before_update = 100;
  std::unique_lock<CCriticalSection> lock(m_useCountSection);
  m_useCounts.reserve(count_before_update);
  m_useCounts.push_back(details);
  if (m_useCounts.size() >= count_before_update)
  {
    AddJob(new CTextureUseCountJob(m_useCounts));
    m_useCounts.clear();
  }
}

bool CTextureCache::SetCachedTextureValid(const std::string &url, bool updateable)
{
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  return m_database.SetCachedTextureValid(url, updateable);
}

bool CTextureCache::ClearCachedTexture(const std::string &url, std::string &cachedURL)
{
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

bool CTextureCache::ClearCachedTexture(int id, std::string &cachedURL)
{
  std::unique_lock<CCriticalSection> lock(m_databaseSection);
  return m_database.ClearCachedTexture(id, cachedURL);
}

std::string CTextureCache::GetCacheFile(const std::string &url)
{
  auto crc = Crc32::ComputeFromLowerCase(url);
  std::string hex = StringUtils::Format("{:08x}", crc);
  std::string hash = StringUtils::Format("{}/{}", hex[0], hex.c_str());
  return hash;
}

std::string CTextureCache::GetCachedPath(const std::string &file)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return URIUtils::AddFileToFolder(profileManager->GetThumbnailsFolder(), file);
}

void CTextureCache::OnCachingComplete(bool success, CTextureCacheJob *job)
{
  if (success)
  {
    if (job->m_details.hashRevalidated)
      SetCachedTextureValid(job->m_url, job->m_details.updateable);
    else
      AddCachedTexture(job->m_url, job->m_details);
  }

  { // remove from our processing list
    std::unique_lock<CCriticalSection> lock(m_processingSection);
    std::set<std::string>::iterator i = m_processinglist.find(job->m_url);
    if (i != m_processinglist.end())
      m_processinglist.erase(i);
  }

  m_completeEvent.Set();
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (strcmp(job->GetType(), kJobTypeCacheImage) == 0)
    OnCachingComplete(success, static_cast<CTextureCacheJob*>(job));
  return CJobQueue::OnJobComplete(jobID, success, job);
}

bool CTextureCache::Export(const std::string &image, const std::string &destination, bool overwrite)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(image, details));
  if (!cachedImage.empty())
  {
    std::string dest = destination + URIUtils::GetExtension(cachedImage);
    if (overwrite || !CFile::Exists(dest))
    {
      if (CFile::Copy(cachedImage, dest))
        return true;
      CLog::Log(LOGERROR, "{} failed exporting '{}' to '{}'", __FUNCTION__, cachedImage, dest);
    }
  }
  return false;
}

bool CTextureCache::Export(const std::string &image, const std::string &destination)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(image, details));
  if (!cachedImage.empty())
  {
    if (CFile::Copy(cachedImage, destination))
      return true;
    CLog::Log(LOGERROR, "{} failed exporting '{}' to '{}'", __FUNCTION__, cachedImage, destination);
  }
  return false;
}

bool CTextureCache::CleanAllUnusedImages()
{
  if (m_cleaningInProgress.test_and_set())
    return false;

  auto progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
      WINDOW_DIALOG_PROGRESS);
  if (progress)
  {
    progress->SetHeading(CVariant{14281}); //"Clean image cache"
    progress->SetText(CVariant{313}); //"Cleaning database"
    progress->SetPercentage(0);
    progress->Open();
    progress->ShowProgressBar(true);
  }

  bool failure = false;
  CServiceBroker::GetJobManager()->Submit([this, progress, &failure]()
                                          { failure = CleanAllUnusedImagesJob(progress); });

  // Wait for clean to complete or be canceled, but render every 10ms so that the
  // pointer movements work on dialog even when clean is reporting progress infrequently
  if (progress)
    progress->Wait();

  m_cleaningInProgress.clear();
  return !failure;
}

bool CTextureCache::CleanAllUnusedImagesJob(CGUIDialogProgress* progress)
{
  auto cleaner = IMAGE_FILES::CImageCacheCleaner::Create();
  if (!cleaner)
  {
    if (progress)
      progress->Close();
    return false;
  }

  const unsigned int cleanAmount = 1000000;
  const auto result = cleaner->ScanOldestCache(cleanAmount);
  if (progress && progress->IsCanceled())
  {
    progress->Close();
    return false;
  }

  const auto total = result.imagesToClean.size();
  unsigned int current = 0;
  for (const auto& image : result.imagesToClean)
  {
    ClearCachedImage(image);
    if (progress)
    {
      if (progress->IsCanceled())
      {
        progress->Close();
        return false;
      }
      int percentage = static_cast<unsigned long long>(current) * 100 / total;
      if (progress->GetPercentage() != percentage)
      {
        progress->SetPercentage(percentage);
        progress->Progress();
      }
      current++;
    }
  }

  if (progress)
    progress->Close();
  return true;
}

void CTextureCache::CleanTimer()
{
  if (IsSleeping())
  {
    CLog::LogF(LOGDEBUG, "Texture cleanup postponed. System is sleeping.");
    m_cleanTimer.Start(1h);
    return;
  }

  CServiceBroker::GetJobManager()->Submit(
      [this]()
      {
        auto next = m_cleaningInProgress.test_and_set() ? 1h : ScanOldestCache();
        m_cleaningInProgress.clear();
        m_cleanTimer.Start(next);
      },
      CJob::PRIORITY_LOW_PAUSABLE);
}

std::chrono::milliseconds CTextureCache::ScanOldestCache()
{
  auto cleaner = IMAGE_FILES::CImageCacheCleaner::Create();
  if (!cleaner)
    return std::chrono::hours(1);

  const unsigned int cleanAmount = 1000;
  const auto result = cleaner->ScanOldestCache(cleanAmount);
  for (const auto& image : result.imagesToClean)
  {
    ClearCachedImage(image);
  }

  // update in the next 6 - 48 hours depending on number of items processed
  const auto minTime = 6;
  const auto maxTime = 24;
  const auto zeroItemTime = 48;
  const unsigned int next =
      result.processedCount == 0
          ? zeroItemTime
          : minTime + (maxTime - minTime) *
                          (1 - (static_cast<float>(result.processedCount) / cleanAmount));
  CLog::LogF(LOGDEBUG, "scheduling the next image cache cleaning in {} hours", next);
  return std::chrono::hours(next);
}
