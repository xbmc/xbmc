/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EventLogManager.h"
#include "EventLog.h"
#include "threads/SingleLock.h"

#include <utility>

CEventLog& CEventLogManager::GetEventLog(unsigned int profileId)
{
  CSingleLock lock(m_eventMutex);

  auto eventLog = m_eventLogs.find(profileId);
  if (eventLog == m_eventLogs.end())
  {
    m_eventLogs.insert(std::make_pair(profileId, std::unique_ptr<CEventLog>(new CEventLog)));
    eventLog = m_eventLogs.find(profileId);
  }

  return *eventLog->second;
}
