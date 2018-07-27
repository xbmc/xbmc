/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/BaseEvent.h"
#include "utils/StringUtils.h"

class CUniqueEvent : public CBaseEvent
{
public:
  ~CUniqueEvent() override = default;

protected:
  CUniqueEvent(const CVariant& label, const CVariant& description, EventLevel level = EventLevel::Information)
    : CBaseEvent(StringUtils::CreateUUID(), label, description, level)
  { }
  CUniqueEvent(const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level = EventLevel::Information)
    : CBaseEvent(StringUtils::CreateUUID(), label, description, icon, level)
  { }
  CUniqueEvent(const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevel::Information)
    : CBaseEvent(StringUtils::CreateUUID(), label, description, icon, details, level)
  { }
  CUniqueEvent(const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level = EventLevel::Information)
    : CBaseEvent(StringUtils::CreateUUID(), label, description, icon, details, executionLabel, level)
  { }
};
