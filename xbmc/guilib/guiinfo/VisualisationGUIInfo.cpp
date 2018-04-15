/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "guilib/guiinfo/VisualisationGUIInfo.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIVisualisationControl.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

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
      value = CServiceBroker::GetSettings().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION);
      if (CServiceBroker::GetAddonMgr().GetAddon(value, addon) && addon)
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
      }
      break;
    }
    case VISUALISATION_ENABLED:
    {
      value = !CServiceBroker::GetSettings().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty();
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
