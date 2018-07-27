/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/UniqueEvent.h"
#include "addons/IAddon.h"

class CAddonEvent : public CUniqueEvent
{
public:
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description);
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details);
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  ~CAddonEvent() override = default;

  const char* GetType() const override { return "AddonEvent"; }

protected:
  ADDON::AddonPtr m_addon;
};
