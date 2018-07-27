/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "EventsDirectory.h"
#include "URL.h"
#include "events/EventLog.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

using namespace XFILE;

bool CEventsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  items.ClearProperties();
  items.SetContent("events");

  CEventLog& log = CServiceBroker::GetEventLog();
  Events events;

  std::string hostname = url.GetHostName();
  if (hostname.empty())
    events = log.Get();
  else
  {
    bool includeHigherLevels = false;
    // check if we should only retrieve events from a specific level or
    // also from all higher levels
    if (StringUtils::EndsWith(hostname, "+"))
    {
      includeHigherLevels = true;

      // remove the "+" from the end of the hostname
      hostname = hostname.substr(0, hostname.size() - 1);
    }

    EventLevel level = CEventLog::EventLevelFromString(hostname);

    // get the events of the specified level(s)
    events = log.Get(level, includeHigherLevels);
  }

  for (auto eventItem : events)
    items.Add(EventToFileItem(eventItem));

  return true;
}

CFileItemPtr CEventsDirectory::EventToFileItem(const EventPtr& eventItem)
{
  if (!eventItem)
    return CFileItemPtr();

  CFileItemPtr item(new CFileItem(eventItem));

  item->SetProperty(PROPERTY_EVENT_IDENTIFIER, eventItem->GetIdentifier());
  item->SetProperty(PROPERTY_EVENT_LEVEL, CEventLog::EventLevelToString(eventItem->GetLevel()));
  item->SetProperty(PROPERTY_EVENT_DESCRIPTION, eventItem->GetDescription());

  return item;
}
