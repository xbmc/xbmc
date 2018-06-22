/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <string>

namespace KODI
{
namespace GAME
{
  class CGameClient;
  class CGameSettings;
}

namespace RETRO
{
  class IAutoSaveCallback
  {
  public:
    virtual ~IAutoSaveCallback() = default;

    virtual bool IsAutoSaveEnabled() const = 0;
    virtual std::string CreateSavestate() = 0;
  };

  class CRetroPlayerAutoSave : protected CThread
  {
  public:
    explicit CRetroPlayerAutoSave(IAutoSaveCallback &callback,
                                  GAME::CGameSettings &settings);

    ~CRetroPlayerAutoSave() override;

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    // Construction parameters
    IAutoSaveCallback &m_callback;
    GAME::CGameSettings &m_settings;
  };
}
}
