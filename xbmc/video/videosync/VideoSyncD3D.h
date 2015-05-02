#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#if defined(TARGET_WINDOWS)

#include "video/videosync/VideoSync.h"
#include <d3d9.h>
#include "guilib/D3DResource.h"
#include "threads/Event.h"

class CVideoSyncD3D : public CVideoSync, ID3DResource
{
public:
  virtual bool Setup(PUPDATECLOCK func);
  virtual void Run(volatile bool& stop);
  virtual void Cleanup();
  virtual float GetFps();
  virtual void RefreshChanged();

  virtual void OnCreateDevice() {}
  virtual void OnDestroyDevice();
  virtual void OnResetDevice();
private:
  LPDIRECT3DDEVICE9 m_D3dDev;
  int m_height;
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
  int64_t m_lastUpdateTime;
};

#endif
