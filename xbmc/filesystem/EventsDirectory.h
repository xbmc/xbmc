#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
    CEventsDirectory() = default;
    ~CEventsDirectory() override = default;

    // implementations of IDirectory
    bool GetDirectory(const CURL& url, CFileItemList& items) override;
    bool Create(const CURL& url) override { return true; }
    bool Exists(const CURL& url) override { return true; }
    bool AllowAll() const override { return true; }

    static CFileItemPtr EventToFileItem(const EventPtr& activity);
  };
}
