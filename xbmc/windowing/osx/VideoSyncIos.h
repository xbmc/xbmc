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

#if defined(TARGET_DARWIN_IOS)
#include "VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncIos : public CVideoSync, IDispResource
{
public:
  CVideoSyncIos(void *clock) : CVideoSync(clock), m_LastVBlankTime(0), m_abort(false){}
  
  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func) override;
  virtual void Run(std::atomic<bool>& stop) override;
  virtual void Cleanup() override;
  virtual float GetFps() override;
  
  // IDispResource interface
  virtual void OnResetDisplay() override;

  // used in the displaylink callback
  void IosVblankHandler();
  
private:
  // CVideoSyncDarwin interface
  virtual bool InitDisplayLink();
  virtual void DeinitDisplayLink();

  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  volatile bool m_abort;
};

#endif// TARGET_DARWIN_IOS
