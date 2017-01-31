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

#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncDRM : public CVideoSync, IDispResource
{
public:
  CVideoSyncDRM(void *clock) : CVideoSync(clock) {};
  virtual bool Setup(PUPDATECLOCK func);
  virtual void Run(std::atomic<bool>& stop);
  virtual void Cleanup();
  virtual float GetFps();
  virtual void OnResetDisplay();
  virtual void RefreshChanged();
private:
  static void EventHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
  int m_fd;
  volatile bool m_abort;
  struct VblInfo
  {
    uint64_t start;
    CVideoSyncDRM *videoSync;
  };
};

