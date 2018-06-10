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

#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "cores/GameSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

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
    bool HasVideoFilter() const override;
    bool HasViewMode() const override;
    bool HasRotation() const override;
    CRenderSettings GetSettings() const override;
    CRect GetDimensions() const override;

    // Render functions
    void Reset();
    void SetSettings(CRenderSettings settings);
    void SetDimensions(const CRect &dimensions);
    void SetVideoFilter(const std::string &videoFilter);
    void SetViewMode(VIEWMODE viewMode);
    void SetRotationDegCCW(unsigned int rotationDegCCW);

  private:
    // Construction parameters
    CGUIGameControl &m_guiControl;

    // Render parameters
    CRenderSettings m_renderSettings;
    CRect m_renderDimensions;

    // Synchronization parameters
    CCriticalSection m_mutex;
  };
}
}
