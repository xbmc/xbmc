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

#include "IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <memory>

class CGameSettings;

namespace KODI
{
namespace RETRO
{
  class CRPProcessInfo;

  class CGUIGameSettings : public IGUIRenderSettings,
                           public Observer
  {
  public:
    CGUIGameSettings(CRPProcessInfo &processInfo);
    ~CGUIGameSettings();

    // implementation of IGUIRenderSettings
    CRenderSettings GetSettings() const override;

    // implementation of Observer
    void Notify(const Observable &obs, const ObservableMessage msg) override;

  private:
    void UpdateSettings();

    // Construction parameters
    CRPProcessInfo &m_processInfo;

    // GUI parameters
    CGameSettings &m_guiSettings;

    // Render parameters
    CRenderSettings m_renderSettings;

    // Synchronization parameters
    CCriticalSection m_mutex;
  };
}
}
