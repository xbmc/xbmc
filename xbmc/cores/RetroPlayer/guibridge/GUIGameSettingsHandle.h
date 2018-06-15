/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO
{
  class CGUIGameRenderManager;

  class CGUIGameSettingsHandle
  {
  public:
    CGUIGameSettingsHandle(CGUIGameRenderManager &renderManager);
    virtual ~CGUIGameSettingsHandle();

    std::string GameClientID();

  private:
    // Construction parameters
    CGUIGameRenderManager &m_renderManager;
  };
}
}
