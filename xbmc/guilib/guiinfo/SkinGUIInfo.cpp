/*
 *      Copyright (C) 2012-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/guiinfo/SkinGUIInfo.h"

#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SkinSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

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
      value = CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      return true;
    }
    case SKIN_COLOUR_THEME:
    {
      value = CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
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
      value = CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_FONT);
      return true;
    }
  }

  return false;
}

bool CSkinGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
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
    case SKIN_STRING:
    {
      if (!info.GetData3().empty())
        value = StringUtils::EqualsNoCase(CSkinSettings::GetInstance().GetString(info.GetData1()), info.GetData3());
      else
        value = !CSkinSettings::GetInstance().GetString(info.GetData1()).empty();
      return true;
    }
    case SKIN_HAS_THEME:
    {
      std::string theme = CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      URIUtils::RemoveExtension(theme);
      value = StringUtils::EqualsNoCase(theme, info.GetData3());
      return true;
    }
  }

  return false;
}
