/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/VisualisationGUIInfo.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIVisualisationControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

using namespace KODI::GUILIB::GUIINFO;

bool CVisualisationGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CVisualisationGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // VISUALISATION_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      if (msg.GetPointer())
      {
        CGUIVisualisationControl* viz = static_cast<CGUIVisualisationControl*>(msg.GetPointer());
        if (viz)
        {
          value = viz->GetActivePresetName();
          URIUtils::RemoveExtension(value);
          return true;
        }
      }
      break;
    }
    case VISUALISATION_NAME:
    {
      ADDON::AddonPtr addon;
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION);
      if (CServiceBroker::GetAddonMgr().GetAddon(value, addon, ADDON::OnlyEnabled::CHOICE_YES) &&
          addon)
      {
        value = addon->Name();
        return true;
      }
      break;
    }
  }

  return false;
}

bool CVisualisationGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CVisualisationGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // VISUALISATION_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case VISUALISATION_LOCKED:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      if (msg.GetPointer())
      {
        CGUIVisualisationControl *pVis = static_cast<CGUIVisualisationControl*>(msg.GetPointer());
        value = pVis->IsLocked();
        return true;
      }
      break;
    }
    case VISUALISATION_ENABLED:
    {
      value = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty();
      return true;
    }
    case VISUALISATION_HAS_PRESETS:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      if (msg.GetPointer())
      {
        CGUIVisualisationControl* viz = static_cast<CGUIVisualisationControl*>(msg.GetPointer());
        value = (viz && viz->HasPresets());
        return true;
      }
      break;
    }
  }

  return false;
}
