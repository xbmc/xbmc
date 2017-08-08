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

#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "games/dialogs/osd/IVideoSelectCallback.h"

namespace KODI
{
namespace RETRO
{
  class CRPRenderManager : public CRenderManager,
                           public GAME::IVideoSelectCallback
  {
  public:
    CRPRenderManager(CDVDClock &clock, IRenderMsg *player);
    ~CRPRenderManager() override = default;

    // Implementation of IVideoSelectCallback
    bool SupportsScalingMethod(ESCALINGMETHOD method) override;
    bool SupportsRenderFeature(ERENDERFEATURE feature) override;
    void SetRenderViewMode(ViewMode mode) override;
    void SetScalingMethod(ESCALINGMETHOD method) override;
  };
}
}
