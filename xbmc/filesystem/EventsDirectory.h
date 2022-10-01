/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

#include <memory>

class CFileItem;
class CFileItemList;
class IEvent;

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

    static std::shared_ptr<CFileItem> EventToFileItem(
        const std::shared_ptr<const IEvent>& activity);
  };
}
