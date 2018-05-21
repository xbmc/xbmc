#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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
#include "threads/Event.h"

class CVideoSyncOsx : public CVideoSync, IDispResource
{
public:

  CVideoSyncOsx(void *clock) :
    CVideoSync(clock),
    m_LastVBlankTime(0),
    m_displayLost(false),
    m_displayReset(false){};
  
  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func) override;
  virtual void Run(CEvent& stopEvent) override;
  virtual void Cleanup() override;
  virtual float GetFps() override;
  virtual void RefreshChanged() override;
  
  // IDispResource interface
  virtual void OnLostDisplay() override;
  virtual void OnResetDisplay() override;

  // used in the displaylink callback
  void VblankHandler(int64_t nowtime, uint32_t timebase);
  
private:
  virtual bool InitDisplayLink();
  virtual void DeinitDisplayLink();

  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
};

