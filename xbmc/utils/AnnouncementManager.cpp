/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AnnouncementManager.h"
#include "SingleLock.h"
#include <stdio.h>
#include "log.h"

using namespace std;
using namespace ANNOUNCEMENT;

CCriticalSection CAnnouncementManager::m_critSection;
vector<IAnnouncer *> CAnnouncementManager::m_announcers;

void CAnnouncementManager::AddAnnouncer(IAnnouncer *listener)
{
  CSingleLock lock (m_critSection);
  m_announcers.push_back(listener);
}

void CAnnouncementManager::RemoveAnnouncer(IAnnouncer *listener)
{
  CSingleLock lock (m_critSection);
  for (unsigned int i = 0; i < m_announcers.size(); i++)
  {
    if (m_announcers[i] == listener)
    {
      m_announcers.erase(m_announcers.begin() + i);
      return;
    }
  }
}

void CAnnouncementManager::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const char *data)
{
  CLog::Log(LOGDEBUG, "CAnnouncementManager - Announcement: %s from %s", message, sender);
  CSingleLock lock (m_critSection);
  for (unsigned int i = 0; i < m_announcers.size(); i++)
    m_announcers[i]->Announce(flag, sender, message, data);
}
