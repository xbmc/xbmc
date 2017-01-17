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

#include "AddonManagementEvent.h"
#include "addons/GUIDialogAddonInfo.h"
#include "filesystem/AddonsDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

CAddonManagementEvent::CAddonManagementEvent(ADDON::AddonPropsPtr addonProps, const CVariant& description)
  : CAddonEvent(addonProps, description)
{ }

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

  CFileItemPtr addonItem;
  if (m_addon) /*! @todo currently still there until everything is reworked to the final way! */
    addonItem = XFILE::CAddonsDirectory::FileItemFromAddon(m_addon, URIUtils::AddFileToFolder("addons://", m_addon->ID()));
  else if (m_addonProps)
    addonItem = XFILE::CAddonsDirectory::FileItemFromAddonProps(m_addonProps, URIUtils::AddFileToFolder("addons://", m_addonProps->ID()));
  if (addonItem == nullptr)
    return false;

  return CGUIDialogAddonInfo::ShowForItem(addonItem);
}
