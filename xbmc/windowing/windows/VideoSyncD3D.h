/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#pragma once

#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"
#include "threads/Event.h"

class CVideoSyncD3D : public CVideoSync, IDispResource
{
public:
  CVideoSyncD3D(void *clock) : CVideoSync(clock), m_displayLost(false), m_displayReset(false), m_lastUpdateTime(0) { };
  bool Setup(PUPDATECLOCK func) override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void RefreshChanged() override;
  // IDispResource overrides
  void OnLostDisplay() override;
  void OnResetDisplay() override;

private:
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
  int64_t m_lastUpdateTime;
};

