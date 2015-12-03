/*
 *      Copyright (C) 2015 Team Kodi
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

#include "EventLog.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/EventsDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"

#include <utility>

std::map<int, std::unique_ptr<CEventLog> > CEventLog::s_eventLogs;
CCriticalSection CEventLog::s_critical;

std::string CEventLog::EventLevelToString(EventLevel level)
{
  switch (level)
  {
  case EventLevelBasic:
    return "basic";

  case EventLevelWarning:
    return "warning";

  case EventLevelError:
    return "error";

  case EventLevelInformation:
  default:
    break;
  }

  return "information";
}

EventLevel CEventLog::EventLevelFromString(const std::string& level)
{
  if (level == "basic")
    return EventLevelBasic;
  if (level == "warning")
    return EventLevelWarning;
  if (level == "error")
    return EventLevelError;

  return EventLevelInformation;
}

CEventLog& CEventLog::GetInstance()
{
  int currentProfileId = CProfilesManager::GetInstance().GetCurrentProfileId();

  CSingleLock lock(s_critical);
  auto eventLog = s_eventLogs.find(currentProfileId);
  if (eventLog == s_eventLogs.end())
  {
    s_eventLogs.insert(std::make_pair(currentProfileId, std::unique_ptr<CEventLog>(new CEventLog())));
    eventLog = s_eventLogs.find(currentProfileId);
  }

  return *eventLog->second;
}

Events CEventLog::Get() const
{
  return m_events;
}

Events CEventLog::Get(EventLevel level, bool includeHigherLevels /* = false */) const
{
  Events events;

  CSingleLock lock(m_critical);
  for (const auto& eventPtr : m_events)
  {
    if (eventPtr->GetLevel() == level ||
       (includeHigherLevels && eventPtr->GetLevel() > level))
      events.push_back(eventPtr);
  }

  return events;
}

EventPtr CEventLog::Get(const std::string& eventPtrIdentifier) const
{
  if (eventPtrIdentifier.empty())
    return EventPtr();

  CSingleLock lock(m_critical);
  const auto& eventPtr = m_eventsMap.find(eventPtrIdentifier);
  if (eventPtr == m_eventsMap.end())
    return EventPtr();

  return eventPtr->second;
}

void CEventLog::Add(const EventPtr& eventPtr)
{
  if (eventPtr == nullptr || eventPtr->GetIdentifier().empty() ||
      !CSettings::GetInstance().GetBool(CSettings::SETTING_EVENTLOG_ENABLED) ||
     (eventPtr->GetLevel() == EventLevelInformation && !CSettings::GetInstance().GetBool(CSettings::SETTING_EVENTLOG_ENABLED_NOTIFICATIONS)))
    return;

  CSingleLock lock(m_critical);
  if (m_eventsMap.find(eventPtr->GetIdentifier()) != m_eventsMap.end())
    return;

  // store the event
  m_events.push_back(eventPtr);
  m_eventsMap.insert(std::make_pair(eventPtr->GetIdentifier(), eventPtr));

  SendMessage(eventPtr, GUI_MSG_EVENT_ADDED);
}

void CEventLog::Add(const EventPtr& eventPtr, bool withNotification, bool withSound /* = true */)
{
  if (!withNotification)
    Add(eventPtr);
  else
    AddWithNotification(eventPtr, withSound);
}

void CEventLog::AddWithNotification(const EventPtr& eventPtr,
                                    unsigned int displayTime /* = NOTIFICATION_DISPLAY_TIME */,
                                    unsigned int messageTime /* = NOTIFICATION_MESSAGE_TIME */,
                                    bool withSound /* = true */)
{
  if (eventPtr == nullptr)
    return;

  Add(eventPtr);

  // queue the eventPtr as a kai toast notification
  if (!eventPtr->GetIcon().empty())
    CGUIDialogKaiToast::QueueNotification(eventPtr->GetIcon(), eventPtr->GetLabel(), eventPtr->GetDescription(), displayTime, withSound, messageTime);
  else
  {
    CGUIDialogKaiToast::eMessageType type = CGUIDialogKaiToast::Info;
    if (eventPtr->GetLevel() == EventLevelWarning)
      type = CGUIDialogKaiToast::Warning;
    else if (eventPtr->GetLevel() == EventLevelError)
      type = CGUIDialogKaiToast::Error;

    CGUIDialogKaiToast::QueueNotification(type, eventPtr->GetLabel(), eventPtr->GetDescription(), displayTime, withSound, messageTime);
  }
}

void CEventLog::AddWithNotification(const EventPtr& eventPtr, bool withSound)
{
  AddWithNotification(eventPtr, NOTIFICATION_DISPLAY_TIME, NOTIFICATION_MESSAGE_TIME, withSound);
}

void CEventLog::Remove(const EventPtr& eventPtr)
{
  if (eventPtr == nullptr)
    return;

  Remove(eventPtr->GetIdentifier());
}

void CEventLog::Remove(const std::string& eventPtrIdentifier)
{
  if (eventPtrIdentifier.empty())
    return;

  CSingleLock lock(m_critical);
  const auto& itEvent = m_eventsMap.find(eventPtrIdentifier);
  if (itEvent == m_eventsMap.end())
    return;

  EventPtr eventPtr = itEvent->second;
  m_eventsMap.erase(itEvent);
  m_events.erase(std::remove(m_events.begin(), m_events.end(), eventPtr), m_events.end());

  SendMessage(eventPtr, GUI_MSG_EVENT_REMOVED);
}

void CEventLog::Clear()
{
  CSingleLock lock(m_critical);
  m_events.clear();
  m_eventsMap.clear();
}

void CEventLog::Clear(EventLevel level, bool includeHigherLevels /* = false */)
{
  EventsList eventsCopy = m_events;
  for (const auto& eventPtr : eventsCopy)
  {

    if (eventPtr->GetLevel() == level ||
      (includeHigherLevels && eventPtr->GetLevel() > level))
      Remove(eventPtr);
  }
}

bool CEventLog::Execute(const std::string& eventPtrIdentifier)
{
  if (eventPtrIdentifier.empty())
    return false;

  CSingleLock lock(m_critical);
  const auto& itEvent = m_eventsMap.find(eventPtrIdentifier);
  if (itEvent == m_eventsMap.end())
    return false;

  return itEvent->second->Execute();
}

void CEventLog::ShowFullEventLog(EventLevel level /* = EventLevelBasic */, bool includeHigherLevels /* = true */)
{
  // put together the path
  std::string path = "events://";
  if (level != EventLevelBasic || !includeHigherLevels)
  {
    // add the level to the path
    path += EventLevelToString(level);
    // add whether to include higher levels or not to the path
    if (includeHigherLevels)
      path += "+";
  }

  // activate the full eventPtr log window
  std::vector<std::string> params;
  params.push_back(path);
  params.push_back("return");
  g_windowManager.ActivateWindow(WINDOW_EVENT_LOG, params);
}

void CEventLog::OnSettingAction(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_EVENTLOG_SHOW)
    ShowFullEventLog();
}

void CEventLog::SendMessage(const EventPtr& eventPtr, int message)
{
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, message, 0, XFILE::CEventsDirectory::EventToFileItem(eventPtr));
  g_windowManager.SendThreadMessage(msg);
}
