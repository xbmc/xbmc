/*
 *      Copyright (C) 2016-2017 Team Kodi
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
