/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderGeometry.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "cores/IPlayer.h"
#include "threads/CriticalSection.h"

namespace KODI
{
namespace RETRO
{
  class CGUIGameControl;

  class CGUIRenderSettings : public IGUIRenderSettings
  {
  public:
    CGUIRenderSettings(CGUIGameControl &guiControl);
    ~CGUIRenderSettings() override = default;

    // implementation of IGUIRenderSettings
    bool HasScalingMethod() const override;
    bool HasViewMode() const override;
    CRenderSettings GetSettings() const override;

    // Render functions
    void Reset();
    void SetSettings(CRenderSettings settings);
    void SetGeometry(CRenderGeometry geometry);
    void SetScalingMethod(ESCALINGMETHOD scalingMethod);
    void SetViewMode(ViewMode viewMode);

  private:
    // Construction parameters
    CGUIGameControl &m_guiControl;

    // Render parameters
    CRenderSettings m_renderSettings;

    // Synchronization parameters
    CCriticalSection m_mutex;
  };
}
}
