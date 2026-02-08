/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUILargeTextureManager.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "commons/ilog.h"
#include "guilib/GUIComponent.h"
#include "guilib/Texture.h"
#include "utils/JobManager.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <cassert>
#include <chrono>
#include <exception>
#include <mutex>

CImageLoader::CImageLoader(const std::string& path, const bool useCache)
  : m_path(path), m_texture(nullptr)
{
  m_use_cache = useCache;
}

CImageLoader::~CImageLoader() = default;

bool CImageLoader::DoWork()
{
  bool needsChecking = false;
  std::string loadPath;

  std::string texturePath = CServiceBroker::GetGUI()->GetTextureManager().GetTexturePath(m_path);
  if (texturePath.empty())
    return false;

  if (m_use_cache)
    loadPath = CServiceBroker::GetTextureCache()->CheckCachedImage(texturePath, needsChecking);
  else
    loadPath = texturePath;

  if (!loadPath.empty())
  {
    // direct route - load the image
    auto start = std::chrono::steady_clock::now();
    m_texture =
        CTexture::LoadFromFile(loadPath, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                               CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (duration.count() > 100)
      CLog::Log(LOGDEBUG, "{} - took {} ms to load {}", __FUNCTION__, duration.count(), loadPath);

    if (m_texture)
    {
      if (needsChecking)
        CServiceBroker::GetTextureCache()->BackgroundCacheImage(texturePath);

      return true;
    }

    // Fallthrough on failure:
    CLog::Log(LOGERROR, "{} - Direct texture file loading failed for {}", __FUNCTION__, loadPath);
  }

  if (!m_use_cache)
    return false; // We're done

  // not in our texture cache or it failed to load from it, so try and load directly and then cache the result
  CServiceBroker::GetTextureCache()->CacheImage(texturePath, &m_texture);
  return (m_texture != nullptr);
}

CGUILargeTextureManager::CLargeTexture::CLargeTexture(const std::string &path):
  m_path(path)
{
  m_refCount = 1;
  m_timeToDelete = 0;
}

CGUILargeTextureManager::CLargeTexture::~CLargeTexture()
{
  assert(m_refCount == 0);
  m_texture.Free();
}

void CGUILargeTextureManager::CLargeTexture::AddRef()
{
  m_refCount++;
}

bool CGUILargeTextureManager::CLargeTexture::DecrRef(bool deleteImmediately)
{
  assert(m_refCount);
  m_refCount--;
  if (m_refCount == 0)
  {
    if (deleteImmediately)
      delete this;
    else
      m_timeToDelete = CTimeUtils::GetFrameTime() + TIME_TO_DELETE;
    return true;
  }
  return false;
}

bool CGUILargeTextureManager::CLargeTexture::DeleteIfRequired(bool deleteImmediately) const {
  if (m_refCount == 0 && (deleteImmediately || m_timeToDelete < CTimeUtils::GetFrameTime()))
  {
    delete this;
    return true;
  }
  return false;
}

void CGUILargeTextureManager::CLargeTexture::SetTexture(std::unique_ptr<CTexture> texture)
{
  assert(!m_texture.size());
  if (texture)
  {
    const auto width = texture->GetWidth();
    const auto height = texture->GetHeight();
    m_texture.Set(std::move(texture), width, height);
  }
}

CGUILargeTextureManager::CGUILargeTextureManager() = default;

CGUILargeTextureManager::~CGUILargeTextureManager() = default;

void CGUILargeTextureManager::CleanupUnusedImages(bool immediately)
{
  std::lock_guard lock(m_listSection);

  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    // Save path before potential deletion to avoid use-after-free
    const std::string path = image->GetPath();
    if (image->DeleteIfRequired(immediately))
    {
      m_allocatedLookup.erase(path);
      it = m_allocated.erase(it);
    }
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
bool CGUILargeTextureManager::GetImage(const std::string &path, CTextureArray &texture, bool firstRequest, const bool useCache)
{
  std::unique_lock<CCriticalSection> lock(m_listSection);

  // O(1) lookup in allocated textures
  auto it = m_allocatedLookup.find(path);
  if (it != m_allocatedLookup.end())
  {
    CLargeTexture *image = it->second;
    if (firstRequest)
      image->AddRef();
    texture = image->GetTexture();
    return texture.size() > 0;
  }

  if (firstRequest)
    QueueImage(path, useCache);

  return true;
}

void CGUILargeTextureManager::ReleaseImage(const std::string &path, bool immediately)
{
  std::unique_lock<CCriticalSection> lock(m_listSection);

  // O(1) lookup in allocated textures
  auto allocIt = m_allocatedLookup.find(path);
  if (allocIt != m_allocatedLookup.end())
  {
    CLargeTexture *image = allocIt->second;
    if (image->DecrRef(immediately) && immediately)
    {
      m_allocatedLookup.erase(allocIt);
      // Also remove from vector
      for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
      {
        if (*it == image)
        {
          m_allocated.erase(it);
          break;
        }
      }
    }
    return;
  }

  // O(1) lookup in queued textures
  auto queueIt = m_queuedLookup.find(path);
  if (queueIt != m_queuedLookup.end())
  {
    CLargeTexture *image = queueIt->second;
    if (image->DecrRef(true))
    {
      // Find and cancel the job, remove from queue vector
      for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
      {
        if (it->second == image)
        {
          CServiceBroker::GetJobManager()->CancelJob(it->first);
          m_queued.erase(it);
          break;
        }
      }
      m_queuedLookup.erase(queueIt);
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(const std::string &path, bool useCache)
{
  if (path.empty())
    return;

  std::unique_lock<CCriticalSection> lock(m_listSection);

  // O(1) lookup to check if already queued
  auto it = m_queuedLookup.find(path);
  if (it != m_queuedLookup.end())
  {
    it->second->AddRef();
    return; // already queued
  }

  // queue the item
  auto image = new CLargeTexture(path);
  unsigned int jobID = CServiceBroker::GetJobManager()->AddJob(new CImageLoader(path, useCache),
                                                               this, CJob::PRIORITY_NORMAL);
  m_queued.emplace_back(jobID, image);
  m_queuedLookup[path] = image;
}

void CGUILargeTextureManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // see if we still have this job id
  std::lock_guard lock(m_listSection);
  
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    if (it->first == jobID)
    { // found our job
      auto loader = static_cast<CImageLoader*>(job);
      CLargeTexture *image = it->second;
      image->SetTexture(std::move(loader->m_texture));
      loader->m_texture = NULL; // we want to keep the texture, and jobs are auto-deleted.

      // Move from queued to allocated - update both containers and lookup maps
      const std::string& path = image->GetPath();
      m_queuedLookup.erase(path);
      m_queued.erase(it);
      m_allocated.push_back(image);
      m_allocatedLookup[path] = image;
      return;
    }
  }
}
