/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/UniqueEvent.h"

class CNotificationEvent : public CUniqueEvent
{
public:
  CNotificationEvent(const CVariant& label, const CVariant& description, EventLevel level = EventLevel::Information)
    : CUniqueEvent(label, description, level)
  { }
  CNotificationEvent(const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level = EventLevel::Information)
    : CUniqueEvent(label, description, icon, level)
  { }
  CNotificationEvent(const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevel::Information)
    : CUniqueEvent(label, description, icon, details, level)
  { }
  CNotificationEvent(const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level = EventLevel::Information)
    : CUniqueEvent(label, description, icon, details, executionLabel, level)
  { }
  ~CNotificationEvent() override = default;

  const char* GetType() const override { return "NotificationEvent"; }

  bool CanExecute() const override { return false; }
  bool Execute() const override { return true; }
};
