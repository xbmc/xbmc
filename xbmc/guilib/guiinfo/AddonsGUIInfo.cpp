/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/AddonsGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "utils/StringUtils.h"

using namespace KODI::GUILIB::GUIINFO;

bool CAddonsGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CAddonsGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  const std::shared_ptr<const ADDON::IAddon> addonInfo = item->GetAddonInfo();
  if (addonInfo)
  {
    switch (info.m_info)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////
      // LISTITEM_*
      ///////////////////////////////////////////////////////////////////////////////////////////////
      case LISTITEM_ADDON_NAME:
        value = addonInfo->Name();
        return true;
      case LISTITEM_ADDON_VERSION:
        value = addonInfo->Version().asString();
        return true;
      case LISTITEM_ADDON_CREATOR:
        value = addonInfo->Author();
        return true;
      case LISTITEM_ADDON_SUMMARY:
        value = addonInfo->Summary();
        return true;
      case LISTITEM_ADDON_DESCRIPTION:
        value = addonInfo->Description();
        return true;
      case LISTITEM_ADDON_DISCLAIMER:
        value = addonInfo->Disclaimer();
        return true;
      case LISTITEM_ADDON_NEWS:
        value = addonInfo->ChangeLog();
        return true;
      case LISTITEM_ADDON_BROKEN:
      {
        // Fallback for old GUI info
        if (addonInfo->LifecycleState() == ADDON::AddonLifecycleState::BROKEN)
          value = addonInfo->LifecycleStateDescription();
        else
          value = "";
        return true;
      }
      case LISTITEM_ADDON_LIFECYCLE_TYPE:
      {
        const ADDON::AddonLifecycleState state = addonInfo->LifecycleState();
        switch (state)
        {
          case ADDON::AddonLifecycleState::BROKEN:
            value = g_localizeStrings.Get(24171); // "Broken"
            break;
          case ADDON::AddonLifecycleState::DEPRECATED:
            value = g_localizeStrings.Get(24170); // "Deprecated";
            break;
          case ADDON::AddonLifecycleState::NORMAL:
          default:
            value = g_localizeStrings.Get(24169); // "Normal";
            break;
        }
        return true;
      }
      case LISTITEM_ADDON_LIFECYCLE_DESC:
        value = addonInfo->LifecycleStateDescription();
        return true;
      case LISTITEM_ADDON_TYPE:
        value = ADDON::CAddonInfo::TranslateType(addonInfo->Type(), true);
        return true;
      case LISTITEM_ADDON_INSTALL_DATE:
        value = addonInfo->InstallDate().GetAsLocalizedDateTime();
        return true;
      case LISTITEM_ADDON_LAST_UPDATED:
        if (addonInfo->LastUpdated().IsValid())
        {
          value = addonInfo->LastUpdated().GetAsLocalizedDateTime();
          return true;
        }
        break;
      case LISTITEM_ADDON_LAST_USED:
        if (addonInfo->LastUsed().IsValid())
        {
          value = addonInfo->LastUsed().GetAsLocalizedDateTime();
          return true;
        }
        break;
      case LISTITEM_ADDON_ORIGIN:
      {
        if (item->GetAddonInfo()->Origin() == ADDON::ORIGIN_SYSTEM)
        {
          value = g_localizeStrings.Get(24992);
          return true;
        }
        if (!item->GetAddonInfo()->OriginName().empty())
        {
          value = item->GetAddonInfo()->OriginName();
          return true;
        }
        else if (!item->GetAddonInfo()->Origin().empty())
        {
          value = item->GetAddonInfo()->Origin();
          return true;
        }
        value = g_localizeStrings.Get(25014);
        return true;
      }
      case LISTITEM_ADDON_SIZE:
      {
        uint64_t packageSize = item->GetAddonInfo()->PackageSize();
        if (packageSize > 0)
        {
          value = StringUtils::FormatFileSize(packageSize);
          return true;
        }
        break;
      }
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // ADDON_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case ADDON_SETTING_STRING:
    {
      ADDON::AddonPtr addon;
      if (!CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon,
                                                  ADDON::OnlyEnabled::CHOICE_YES))
      {
        return false;
      }
      value = addon->GetSetting(info.GetData5());
      return true;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_ADDON_TITLE:
    case SYSTEM_ADDON_ICON:
    case SYSTEM_ADDON_VERSION:
    {
      // This logic does not check/care whether an addon has been disabled/marked as broken,
      // it simply retrieves it's name or icon that means if an addon is placed on the home screen it
      // will stay there even if it's disabled/marked as broken. This might need to be changed/fixed
      // in the future.
      ADDON::AddonPtr addon;
      if (!info.GetData3().empty())
      {
        bool success = CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon,
                                                              ADDON::OnlyEnabled::CHOICE_YES);
        if (!success || !addon)
          break;

        if (info.m_info == SYSTEM_ADDON_TITLE)
        {
          value = addon->Name();
          return true;
        }
        if (info.m_info == SYSTEM_ADDON_ICON)
        {
          value = addon->Icon();
          return true;
        }
        if (info.m_info == SYSTEM_ADDON_VERSION)
        {
          value = addon->Version().asString();
          return true;
        }
      }
      break;
    }
  }

  return false;
}

bool CAddonsGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // ADDON_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case ADDON_SETTING_INT:
    {
      ADDON::AddonPtr addon;
      if (!CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon,
                                                  ADDON::OnlyEnabled::CHOICE_YES))
      {
        return false;
      }
      return addon->GetSettingInt(info.GetData5(), value);
    }
  }
  return false;
}

bool CAddonsGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // ADDON_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case ADDON_SETTING_BOOL:
    {
      ADDON::AddonPtr addon;
      if (!CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon,
                                                  ADDON::OnlyEnabled::CHOICE_YES))
      {
        return false;
      }
      return addon->GetSettingBool(info.GetData5(), value);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_HAS_ADDON:
    {
      ADDON::AddonPtr addon;
      value = CServiceBroker::GetAddonMgr().IsAddonInstalled(info.GetData3());
      return true;
    }
    case SYSTEM_ADDON_IS_ENABLED:
    {
      value = false;
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        value = !CServiceBroker::GetAddonMgr().IsAddonDisabled(info.GetData3());
      return true;
    }
    case LISTITEM_ISAUTOUPDATEABLE:
    {
      value = true;
      const CFileItem* item = static_cast<const CFileItem*>(gitem);
      if (item->GetAddonInfo())
        value = CServiceBroker::GetAddonMgr().IsAutoUpdateable(item->GetAddonInfo()->ID()) ||
                !CServiceBroker::GetAddonMgr().IsAddonInstalled(item->GetAddonInfo()->ID(),
                                                                item->GetAddonInfo()->Origin());

      //! @Todo: store origin of not-autoupdateable installed addons in table 'update_rules'
      //         of the addon database. this is needed to pin ambiguous addon-id's that are
      //         available from multiple origins accordingly.
      //
      //         after this is done the above call should be changed to
      //
      //         value = CServiceBroker::GetAddonMgr().IsAutoUpdateable(item->GetAddonInfo()->ID(),
      //                                                                item->GetAddonInfo()->Origin());

      return true;
    }
  }

  return false;
}
