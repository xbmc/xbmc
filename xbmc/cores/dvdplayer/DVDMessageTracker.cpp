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

#include "threads/SystemClock.h"
#include "DVDMessageTracker.h"
#include "DVDMessage.h"
#include "threads/SingleLock.h"

#ifdef DVDDEBUG_MESSAGE_TRACKER



CDVDMessageTracker g_dvdMessageTracker;

CDVDMessageTracker::CDVDMessageTracker() : CThread()
{
  m_bInitialized = false;
  InitializeCriticalSection(&m_critSection);
}

CDVDMessageTracker::~CDVDMessageTracker()
{
  DeInit();
  DeleteCriticalSection(&m_critSection);
}

void CDVDMessageTracker::Init()
{
  DeInit();

  Create();
}

void CDVDMessageTracker::DeInit()
{
  StopThread();
}

void CDVDMessageTracker::Register(CDVDMsg* pMsg)
{
  if (m_bInitialized)
  {
    CSingleLock lock(m_critSection);
    m_messageList.push_back(new CDVDMessageTrackerItem(pMsg));
  }
}

void CDVDMessageTracker::UnRegister(CDVDMsg* pMsg)
{
  CSingleLock lock(m_critSection);

  std::list<CDVDMessageTrackerItem*>::iterator iter = m_messageList.begin();
  while (iter != m_messageList.end())
  {
    CDVDMessageTrackerItem* pItem = *iter;
    if (pItem->m_pMsg == pMsg)
    {
      m_messageList.remove(pItem);
      delete pItem;
      break;
    }
    iter++;
  }

}

void CDVDMessageTracker::OnStartup()
{
  m_bInitialized = true;
}

void CDVDMessageTracker::Process()
{
  while (!m_bStop)
  {
    Sleep(1000);

    CSingleLock lock(m_critSection);

    std::list<CDVDMessageTrackerItem*>::iterator iter = m_messageList.begin();
    while (!m_bStop && iter != m_messageList.end())
    {
      CDVDMessageTrackerItem* pItem = *iter;
      if ((XbmcThreads::SystemClockMillis() - pItem->m_time_created) > 60000)
      {
        if (!pItem->m_debug_logged)
        {
          CLog::Log(LOGWARNING, "CDVDMsgTracker::Process() - Msg still in system after 60 seconds.");
          CLog::Log(LOGWARNING, "                            Type : %d", pItem->m_pMsg->GetMessageType());

          pItem->m_debug_logged = true;
        }
      }
      iter++;
    }

  }
}

void CDVDMessageTracker::OnExit()
{
  m_bInitialized = false;

  CLog::Log(LOGWARNING, "CDVDMsgTracker::Process() - nr of messages in system : %d", m_messageList.size());

  m_messageList.clear();
}

#endif // DVDDEBUG_MESSAGE_TRACKER

