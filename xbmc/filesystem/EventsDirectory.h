#pragma once
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

#include "FileItem.h"
#include "events/IEvent.h"
#include "filesystem/IDirectory.h"

#define PROPERTY_EVENT_IDENTIFIER  "Event.ID"
#define PROPERTY_EVENT_LEVEL       "Event.Level"
#define PROPERTY_EVENT_DESCRIPTION "Event.Description"

namespace XFILE
{
  class CEventsDirectory : public IDirectory
  {
  public:
    CEventsDirectory() { }
    virtual ~CEventsDirectory() { }

    // implementations of IDirectory
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool Create(const CURL& url) { return true; }
    virtual bool Exists(const CURL& url) { return true; }
    virtual bool AllowAll() const { return true; }

    static CFileItemPtr EventToFileItem(const EventPtr& activity);
  };
}
