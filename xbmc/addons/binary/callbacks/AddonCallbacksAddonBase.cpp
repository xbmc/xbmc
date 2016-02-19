/*
 *      Copyright (C) 2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonCallbacksAddonBase.h"
#include "addons/binary/callbacks/api1/GUI/AddonCallbacksGUI.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI::MESSAGING;

namespace ADDON
{

int CAddonCallbacksAddonBase::APILevel()
{
  return V2::KodiAPI::CAddonCallbacksAddon::APILevel();
}

int CAddonCallbacksAddonBase::MinAPILevel()
{
  return V1::KodiAPI::AddOn::CAddonCallbacksAddon::APILevel();
}

std::string CAddonCallbacksAddonBase::Version()
{
  return V2::KodiAPI::CAddonCallbacksAddon::Version();
}

std::string CAddonCallbacksAddonBase::MinVersion()
{
  return V1::KodiAPI::AddOn::CAddonCallbacksAddon::Version();
}

void* CAddonCallbacksAddonBase::CreateHelper(CAddonCallbacks* addon, int level)
{
  switch (level)
  {
  case 1:
    addon->m_helperAddOn = new V1::KodiAPI::AddOn::CAddonCallbacksAddon(addon->m_addon);
    return static_cast<V1::KodiAPI::AddOn::CAddonCallbacksAddon*>(addon->m_helperAddOn)->GetCallbacks();
  case 2:
    addon->m_helperAddOn = new V2::KodiAPI::CAddonCallbacksAddon(addon->m_addon);
    return static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(addon->m_helperAddOn)->GetCallbacks();
  };
  return nullptr;
}

void CAddonCallbacksAddonBase::DestroyHelper(CAddonCallbacks* addon)
{
  if (!addon->m_helperAddOn)
    return;

  switch (static_cast<IAddonCallback*>(addon->m_helperAddOn)->APILevel())
  {
  case 1:
    delete static_cast<V1::KodiAPI::AddOn::CAddonCallbacksAddon*>(addon->m_helperAddOn);
    break;
  case 2:
    delete static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(addon->m_helperAddOn);
    break;
  };
  addon->m_helperAddOn = nullptr;
}

void CAddonCallbacksAddonBase::OnApplicationMessage(ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_ADDON_DIALOG:
  {
    if (pMsg->lpVoid)
    { // TODO: This is ugly - really these binary add-on dialogs should just be normal Kodi dialogs
      switch (pMsg->param1)
      {
      case 1:
        static_cast<V1::KodiAPI::GUI::CGUIAddonWindowDialog*>(pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
        break;
      case 2:
        static_cast<V2::KodiAPI::GUI::CGUIAddonWindowDialog*>(pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
        break;
      };
    }
  }
  break;
  }
}

}; /* namespace ADDON */
