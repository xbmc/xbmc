/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidTextureManager.h"
#include "platform/android/activity/JNIMainActivity.h"
#include "platform/android/activity/XBMCApp.h"
#include "textures/TextureManager.h"
#include "utils/log.h"
#include "threads/Thread.h"
#include "threads/SystemClock.h"
#include "ServiceBroker.h"
#include "utils/JobManager.h"

#include <algorithm>
#include <future>

int CAndroidTextureManager::m_deviceMemoryClass = 0;
bool CAndroidTextureManager::m_backgroundLoadingPaused = false;
std::mutex CAndroidTextureManager::m_textureLock;

class CBackgroundTextureJob : public CJob
{
public:
  CBackgroundTextureJob(const std::vector<std::string>& urls, 
                        std::function<void(std::map<std::string, CTextureDetails>)> callback)
    : m_urls(urls)
    , m_callback(callback)
  {}

  ~CBackgroundTextureJob() override = default;

  bool DoWork() override 
  {
    std::map<std::string, CTextureDetails> results;
    
    for (const auto& url : m_urls)
    {
      // Check if we should pause or cancel the background loading
      if (CAndroidTextureManager::m_backgroundLoadingPaused)
      {
        do {
          CThread::Sleep(50);
        } while (CAndroidTextureManager::m_backgroundLoadingPaused && !ShouldCancel());
        
        if (ShouldCancel())
          return false;
      }
      
      CTextureDetails details;
      if (CServiceBroker::GetTextureDatabase().GetCachedTexture(url, details))
      {
        results[url] = details;
      }
      
      // Yield after each texture to maintain UI responsiveness
      if (m_urls.size() > TEXTURE_LOAD_BATCH_SIZE)
      {
        CThread::Sleep(TEXTURE_LOAD_PAUSE_MS);
      }
    }
    
    // Execute callback on main thread
    if (m_callback)
    {
      m_callback(results);
    }
    
    return true;
  }
  
private:
  std::vector<std::string> m_urls;
  std::function<void(std::map<std::string, CTextureDetails>)> m_callback;
};

void CAndroidTextureManager::Initialize()
{
  // Query the device's memory class from Android
  JNIEnv* env = xbmc_jnienv();
  if (env)
  {
    jclass activityClass = env->FindClass(CJNIMainActivity::getClassLoader().c_str());
    if (activityClass)
    {
      jmethodID getMemoryClassMethod = env->GetMethodID(activityClass, "getMemoryClass", "()I");
      if (getMemoryClassMethod)
      {
        jobject activity = CJNIMainActivity::getActivity();
        m_deviceMemoryClass = env->CallIntMethod(activity, getMemoryClassMethod);
        CLog::Log(LOGINFO, "CAndroidTextureManager: Device memory class: %d MB", m_deviceMemoryClass);
      }
      env->DeleteLocalRef(activityClass);
    }
  }
  
  // If we couldn't get the memory class, use a safe default
  if (m_deviceMemoryClass <= 0)
  {
    m_deviceMemoryClass = MEM_THRESHOLD_LOW_MB;
    CLog::Log(LOGWARNING, "CAndroidTextureManager: Could not determine memory class, defaulting to %d MB", m_deviceMemoryClass);
  }
  
  // Initialize paused state
  m_backgroundLoadingPaused = false;
}

int CAndroidTextureManager::GetDeviceMemoryClass()
{
  return m_deviceMemoryClass;
}

bool CAndroidTextureManager::AddCachedTextureOptimized(const std::string &url, const CTextureDetails &details)
{
  // Check if running on device with limited memory
  bool isLowMemoryDevice = (m_deviceMemoryClass < MEM_THRESHOLD_MEDIUM_MB);
  bool isMediumMemoryDevice = (m_deviceMemoryClass >= MEM_THRESHOLD_MEDIUM_MB && m_deviceMemoryClass < MEM_THRESHOLD_HIGH_MB);
  
  // For low/medium memory Android devices, use more aggressive texture optimization
  if (isLowMemoryDevice || isMediumMemoryDevice)
  {
    CTextureDetails optimizedDetails = details;
    
    int maxWidth, maxHeight;
    if (isLowMemoryDevice)
    {
      maxWidth = MAX_TEXTURE_WIDTH_FOR_LOW_MEM;
      maxHeight = MAX_TEXTURE_HEIGHT_FOR_LOW_MEM;
    }
    else // medium memory device
    {
      maxWidth = MAX_TEXTURE_WIDTH_FOR_MEDIUM_MEM;
      maxHeight = MAX_TEXTURE_HEIGHT_FOR_MEDIUM_MEM;
    }
    
    // Only resize if texture is larger than our maximums
    if (details.width > maxWidth || details.height > maxHeight)
    {
      // Calculate aspect ratio preserving dimensions
      float aspectRatio = (float)details.width / details.height;
      if (details.width > details.height)
      {
        optimizedDetails.width = maxWidth;
        optimizedDetails.height = (int)(maxWidth / aspectRatio);
      }
      else
      {
        optimizedDetails.height = maxHeight;
        optimizedDetails.width = (int)(maxHeight * aspectRatio);
      }
      
      CLog::Log(LOGDEBUG, "CAndroidTextureManager: Optimizing texture %s from %dx%d to %dx%d",
                url.c_str(), details.width, details.height, optimizedDetails.width, optimizedDetails.height);
    }
    
    return CServiceBroker::GetTextureDatabase().AddCachedTexture(url, optimizedDetails);
  }
  
  // For high memory devices, use standard caching
  return CServiceBroker::GetTextureDatabase().AddCachedTexture(url, details);
}

void CAndroidTextureManager::GetCachedTexturesAsync(const std::vector<std::string> &urls,
                                                std::function<void(std::map<std::string, CTextureDetails>)> callback)
{
  if (urls.empty())
  {
    if (callback)
    {
      callback(std::map<std::string, CTextureDetails>());
    }
    return;
  }
  
  // Create a background job for texture loading
  std::shared_ptr<CBackgroundTextureJob> job = std::make_shared<CBackgroundTextureJob>(urls, callback);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}

void CAndroidTextureManager::PauseBackgroundLoading(bool pause)
{
  std::unique_lock<std::mutex> lock(m_textureLock);
  m_backgroundLoadingPaused = pause;
  
  if (pause)
  {
    CLog::Log(LOGDEBUG, "CAndroidTextureManager: Pausing background texture loading");
  }
  else
  {
    CLog::Log(LOGDEBUG, "CAndroidTextureManager: Resuming background texture loading");
  }
}

void CAndroidTextureManager::CleanupTextureCache()
{
  // Check device memory and adjust cleanup strategy
  int memClass = GetDeviceMemoryClass();
  int maxTextureCacheSizeMB;
  
  if (memClass < MEM_THRESHOLD_LOW_MB)
  {
    maxTextureCacheSizeMB = 100; // Very aggressive (100MB)
  }
  else if (memClass < MEM_THRESHOLD_MEDIUM_MB)
  {
    maxTextureCacheSizeMB = 250; // Aggressive (250MB)
  }
  else if (memClass < MEM_THRESHOLD_HIGH_MB)
  {
    maxTextureCacheSizeMB = 500; // Standard (500MB)
  }
  else
  {
    maxTextureCacheSizeMB = 1000; // Generous (1GB)
  }
  
  CLog::Log(LOGINFO, "CAndroidTextureManager: Cleaning texture cache with limit of %d MB", maxTextureCacheSizeMB);
  
  // Calculate current texture cache size
  int64_t currentSize = 0;
  std::vector<CTextureDetails> textures;
  
  if (CServiceBroker::GetTextureDatabase().GetTextures(textures, currentSize))
  {
    int64_t maxSize = static_cast<int64_t>(maxTextureCacheSizeMB) * 1024 * 1024;
    
    if (currentSize > maxSize)
    {
      CLog::Log(LOGINFO, "CAndroidTextureManager: Current texture cache size (%lld MB) exceeds limit, cleaning...", 
                currentSize / (1024 * 1024));
      
      // Sort textures by last used time so we can delete oldest first
      std::sort(textures.begin(), textures.end(), 
               [](const CTextureDetails &a, const CTextureDetails &b) {
                 return a.lastUsed < b.lastUsed;
               });
      
      // Delete textures until we're under the maximum size
      int64_t deletedSize = 0;
      for (const auto& texture : textures)
      {
        if (currentSize - deletedSize <= maxSize)
          break;
        
        if (CServiceBroker::GetTextureDatabase().ClearCachedTexture(texture.url))
        {
          deletedSize += texture.size;
          CLog::Log(LOGDEBUG, "CAndroidTextureManager: Removed texture %s, size: %lld bytes, last used: %s", 
                    texture.url.c_str(), texture.size, texture.lastUsed.GetAsDBDateTime().c_str());
        }
      }
      
      CLog::Log(LOGINFO, "CAndroidTextureManager: Removed %lld MB from texture cache", deletedSize / (1024 * 1024));
    }
  }
}