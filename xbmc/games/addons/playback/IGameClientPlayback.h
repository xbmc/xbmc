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
    virtual void PauseUnpause() = 0;
    virtual unsigned int GetTimeMs() const = 0;
    virtual unsigned int GetTotalTimeMs() const = 0;
    virtual unsigned int GetCacheTimeMs() const = 0;
    virtual void SeekTimeMs(unsigned int timeMs) = 0;
    virtual double GetSpeed() const = 0;
    virtual void SetSpeed(double speedFactor) = 0;

    // Savestates
    virtual std::string CreateSavestate() = 0; // Returns the path of savestate on success
    virtual bool LoadSavestate(const std::string& path) = 0;
  };
}
}
