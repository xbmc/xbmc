/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventLog.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/EventsDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <mutex>
#include <utility>

std::string CEventLog::EventLevelToString(EventLevel level)
{
  switch (level)
  {
  case EventLevel::Basic:
    return "basic";

  case EventLevel::Warning:
    return "warning";

  case EventLevel::Error:
    return "error";

  case EventLevel::Information:
  default:
    break;
  }

  return "information";
}

EventLevel CEventLog::EventLevelFromString(const std::string& level)
{
  if (level == "basic")
    return EventLevel::Basic;
  if (level == "warning")
    return EventLevel::Warning;
  if (level == "error")
    return EventLevel::Error;

  return EventLevel::Information;
}

Events CEventLog::Get() const
{
  return m_events;
}

Events CEventLog::Get(EventLevel level, bool includeHigherLevels /* = false */) const
{
  Events events;

  std::unique_lock<CCriticalSection> lock(m_critical);
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

  std::unique_lock<CCriticalSection> lock(m_critical);
  const auto& eventPtr = m_eventsMap.find(eventPtrIdentifier);
  if (eventPtr == m_eventsMap.end())
    return EventPtr();

  return eventPtr->second;
}

void CEventLog::Add(const EventPtr& eventPtr)
{
  if (eventPtr == nullptr || eventPtr->GetIdentifier().empty() ||
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EVENTLOG_ENABLED) ||
     (eventPtr->GetLevel() == EventLevel::Information && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EVENTLOG_ENABLED_NOTIFICATIONS)))
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);
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
    if (eventPtr->GetLevel() == EventLevel::Warning)
      type = CGUIDialogKaiToast::Warning;
    else if (eventPtr->GetLevel() == EventLevel::Error)
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

  std::unique_lock<CCriticalSection> lock(m_critical);
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
  std::unique_lock<CCriticalSection> lock(m_critical);
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

  std::unique_lock<CCriticalSection> lock(m_critical);
  const auto& itEvent = m_eventsMap.find(eventPtrIdentifier);
  if (itEvent == m_eventsMap.end())
    return false;

  return itEvent->second->Execute();
}

void CEventLog::ShowFullEventLog(EventLevel level /* = EventLevel::Basic */, bool includeHigherLevels /* = true */)
{
  // put together the path
  std::string path = "events://";
  if (level != EventLevel::Basic || !includeHigherLevels)
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
  params.emplace_back("return");
  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_EVENT_LOG, params);
}

void CEventLog::SendMessage(const EventPtr& eventPtr, int message)
{
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, message, 0, XFILE::CEventsDirectory::EventToFileItem(eventPtr));
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}
