/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "AddonManager.h"
#include "Repository.h"
#include "RepositoryUpdater.h"
#include "ServiceBroker.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/gui/GUIHelpers.h"

namespace CONTEXTMENU
{

using namespace ADDON;

bool CAddonSettings::IsVisible(const CFileItem& item) const
{
  AddonPtr addon;
  return item.HasAddonInfo() &&
         CServiceBroker::GetAddonMgr().GetAddon(item.GetAddonInfo()->ID(), addon, ADDON_UNKNOWN,
                                                OnlyEnabled::NO) &&
         addon->HasSettings();
}

bool CAddonSettings::Execute(const CFileItemPtr& item) const
{
  AddonPtr addon;
  return CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_UNKNOWN,
                                                OnlyEnabled::NO) &&
         CGUIDialogAddonSettings::ShowForAddon(addon);
}

bool CCheckForUpdates::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo() && item.GetAddonInfo()->Type() == ADDON::ADDON_REPOSITORY;
}

bool CCheckForUpdates::Execute(const CFileItemPtr& item) const
{
  AddonPtr addon;
  if (item->HasAddonInfo() &&
      CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_REPOSITORY,
                                             OnlyEnabled::YES))
  {
    CServiceBroker::GetRepositoryUpdater().CheckForUpdates(std::static_pointer_cast<CRepository>(addon), true);
    return true;
  }
  return false;
}


bool CEnableAddon::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo() &&
      CServiceBroker::GetAddonMgr().IsAddonDisabled(item.GetAddonInfo()->ID()) &&
      CServiceBroker::GetAddonMgr().CanAddonBeEnabled(item.GetAddonInfo()->ID());
}

bool CEnableAddon::Execute(const CFileItemPtr& item) const
{
  // Check user want to enable if lifecycle not normal
  if (!ADDON::GUI::CHelpers::DialogAddonLifecycleUseAsk(item->GetAddonInfo()))
    return false;

  return CServiceBroker::GetAddonMgr().EnableAddon(item->GetAddonInfo()->ID());
}

bool CDisableAddon::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo() &&
      !CServiceBroker::GetAddonMgr().IsAddonDisabled(item.GetAddonInfo()->ID()) &&
      CServiceBroker::GetAddonMgr().CanAddonBeDisabled(item.GetAddonInfo()->ID());
}

bool CDisableAddon::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetAddonMgr().DisableAddon(item->GetAddonInfo()->ID(),
                                                    AddonDisabledReason::USER);
}
}
