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

#include "cores/IPlayer.h"

namespace KODI
{
namespace RETRO
{
  class CGUIRenderSettings
  {
  public:
    void EnableGuiRenderSettings(bool bEnabled) { m_guiRender = bEnabled; }
    bool IsGuiRenderSettingsEnabled() const { return m_guiRender; }

    ViewMode GetRenderViewMode() { return m_viewMode; }
    void SetRenderViewMode(ViewMode mode) { m_viewMode = mode; }

    ESCALINGMETHOD GetScalingMethod() { return m_scalingMethod; }
    void SetScalingMethod(ESCALINGMETHOD method) { m_scalingMethod = method; }

  private:
    bool m_guiRender = false;
    ViewMode m_viewMode = ViewModeNormal;
    ESCALINGMETHOD m_scalingMethod = VS_SCALINGMETHOD_NEAREST;
  };
}
}
