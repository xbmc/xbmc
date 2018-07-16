/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

namespace KODI
{
namespace GAME
{
  class CGameClient;
  class CGameSettings;
}

namespace RETRO
{
  class CRetroPlayerAutoSave : protected CThread
  {
  public:
    explicit CRetroPlayerAutoSave(GAME::CGameClient &gameClient,
                                  GAME::CGameSettings &settings);

    ~CRetroPlayerAutoSave() override;

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    // Construction parameters
    GAME::CGameClient &m_gameClient;
    GAME::CGameSettings &m_settings;
  };
}
}
