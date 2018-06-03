/*
 *      Copyright (C) 2018-present Team Kodi
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
