/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "General.h"
#include "DialogOK.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/General.h"

#include "addons/AddonDll.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::gui; // addon-dev-kit namespace

namespace ADDON
{
int Interface_GUIGeneral::m_iAddonGUILockRef = 0;
};

extern "C"
{

namespace ADDON
{

void Interface_GUIGeneral::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi.kodi_gui = (AddonToKodiFuncTable_kodi_gui*)malloc(sizeof(AddonToKodiFuncTable_kodi_gui));
  addonInterface->toKodi.kodi_gui->general.lock = lock;
  addonInterface->toKodi.kodi_gui->general.unlock = unlock;
  addonInterface->toKodi.kodi_gui->general.get_screen_height = get_screen_height;
  addonInterface->toKodi.kodi_gui->general.get_screen_width = get_screen_width;
  addonInterface->toKodi.kodi_gui->general.get_video_resolution = get_video_resolution;
  addonInterface->toKodi.kodi_gui->general.get_current_window_dialog_id = get_current_window_dialog_id;
  addonInterface->toKodi.kodi_gui->general.get_current_window_id = get_current_window_id;

  Interface_GUIDialogOK::Init(addonInterface);
}

void Interface_GUIGeneral::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi.kodi_gui)
  {
    free(addonInterface->toKodi.kodi_gui);
    addonInterface->toKodi.kodi_gui = nullptr;
  }
}

//@{
void Interface_GUIGeneral::lock()
{
  if (m_iAddonGUILockRef == 0)
    g_graphicsContext.Lock();
  ++m_iAddonGUILockRef;
}

void Interface_GUIGeneral::unlock()
{
  if (m_iAddonGUILockRef > 0)
  {
    --m_iAddonGUILockRef;
    if (m_iAddonGUILockRef == 0)
      g_graphicsContext.Unlock();
  }
}
//@}

//@{
int Interface_GUIGeneral::get_screen_height(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return g_graphicsContext.GetHeight();
}

int Interface_GUIGeneral::get_screen_width(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return g_graphicsContext.GetWidth();
}

int Interface_GUIGeneral::get_video_resolution(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  return (int)g_graphicsContext.GetVideoResolution();
}
//@}

//@{
int Interface_GUIGeneral::get_current_window_dialog_id(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CSingleLock gl(g_graphicsContext);
  return g_windowManager.GetTopMostModalDialogID();
}

int Interface_GUIGeneral::get_current_window_id(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "kodi::gui::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CSingleLock gl(g_graphicsContext);
  return g_windowManager.GetActiveWindow();
}

//@}

} /* namespace ADDON */
} /* extern "C" */
