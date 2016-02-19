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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIGeneral.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

int CAddOnGUIGeneral::m_iAddonGUILockRef = 0;

extern "C"
{

void CAddOnGUIGeneral::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.General.Lock                     = CAddOnGUIGeneral::Lock;
  callbacks->GUI.General.Unlock                   = CAddOnGUIGeneral::Unlock;
  callbacks->GUI.General.GetScreenHeight          = CAddOnGUIGeneral::GetScreenHeight;
  callbacks->GUI.General.GetScreenWidth           = CAddOnGUIGeneral::GetScreenWidth;
  callbacks->GUI.General.GetVideoResolution       = CAddOnGUIGeneral::GetVideoResolution;
  callbacks->GUI.General.GetCurrentWindowDialogId = CAddOnGUIGeneral::GetCurrentWindowDialogId;
  callbacks->GUI.General.GetCurrentWindowId       = CAddOnGUIGeneral::GetCurrentWindowId;
}

//@{
void CAddOnGUIGeneral::Lock()
{
  if (m_iAddonGUILockRef == 0)
    g_graphicsContext.Lock();
  ++m_iAddonGUILockRef;
}

void CAddOnGUIGeneral::Unlock()
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
int CAddOnGUIGeneral::GetScreenHeight()
{
  try
  {
    return g_graphicsContext.GetHeight();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

int CAddOnGUIGeneral::GetScreenWidth()
{
  try
  {
    return g_graphicsContext.GetWidth();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

int CAddOnGUIGeneral::GetVideoResolution()
{
  try
  {
    return (int)g_graphicsContext.GetVideoResolution();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}
//@}

//@{
int CAddOnGUIGeneral::GetCurrentWindowDialogId()
{
  try
  {
    CSingleLock gl(g_graphicsContext);
    return g_windowManager.GetTopMostModalDialogID();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

int CAddOnGUIGeneral::GetCurrentWindowId()
{
  try
  {
    CSingleLock gl(g_graphicsContext);
    return g_windowManager.GetActiveWindow();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

//@}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
