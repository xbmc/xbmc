/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferManager.h"

#include "IRenderBufferPool.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

#include <algorithm>
#include <mutex>

using namespace KODI;
using namespace RETRO;

CRenderBufferManager::~CRenderBufferManager()
{
  FlushPools();
}

void CRenderBufferManager::RegisterPools(IRendererFactory* factory, RenderBufferPoolVector pools)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_pools.emplace_back(RenderBufferPools{factory, std::move(pools)});
}

RenderBufferPoolVector CRenderBufferManager::GetPools(IRendererFactory* factory)
{
  RenderBufferPoolVector bufferPools;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  auto it =
      std::find_if(m_pools.begin(), m_pools.end(),
                   [factory](const RenderBufferPools& pools) { return pools.factory == factory; });

  if (it != m_pools.end())
    bufferPools = it->pools;

  return bufferPools;
}

std::vector<IRenderBufferPool*> CRenderBufferManager::GetBufferPools()
{
  std::vector<IRenderBufferPool*> bufferPools;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& pools : m_pools)
  {
    for (const auto& pool : pools.pools)
      bufferPools.emplace_back(pool.get());
  }

  return bufferPools;
}

void CRenderBufferManager::FlushPools()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& pools : m_pools)
  {
    for (const auto& pool : pools.pools)
      pool->Flush();
  }
}

std::string CRenderBufferManager::GetRenderSystemName(IRenderBufferPool* renderBufferPool) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& pools : m_pools)
  {
    for (const auto& pool : pools.pools)
    {
      if (pool.get() == renderBufferPool)
        return pools.factory->RenderSystemName();
    }
  }

  return "";
}

bool CRenderBufferManager::HasScalingMethod(SCALINGMETHOD scalingMethod) const
{
  CRenderVideoSettings videoSettings;
  videoSettings.SetScalingMethod(scalingMethod);

  for (const auto& pools : m_pools)
  {
    for (const auto& pool : pools.pools)
      if (pool->IsCompatible(videoSettings))
        return true;
  }

  return false;
}
