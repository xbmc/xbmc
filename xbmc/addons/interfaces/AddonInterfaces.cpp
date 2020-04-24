/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInterfaces.h"

#include "addons/Addon.h"
#include "addons/interfaces/Addon/AddonCallbacksAddon.h"
#include "addons/interfaces/gui/AddonCallbacksGUI.h"
#include "addons/interfaces/gui/AddonGUIWindow.h"
#include "addons/interfaces/gui/Window.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/addons/PVRClient.h"
#include "utils/log.h"

using namespace KODI;
using namespace MESSAGING;

namespace ADDON
{

CAddonInterfaces::CAddonInterfaces(CAddon* addon)
  : m_callbacks(new AddonCB),
    m_addon(addon),
    m_helperAddOn(nullptr),
    m_helperGUI(nullptr)
{
  m_callbacks->libBasePath                  = strdup(CSpecialProtocol::TranslatePath("special://xbmcbinaddons").c_str());
  m_callbacks->addonData                    = this;

  m_callbacks->AddOnLib_RegisterMe          = CAddonInterfaces::AddOnLib_RegisterMe;
  m_callbacks->AddOnLib_UnRegisterMe        = CAddonInterfaces::AddOnLib_UnRegisterMe;
  m_callbacks->GUILib_RegisterMe            = CAddonInterfaces::GUILib_RegisterMe;
  m_callbacks->GUILib_UnRegisterMe          = CAddonInterfaces::GUILib_UnRegisterMe;
  m_callbacks->PVRLib_RegisterMe            = CAddonInterfaces::PVRLib_RegisterMe;
  m_callbacks->PVRLib_UnRegisterMe          = CAddonInterfaces::PVRLib_UnRegisterMe;
}

CAddonInterfaces::~CAddonInterfaces()
{
  delete static_cast<KodiAPI::AddOn::CAddonCallbacksAddon*>(m_helperAddOn);
  delete static_cast<KodiAPI::GUI::CAddonCallbacksGUI*>(m_helperGUI);

  free(const_cast<char*>(m_callbacks->libBasePath));
  delete m_callbacks;
}

/*\_____________________________________________________________________________
\*/

void* CAddonInterfaces::AddOnLib_RegisterMe(void *addonData)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "CAddonInterfaces - %s - called with a null pointer", __FUNCTION__);
    return nullptr;
  }

  addon->m_helperAddOn = new KodiAPI::AddOn::CAddonCallbacksAddon(addon->m_addon);
  return static_cast<KodiAPI::AddOn::CAddonCallbacksAddon*>(addon->m_helperAddOn)->GetCallbacks();
}

void CAddonInterfaces::AddOnLib_UnRegisterMe(void *addonData, void *cbTable)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "CAddonInterfaces - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  delete static_cast<KodiAPI::AddOn::CAddonCallbacksAddon*>(addon->m_helperAddOn);
  addon->m_helperAddOn = nullptr;
}
/*\_____________________________________________________________________________
\*/
void* CAddonInterfaces::GUILib_RegisterMe(void *addonData)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "CAddonInterfaces - %s - called with a null pointer", __FUNCTION__);
    return nullptr;
  }

  addon->m_helperGUI = new KodiAPI::GUI::CAddonCallbacksGUI(addon->m_addon);
  return static_cast<KodiAPI::GUI::CAddonCallbacksGUI*>(addon->m_helperGUI)->GetCallbacks();
}

void CAddonInterfaces::GUILib_UnRegisterMe(void *addonData, void *cbTable)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "CAddonInterfaces - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  delete static_cast<KodiAPI::GUI::CAddonCallbacksGUI*>(addon->m_helperGUI);
  addon->m_helperGUI = nullptr;
}
/*\_____________________________________________________________________________
\*/
void* CAddonInterfaces::PVRLib_RegisterMe(void *addonData)
{
  CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(addonData);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "CAddonInterfaces - %s - called with a null pointer", __FUNCTION__);
    return nullptr;
  }

  return dynamic_cast<PVR::CPVRClient*>(addon->m_addon)->GetInstanceInterface();
}

void CAddonInterfaces::PVRLib_UnRegisterMe(void *addonData, void *cbTable)
{
}
/*\_____________________________________________________________________________
\*/
void CAddonInterfaces::OnApplicationMessage(ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_ADDON_DIALOG:
  {
    if (pMsg->lpVoid)
    { //! @todo This is ugly - really these binary add-on dialogs should just be normal Kodi dialogs
      switch (pMsg->param1)
      {
      case 0:
        static_cast<ADDON::CGUIAddonWindowDialog*>(pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
        break;
      case 1:
        static_cast<KodiAPI::GUI::CGUIAddonWindowDialog*>(pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
        break;
      };
    }
  }
  break;
  }
}

} /* namespace ADDON */
