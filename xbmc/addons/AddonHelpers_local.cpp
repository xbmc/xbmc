/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Addon.h"
#include "AddonHelpers_local.h"
#include "AddonHelpers_Addon.h"
#include "AddonHelpers_GUI.h"
#include "AddonHelpers_PVR.h"
#include "FileSystem/SpecialProtocol.h"
#include "log.h"

namespace ADDON
{

CAddonHelpers::CAddonHelpers(CAddon* addon)
{
  m_addon       = addon;
  m_callbacks   = new AddonCB;
  m_helperAddon = NULL;
  m_helperGUI   = NULL;
  m_helperPVR   = NULL;

  m_callbacks->libBasePath           = strdup(_P("special://xbmcbin/addons"));
  m_callbacks->addonData             = this;
  m_callbacks->AddOnLib_RegisterMe   = CAddonHelpers::AddOnLib_RegisterMe;
  m_callbacks->AddOnLib_UnRegisterMe = CAddonHelpers::AddOnLib_UnRegisterMe;
  m_callbacks->GUILib_RegisterMe     = CAddonHelpers::GUILib_RegisterMe;
  m_callbacks->GUILib_UnRegisterMe   = CAddonHelpers::GUILib_UnRegisterMe;
  m_callbacks->PVRLib_RegisterMe     = CAddonHelpers::PVRLib_RegisterMe;
  m_callbacks->PVRLib_UnRegisterMe   = CAddonHelpers::PVRLib_UnRegisterMe;
}

CAddonHelpers::~CAddonHelpers()
{
  delete m_helperAddon;
  m_helperAddon = NULL;
  delete m_helperGUI;
  m_helperGUI = NULL;
  delete m_helperPVR;
  m_helperPVR = NULL;
  delete m_callbacks;
  m_callbacks = NULL;
}

CB_AddOnLib* CAddonHelpers::AddOnLib_RegisterMe(void *addonData)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: AddOnLib_RegisterMe is called with NULL-Pointer!!!");
    return NULL;
  }

  helper->m_helperAddon = new CAddonHelpers_Addon(helper->m_addon);
  return helper->m_helperAddon->GetCallbacks();
}

void CAddonHelpers::AddOnLib_UnRegisterMe(void *addonData, CB_AddOnLib *cbTable)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: AddOnLib_UnRegisterMe is called with NULL-Pointer!!!");
    return;
  }

  delete helper->m_helperAddon;
  helper->m_helperAddon = NULL;
}

CB_GUILib* CAddonHelpers::GUILib_RegisterMe(void *addonData)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: GUILib_RegisterMe is called with NULL-Pointer!!!");
    return NULL;
  }

  helper->m_helperGUI = new CAddonHelpers_GUI(helper->m_addon);
  return helper->m_helperGUI->GetCallbacks();
}

void CAddonHelpers::GUILib_UnRegisterMe(void *addonData, CB_GUILib *cbTable)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: GUILib_UnRegisterMe is called with NULL-Pointer!!!");
    return;
  }

  delete helper->m_helperGUI;
  helper->m_helperGUI = NULL;
}

CB_PVRLib* CAddonHelpers::PVRLib_RegisterMe(void *addonData)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: PVRLib_RegisterMe is called with NULL-Pointer!!!");
    return NULL;
  }

  helper->m_helperPVR = new CAddonHelpers_PVR(helper->m_addon);
  return helper->m_helperPVR->GetCallbacks();
}

void CAddonHelpers::PVRLib_UnRegisterMe(void *addonData, CB_PVRLib *cbTable)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (helper == NULL)
  {
    CLog::Log(LOGERROR, "Addon-Helper: PVRLib_UnRegisterMe is called with NULL-Pointer!!!");
    return;
  }

  delete helper->m_helperPVR;
  helper->m_helperPVR = NULL;
}

}; /* namespace ADDON */
