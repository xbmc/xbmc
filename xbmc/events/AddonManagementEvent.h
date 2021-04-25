/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "events/AddonEvent.h"
#include "events/IEvent.h"
#include "utils/Variant.h"

#include <cstddef>
#include <memory>
#include <string>

class CAddonManagementEvent : public CAddonEvent
{
public:
  CAddonManagementEvent(const ADDON::AddonPtr& addon, const CVariant& description);
  CAddonManagementEvent(const ADDON::AddonPtr& addon,
                        const CVariant& description,
                        const CVariant& details);
  CAddonManagementEvent(const ADDON::AddonPtr& addon,
                        const CVariant& description,
                        const CVariant& details,
                        const CVariant& executionLabel);
  CAddonManagementEvent(const ADDON::AddonPtr& addon,
                        EventLevel level,
                        const CVariant& description);
  CAddonManagementEvent(const ADDON::AddonPtr& addon,
                        EventLevel level,
                        const CVariant& description,
                        const CVariant& details);
  CAddonManagementEvent(const ADDON::AddonPtr& addon,
                        EventLevel level,
                        const CVariant& description,
                        const CVariant& details,
                        const CVariant& executionLabel);
  ~CAddonManagementEvent() override = default;

  const char* GetType() const override { return "AddonManagementEvent"; }
  std::string GetExecutionLabel() const override;

  bool CanExecute() const override { return m_addon != NULL; }
  bool Execute() const override;
};
