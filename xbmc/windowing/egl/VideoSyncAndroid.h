#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_ANDROID)
#include "VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncAndroid : public CVideoSync, IDispResource
{
public:
  CVideoSyncAndroid(CVideoReferenceClock *clock) : CVideoSync(clock), m_LastVBlankTime(0), m_abort(false){}
  
  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func);
  virtual void Run(std::atomic<bool>& stop);
  virtual void Cleanup();
  virtual float GetFps();
  
  // IDispResource interface
  virtual void OnResetDisplay();

  // Choreographer callback
  void FrameCallback(int64_t frameTimeNanos);
  
private:
  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  volatile bool m_abort;
};

#endif// TARGET_ANDROID
