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
#include "jobs/JobManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <mutex>

namespace
{
constexpr unsigned int HIDDEN_SIZE = 8;

void NormalizeRequest(unsigned int& width,
                      unsigned int& height,
                      CAspectRatio::AspectRatio& aspectRatio)
{
  if (aspectRatio == CAspectRatio::CENTER)
    return;

  // decodes keep image's shape so any can stand in for another, and cover never upscales
  if (aspectRatio == CAspectRatio::STRETCH)
    aspectRatio = CAspectRatio::SCALE;

  if (std::max(width, height) < HIDDEN_SIZE)
  {
    aspectRatio = CAspectRatio::KEEP;
    width = height = 0;
  }
}

// a loaded decode of the same image can stand in for a request it covers; bilinear filtering
// shrinks up to 2x without aliasing
bool Covers(const CTextureArray& texture,
            unsigned int width,
            unsigned int height,
            CAspectRatio::AspectRatio aspectRatio)
{
  if (!texture.size() || !width || !height || texture.m_width <= 0 || texture.m_height <= 0)
    return false;

  const float aspect = static_cast<float>(texture.m_width) / texture.m_height;
  float needWidth = aspectRatio == CAspectRatio::SCALE
                        ? std::max(static_cast<float>(width), height * aspect)
                        : std::min(static_cast<float>(width), height * aspect);
  needWidth = std::min(needWidth, static_cast<float>(texture.m_textures[0]->GetOriginalWidth()));
  return texture.m_width + 1 >= needWidth && texture.m_width <= 2 * needWidth;
}
} // namespace

CImageLoader::CImageLoader(const std::string& path,
                           unsigned int targetWidth,
                           unsigned int targetHeight,
                           CAspectRatio::AspectRatio aspectRatio,
                           const bool useCache)
  : m_path(path),
    m_texture(nullptr),
    m_targetWidth(targetWidth),
    m_targetHeight(targetHeight),
    m_aspectRatio(aspectRatio)
{
  m_use_cache = useCache;

  if (m_aspectRatio != CAspectRatio::CENTER && m_targetWidth == 0 && m_targetHeight == 0)
  {
    const CGraphicContext& gfxContext = CServiceBroker::GetWinSystem()->GetGfxContext();
    m_targetWidth = static_cast<unsigned int>(gfxContext.GetWidth());
    m_targetHeight = static_cast<unsigned int>(gfxContext.GetHeight());
  }
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
    m_texture = CTexture::LoadFromFile(loadPath, m_targetWidth, m_targetHeight, m_aspectRatio);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (duration.count() > 100)
      CLog::Log(LOGDEBUG, "{} - took {} ms to load {}", __FUNCTION__, duration.count(), loadPath);

    if (m_texture)
    {
      if (needsChecking)
        CServiceBroker::GetTextureCache()->BackgroundCacheImage(texturePath);

      if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAsyncTextureUpload)
        m_texture->LoadToGPUAsync();

      return true;
    }

    // Fallthrough on failure:
    CLog::Log(LOGERROR, "{} - Direct texture file loading failed for {}", __FUNCTION__, loadPath);
  }

  if (!m_use_cache)
    return false; // We're done

  // not in our texture cache or it failed to load from it, so try and load directly and then cache the result
  CServiceBroker::GetTextureCache()->CacheImage(texturePath, &m_texture, nullptr, m_targetWidth,
                                                m_targetHeight, m_aspectRatio);

  if (!m_texture)
    return false;

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAsyncTextureUpload)
    m_texture->LoadToGPUAsync();

  return true;
}

CGUILargeTextureManager::CLargeTexture::CLargeTexture(const std::string& path,
                                                      unsigned int targetWidth,
                                                      unsigned int targetHeight,
                                                      CAspectRatio::AspectRatio aspectRatio)
  : m_path(path),
    m_targetWidth(targetWidth),
    m_targetHeight(targetHeight),
    m_aspectRatio(aspectRatio)
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

bool CGUILargeTextureManager::CLargeTexture::DeleteIfRequired(bool deleteImmediately)
{
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
  std::unique_lock lock(m_listSection);
  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    if (image->DeleteIfRequired(immediately))
      it = m_allocated.erase(it);
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
bool CGUILargeTextureManager::GetImage(const std::string& path,
                                       CTextureArray& texture,
                                       unsigned int width,
                                       unsigned int height,
                                       CAspectRatio::AspectRatio aspectRatio,
                                       bool firstRequest,
                                       const bool useCache)
{
  NormalizeRequest(width, height, aspectRatio);

  std::unique_lock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path && image->GetTargetWidth() == width &&
        image->GetTargetHeight() == height && image->GetAspectRatio() == aspectRatio)
    {
      if (firstRequest)
        image->AddRef();
      texture = image->GetTexture();
      return texture.size() > 0;
    }
  }

  if (!firstRequest)
    return true;

  const bool queued = std::any_of(m_queued.begin(), m_queued.end(),
                                  [&](const auto& job)
                                  {
                                    const CLargeTexture* image = job.second;
                                    return image->GetPath() == path &&
                                           image->GetTargetWidth() == width &&
                                           image->GetTargetHeight() == height &&
                                           image->GetAspectRatio() == aspectRatio;
                                  });
  if (!queued && aspectRatio != CAspectRatio::CENTER)
  {
    const CLargeTexture* best = nullptr;
    for (const CLargeTexture* image : m_allocated)
    {
      if (image->GetPath() == path && image->GetAspectRatio() != CAspectRatio::CENTER &&
          Covers(image->GetTexture(), width, height, aspectRatio) &&
          (!best || image->GetTexture().m_width < best->GetTexture().m_width))
        best = image;
    }
    if (best)
    {
      auto* image = new CLargeTexture(path, width, height, aspectRatio);
      image->ShareTexture(best->GetTexture());
      m_allocated.push_back(image);
      texture = image->GetTexture();
      return true;
    }
  }

  QueueImage(path, width, height, aspectRatio, useCache);
  return true;
}

bool CGUILargeTextureManager::GetInterimImage(const std::string& path,
                                              CAspectRatio::AspectRatio aspectRatio,
                                              CTextureArray& texture)
{
  if (aspectRatio == CAspectRatio::CENTER)
    return false;

  std::unique_lock lock(m_listSection);
  const CLargeTexture* best = nullptr;
  for (const CLargeTexture* image : m_allocated)
  {
    if (image->GetPath() == path && image->GetAspectRatio() != CAspectRatio::CENTER &&
        image->GetTexture().size() &&
        (!best || image->GetTexture().m_width > best->GetTexture().m_width))
      best = image;
  }
  if (!best)
    return false;

  texture = best->GetTexture();
  return true;
}

void CGUILargeTextureManager::ReleaseImage(const std::string& path,
                                           unsigned int width,
                                           unsigned int height,
                                           CAspectRatio::AspectRatio aspectRatio,
                                           bool immediately)
{
  NormalizeRequest(width, height, aspectRatio);

  std::unique_lock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path && image->GetTargetWidth() == width &&
        image->GetTargetHeight() == height && image->GetAspectRatio() == aspectRatio)
    {
      if (image->DecrRef(immediately) && immediately)
        m_allocated.erase(it);
      return;
    }
  }
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    unsigned int id = it->first;
    CLargeTexture *image = it->second;
    if (image->GetPath() == path && image->GetTargetWidth() == width &&
        image->GetTargetHeight() == height && image->GetAspectRatio() == aspectRatio &&
        image->DecrRef(true))
    {
      // cancel this job
      CServiceBroker::GetJobManager()->CancelJob(id);
      m_queued.erase(it);
      return;
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(const std::string& path,
                                         unsigned int width,
                                         unsigned int height,
                                         CAspectRatio::AspectRatio aspectRatio,
                                         bool useCache)
{
  if (path.empty())
    return;

  std::unique_lock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = it->second;
    if (image->GetPath() == path && image->GetTargetWidth() == width &&
        image->GetTargetHeight() == height && image->GetAspectRatio() == aspectRatio)
    {
      image->AddRef();
      return; // already queued
    }
  }

  // queue the item
  CLargeTexture* image = new CLargeTexture(path, width, height, aspectRatio);
  unsigned int jobID = CServiceBroker::GetJobManager()->AddJob(
      new CImageLoader(path, width, height, aspectRatio, useCache), this, CJob::PRIORITY_NORMAL);
  m_queued.emplace_back(jobID, image);
}

void CGUILargeTextureManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // see if we still have this job id
  std::unique_lock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    if (it->first == jobID)
    { // found our job
      CImageLoader *loader = static_cast<CImageLoader*>(job);
      CLargeTexture *image = it->second;
      image->SetTexture(std::move(loader->m_texture));
      loader->m_texture = NULL; // we want to keep the texture, and jobs are auto-deleted.
      m_queued.erase(it);
      m_allocated.push_back(image);
      return;
    }
  }
}
