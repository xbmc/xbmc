/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/IEvent.h"
#include "threads/CriticalSection.h"

#include <map>
#include <string>
#include <vector>

#define NOTIFICATION_DISPLAY_TIME 5000
#define NOTIFICATION_MESSAGE_TIME 1000

typedef std::vector<EventPtr> Events;

class CEventLog
{
public:
  CEventLog() = default;
  CEventLog(const CEventLog&) = delete;
  CEventLog& operator=(CEventLog const&) = delete;
  ~CEventLog() = default;

  Events Get() const;
  Events Get(EventLevel level, bool includeHigherLevels = false) const;
  EventPtr Get(const std::string& eventIdentifier) const;

  void Add(const EventPtr& event);
  void Add(const EventPtr& event, bool withNotification, bool withSound = true);
  void AddWithNotification(const EventPtr& event,
                           unsigned int displayTime = NOTIFICATION_DISPLAY_TIME,
                           unsigned int messageTime = NOTIFICATION_MESSAGE_TIME,
                           bool withSound = true);
  void AddWithNotification(const EventPtr& event, bool withSound);
  void Remove(const EventPtr& event);
  void Remove(const std::string& eventIdentifier);
  void Clear();
  void Clear(EventLevel level, bool includeHigherLevels = false);

  bool Execute(const std::string& eventIdentifier);

  static std::string EventLevelToString(EventLevel level);
  static EventLevel EventLevelFromString(const std::string& level);

  void ShowFullEventLog(EventLevel level = EventLevel::Basic, bool includeHigherLevels = true);

private:
  void SendMessage(const EventPtr& event, int message);

  typedef std::vector<EventPtr> EventsList;
  typedef std::map<std::string, EventPtr> EventsMap;
  EventsList m_events;
  EventsMap m_eventsMap;
  mutable CCriticalSection m_critical;
};
