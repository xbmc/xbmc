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

#pragma once

#include "cores/RetroPlayer/RetroPlayerTypes.h"
#include "cores/GameSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class IRendererFactory;
  class IRenderBufferPools;

  class CRenderBufferManager
  {
  public:
    CRenderBufferManager() = default;
    ~CRenderBufferManager();

    void RegisterPools(IRendererFactory *factory, RenderBufferPoolVector pools);
    RenderBufferPoolVector GetPools(IRendererFactory *factory);
    std::vector<IRenderBufferPool*> GetBufferPools();
    void FlushPools();

    std::string GetRenderSystemName(IRenderBufferPool *renderBufferPool) const;

    bool HasScalingMethod(SCALINGMETHOD scalingMethod) const;

  protected:
    struct RenderBufferPools
    {
      IRendererFactory* factory;
      RenderBufferPoolVector pools;
    };

    std::vector<RenderBufferPools> m_pools;
    CCriticalSection m_critSection;
  };
}
}
