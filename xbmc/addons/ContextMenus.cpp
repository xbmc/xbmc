/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/gui/GUIHelpers.h"

namespace CONTEXTMENU
{

using namespace ADDON;

bool CAddonInfo::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo();
}

bool CAddonInfo::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CGUIDialogAddonInfo::ShowForItem(item);
}

bool CAddonSettings::IsVisible(const CFileItem& item) const
{
  AddonPtr addon;
  return item.HasAddonInfo() &&
         CServiceBroker::GetAddonMgr().GetAddon(item.GetAddonInfo()->ID(), addon,
                                                OnlyEnabled::CHOICE_NO) &&
         addon->CanHaveAddonOrInstanceSettings();
}

bool CAddonSettings::Execute(const std::shared_ptr<CFileItem>& item) const
{
  AddonPtr addon;
  return CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), addon,
                                                OnlyEnabled::CHOICE_NO) &&
         CGUIDialogAddonSettings::ShowForAddon(addon);
}

bool CCheckForUpdates::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo() && item.GetAddonInfo()->Type() == AddonType::REPOSITORY;
}

bool CCheckForUpdates::Execute(const std::shared_ptr<CFileItem>& item) const
{
  AddonPtr addon;
  if (item->HasAddonInfo() &&
      CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), addon,
                                             AddonType::REPOSITORY, OnlyEnabled::CHOICE_YES))
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

bool CEnableAddon::Execute(const std::shared_ptr<CFileItem>& item) const
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

bool CDisableAddon::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetAddonMgr().DisableAddon(item->GetAddonInfo()->ID(),
                                                    AddonDisabledReason::USER);
}
}
