#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncAndroid : public CVideoSync, IDispResource
{
public:
  CVideoSyncAndroid(void *clock) : CVideoSync(clock), m_LastVBlankTime(0) {}

  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func) override;
  virtual void Run(CEvent& stop) override;
  virtual void Cleanup() override;
  virtual float GetFps() override;

  // IDispResource interface
  virtual void OnResetDisplay() override;

  // Choreographer callback
  void FrameCallback(int64_t frameTimeNanos);

private:
  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  CEvent m_abortEvent;
};
