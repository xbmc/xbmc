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


#include "EventsDirectory.h"
#include "URL.h"
#include "events/EventLog.h"
#include "utils/StringUtils.h"

using namespace XFILE;

bool CEventsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  items.ClearProperties();
  items.SetContent("events");

  CEventLog& log = CEventLog::GetInstance();
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

    EventLevel level = CEventLog::GetInstance().EventLevelFromString(hostname);

    // get the events of the specified level(s)
    events = log.Get(level, includeHigherLevels);
  }

  for (auto eventItem : events)
    items.Add(EventToFileItem(eventItem));

  return true;
}

CFileItemPtr CEventsDirectory::EventToFileItem(const EventPtr& eventItem)
{
  if (eventItem == NULL)
    return CFileItemPtr();

  CFileItemPtr item(new CFileItem(eventItem->GetLabel()));
  item->m_dateTime = eventItem->GetDateTime();
  if (!eventItem->GetIcon().empty())
    item->SetIconImage(eventItem->GetIcon());

  item->SetProperty(PROPERTY_EVENT_IDENTIFIER, eventItem->GetIdentifier());
  item->SetProperty(PROPERTY_EVENT_LEVEL, CEventLog::GetInstance().EventLevelToString(eventItem->GetLevel()));
  item->SetProperty(PROPERTY_EVENT_DESCRIPTION, eventItem->GetDescription());

  return item;
}
