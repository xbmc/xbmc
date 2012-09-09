#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "dvd_config.h"

#ifdef DVDDEBUG_MESSAGE_TRACKER

#include "utils/thread.h"
#include "threads/CriticalSection.h"

class CDVDMsg;

class CDVDMessageTrackerItem
{
public:
  CDVDMessageTrackerItem(CDVDMsg* pMsg)
  {
    m_pMsg = pMsg;
    m_debug_logged = false;
    m_time_created = CTimeUtils::GetTimeMS();
  }

  CDVDMsg* m_pMsg;
  bool m_debug_logged;
  DWORD m_time_created;

};

class CDVDMessageTracker : private CThread
{
public:
  CDVDMessageTracker();
  ~CDVDMessageTracker();

  void Init();
  void DeInit();

  void Register(CDVDMsg* pMsg);
  void UnRegister(CDVDMsg* pMsg);

protected:
  void OnStartup();
  void Process();
  void OnExit();

private:
  bool m_bInitialized;
  std::list<CDVDMessageTrackerItem*> m_messageList;
  CCriticalSection m_critSection;
};

extern CDVDMessageTracker g_dvdMessageTracker;

#endif //DVDDEBUG_MESSAGE_TRACKER
