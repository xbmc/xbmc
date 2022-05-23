/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/SkinGUIInfo.h"

#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI::GUILIB::GUIINFO;

bool CSkinGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CSkinGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SKIN_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SKIN_BOOL:
    {
      bool bInfo = CSkinSettings::GetInstance().GetBool(info.GetData1());
      if (bInfo)
      {
        value = g_localizeStrings.Get(20122); // True
        return true;
      }
      break;
    }
    case SKIN_STRING:
    {
      value = CSkinSettings::GetInstance().GetString(info.GetData1());
      return true;
    }
    case SKIN_THEME:
    {
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      return true;
    }
    case SKIN_COLOUR_THEME:
    {
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
      return true;
    }
    case SKIN_ASPECT_RATIO:
    {
      if (g_SkinInfo)
      {
        value = g_SkinInfo->GetCurrentAspect();
        return true;
      }
      break;
    }
    case SKIN_FONT:
    {
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_FONT);
      return true;
    }
    case SKIN_TIMER_ELAPSEDSECS:
    {
      value = std::to_string(g_SkinInfo->GetTimerElapsedSeconds(info.GetData3()));
      return true;
    }
  }

  return false;
}

bool CSkinGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    case SKIN_INTEGER:
    {
      value = CSkinSettings::GetInstance().GetInt(info.GetData1());
      return true;
    }
    case SKIN_TIMER_ELAPSEDSECS:
    {
      value = g_SkinInfo->GetTimerElapsedSeconds(info.GetData3());
      return true;
    }
  }
  return false;
}

bool CSkinGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SKIN_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SKIN_BOOL:
    {
      value = CSkinSettings::GetInstance().GetBool(info.GetData1());
      return true;
    }
    case SKIN_STRING_IS_EQUAL:
    {
      value = StringUtils::EqualsNoCase(CSkinSettings::GetInstance().GetString(info.GetData1()), info.GetData3());
      return true;
    }
    case SKIN_STRING:
    {
      value = !CSkinSettings::GetInstance().GetString(info.GetData1()).empty();
      return true;
    }
    case SKIN_HAS_THEME:
    {
      std::string theme = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      URIUtils::RemoveExtension(theme);
      value = StringUtils::EqualsNoCase(theme, info.GetData3());
      return true;
    }
    case SKIN_TIMER_IS_RUNNING:
    {
      value = g_SkinInfo->TimerIsRunning(info.GetData3());
      return true;
    }
  }

  return false;
}
