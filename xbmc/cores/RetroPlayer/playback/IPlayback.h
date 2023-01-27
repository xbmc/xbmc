/*
 *  Copyright (C) 2016-2018 Team Kodi
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
class IPlayback
{
public:
  virtual ~IPlayback() = default;

  // Lifetime management
  virtual void Initialize() = 0;
  virtual void Deinitialize() = 0;

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
  virtual std::string CreateSavestate(
      bool autosave,
      const std::string& savestatePath = "") = 0; // Returns the path of savestate on success
  virtual bool LoadSavestate(const std::string& savestatePath) = 0;
};
} // namespace RETRO
} // namespace KODI
