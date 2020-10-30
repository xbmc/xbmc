/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonEvent.h"

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon, const CVariant& description)
  : CUniqueEvent(addon->Name(), description, addon->Icon()), m_addon(addon)
{ }

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon,
                         const CVariant& description,
                         const CVariant& details)
  : CUniqueEvent(addon->Name(), description, addon->Icon(), details), m_addon(addon)
{ }

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon,
                         const CVariant& description,
                         const CVariant& details,
                         const CVariant& executionLabel)
  : CUniqueEvent(addon->Name(), description, addon->Icon(), details, executionLabel), m_addon(addon)
{ }

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon,
                         EventLevel level,
                         const CVariant& description)
  : CUniqueEvent(addon->Name(), description, addon->Icon(), level), m_addon(addon)
{ }

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon,
                         EventLevel level,
                         const CVariant& description,
                         const CVariant& details)
  : CUniqueEvent(addon->Name(), description, addon->Icon(), details, level), m_addon(addon)
{ }

CAddonEvent::CAddonEvent(const ADDON::AddonPtr& addon,
                         EventLevel level,
                         const CVariant& description,
                         const CVariant& details,
                         const CVariant& executionLabel)
  : CUniqueEvent(addon->Name(), description, addon->Icon(), details, executionLabel, level),
    m_addon(addon)
{ }
