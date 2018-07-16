/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    mutable CCriticalSection m_mutex;
  };
}
}
