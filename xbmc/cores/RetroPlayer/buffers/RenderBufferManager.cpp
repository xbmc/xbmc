/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferManager.h"
#include "IRenderBufferPool.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "threads/SingleLock.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

CRenderBufferManager::~CRenderBufferManager()
{
  FlushPools();
}

void CRenderBufferManager::RegisterPools(IRendererFactory *factory, RenderBufferPoolVector pools)
{
  CSingleLock lock(m_critSection);

  m_pools.emplace_back(RenderBufferPools{factory, std::move(pools)});
}

RenderBufferPoolVector CRenderBufferManager::GetPools(IRendererFactory *factory)
{
  RenderBufferPoolVector bufferPools;

  CSingleLock lock(m_critSection);

  auto it = std::find_if(m_pools.begin(), m_pools.end(),
    [factory](const RenderBufferPools &pools)
    {
      return pools.factory == factory;
    });

  if (it != m_pools.end())
    bufferPools = it->pools;

  return bufferPools;
}

std::vector<IRenderBufferPool*> CRenderBufferManager::GetBufferPools()
{
  std::vector<IRenderBufferPool*> bufferPools;

  CSingleLock lock(m_critSection);

  for (const auto &pools : m_pools)
  {
    for (const auto &pool : pools.pools)
      bufferPools.emplace_back(pool.get());
  }

  return bufferPools;
}

void CRenderBufferManager::FlushPools()
{
  CSingleLock lock(m_critSection);

  for (const auto &pools : m_pools)
  {
    for (const auto &pool : pools.pools)
      pool->Flush();
  }
}

std::string CRenderBufferManager::GetRenderSystemName(IRenderBufferPool *renderBufferPool) const
{
  CSingleLock lock(m_critSection);

  for (const auto &pools : m_pools)
  {
    for (const auto &pool : pools.pools)
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

  for (const auto &pools : m_pools)
  {
    for (const auto &pool : pools.pools)
      if (pool->IsCompatible(videoSettings))
        return true;
  }

  return false;
}
