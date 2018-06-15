/*
 *  Copyright (C) 2016-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IGameClientPlayback.h"

namespace KODI
{
namespace GAME
{
  class CGameClientRealtimePlayback : public IGameClientPlayback
  {
  public:
    virtual ~CGameClientRealtimePlayback() = default;

    // implementation of IGameClientPlayback
    virtual bool CanPause() const override { return false; }
    virtual bool CanSeek() const override { return false; }
    virtual unsigned int GetTimeMs() const override { return 0; }
    virtual unsigned int GetTotalTimeMs() const override { return 0; }
    virtual unsigned int GetCacheTimeMs() const override { return 0; }
    virtual void SeekTimeMs(unsigned int timeMs) override { }
    virtual double GetSpeed() const override { return 1.0; }
    virtual void SetSpeed(double speedFactor) override { }
    virtual void PauseAsync() override { }
    virtual std::string CreateSavestate() override { return ""; }
    virtual bool LoadSavestate(const std::string& path) override { return false; }
  };
}
}
