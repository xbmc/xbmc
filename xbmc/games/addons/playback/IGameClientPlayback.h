/*
 *  Copyright (C) 2016-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace KODI
{
namespace GAME
{
  class IGameClientPlayback
  {
  public:
    virtual ~IGameClientPlayback() = default;

    // Playback capabilities
    virtual bool CanPause() const = 0;
    virtual bool CanSeek() const = 0;

    // Control playback
    virtual unsigned int GetTimeMs() const = 0;
    virtual unsigned int GetTotalTimeMs() const = 0;
    virtual unsigned int GetCacheTimeMs() const = 0;
    virtual void SeekTimeMs(unsigned int timeMs) = 0;
    virtual double GetSpeed() const = 0;
    virtual void SetSpeed(double speedFactor) = 0;
    virtual void PauseAsync() = 0; // Pauses after the following frame

    // Savestates
    virtual std::string CreateSavestate() = 0; // Returns the path of savestate on success
    virtual bool LoadSavestate(const std::string& path) = 0;
  };
}
}
