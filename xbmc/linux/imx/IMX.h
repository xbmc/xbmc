#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "guilib/DispResource.h"

class CIMX : public CThread, IDispResource
{
public:
  CIMX(void);
  ~CIMX(void);

  bool          Initialize();
  void          Deinitialize();

  int           WaitVsync();
  virtual void  OnResetDisplay();

private:
  virtual void  Process();
  bool          UpdateDCIC();

  int           m_fddcic;
  bool          m_change;
  unsigned long m_counter;
  unsigned long m_counterLast;
  CEvent        m_VblankEvent;

  double        m_frameTime;
  CCriticalSection m_critSection;

  uint32_t      m_lastSyncFlag;
};

extern CIMX g_IMX;
