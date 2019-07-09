/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonManagementEvent.h"

#include "addons/GUIDialogAddonInfo.h"
#include "filesystem/AddonsDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description)
  : CAddonEvent(addon, description)
{ }

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details)
  : CAddonEvent(addon, description, details)
{ }

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details, const CVariant& executionLabel)
  : CAddonEvent(addon, description, details, executionLabel)
{ }

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description)
  : CAddonEvent(addon, level, description)
{ }

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details)
  : CAddonEvent(addon, level, description, details)
{ }

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details, const CVariant& executionLabel)
  : CAddonEvent(addon, level, description, details, executionLabel)
{ }

std::string CAddonManagementEvent::GetExecutionLabel() const
{
  std::string executionLabel = CAddonEvent::GetExecutionLabel();
  if (!executionLabel.empty())
    return executionLabel;

  return g_localizeStrings.Get(24139);
}

bool CAddonManagementEvent::Execute() const
{
  if (!CanExecute())
    return false;

  CFileItemPtr addonItem = XFILE::CAddonsDirectory::FileItemFromAddon(m_addon, URIUtils::AddFileToFolder("addons://", m_addon->ID()));
  if (addonItem == nullptr)
    return false;

  return CGUIDialogAddonInfo::ShowForItem(addonItem);
}
