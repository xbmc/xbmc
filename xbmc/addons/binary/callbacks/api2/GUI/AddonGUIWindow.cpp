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

#include "Application.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "addons/Skin.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "AddonGUIWindow.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_LABELFILES      12

using namespace ADDON;
using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnWindow::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Window.New                     = CAddOnWindow::New;
  callbacks->GUI.Window.Delete                  = CAddOnWindow::Delete;
  callbacks->GUI.Window.SetCallbacks            = CAddOnWindow::SetCallbacks;
  callbacks->GUI.Window.Show                    = CAddOnWindow::Show;
  callbacks->GUI.Window.Close                   = CAddOnWindow::Close;
  callbacks->GUI.Window.DoModal                 = CAddOnWindow::DoModal;
  callbacks->GUI.Window.SetFocusId              = CAddOnWindow::SetFocusId;
  callbacks->GUI.Window.GetFocusId              = CAddOnWindow::GetFocusId;
  callbacks->GUI.Window.SetProperty             = CAddOnWindow::SetProperty;
  callbacks->GUI.Window.SetPropertyInt          = CAddOnWindow::SetPropertyInt;
  callbacks->GUI.Window.SetPropertyBool         = CAddOnWindow::SetPropertyBool;
  callbacks->GUI.Window.SetPropertyDouble       = CAddOnWindow::SetPropertyDouble;
  callbacks->GUI.Window.GetProperty             = CAddOnWindow::GetProperty;
  callbacks->GUI.Window.GetPropertyInt          = CAddOnWindow::GetPropertyInt;
  callbacks->GUI.Window.GetPropertyBool         = CAddOnWindow::GetPropertyBool;
  callbacks->GUI.Window.GetPropertyDouble       = CAddOnWindow::GetPropertyDouble;
  callbacks->GUI.Window.ClearProperties         = CAddOnWindow::ClearProperties;
  callbacks->GUI.Window.ClearProperty           = CAddOnWindow::ClearProperty;
  callbacks->GUI.Window.GetListSize             = CAddOnWindow::GetListSize;
  callbacks->GUI.Window.ClearList               = CAddOnWindow::ClearList;
  callbacks->GUI.Window.AddItem                 = CAddOnWindow::AddItem;
  callbacks->GUI.Window.AddStringItem           = CAddOnWindow::AddStringItem;
  callbacks->GUI.Window.RemoveItem              = CAddOnWindow::RemoveItem;
  callbacks->GUI.Window.RemoveItemFile          = CAddOnWindow::RemoveItemFile;
  callbacks->GUI.Window.GetListItem             = CAddOnWindow::GetListItem;
  callbacks->GUI.Window.SetCurrentListPosition  = CAddOnWindow::SetCurrentListPosition;
  callbacks->GUI.Window.GetCurrentListPosition  = CAddOnWindow::GetCurrentListPosition;
  callbacks->GUI.Window.SetControlLabel         = CAddOnWindow::SetControlLabel;
  callbacks->GUI.Window.MarkDirtyRegion         = CAddOnWindow::MarkDirtyRegion;

  callbacks->GUI.Window.GetControl_Button       = CAddOnWindow::GetControl_Button;
  callbacks->GUI.Window.GetControl_Edit         = CAddOnWindow::GetControl_Edit;
  callbacks->GUI.Window.GetControl_FadeLabel    = CAddOnWindow::GetControl_FadeLabel;
  callbacks->GUI.Window.GetControl_Image        = CAddOnWindow::GetControl_Image;
  callbacks->GUI.Window.GetControl_Label        = CAddOnWindow::GetControl_Label;
  callbacks->GUI.Window.GetControl_Spin         = CAddOnWindow::GetControl_Spin;
  callbacks->GUI.Window.GetControl_RadioButton  = CAddOnWindow::GetControl_RadioButton;
  callbacks->GUI.Window.GetControl_Progress     = CAddOnWindow::GetControl_Progress;
  callbacks->GUI.Window.GetControl_RenderAddon  = CAddOnWindow::GetControl_RenderAddon;
  callbacks->GUI.Window.GetControl_Slider       = CAddOnWindow::GetControl_Slider;
  callbacks->GUI.Window.GetControl_SettingsSlider= CAddOnWindow::GetControl_SettingsSlider;
  callbacks->GUI.Window.GetControl_TextBox      = CAddOnWindow::GetControl_TextBox;
}


GUIHANDLE CAddOnWindow::New(void *addonData, const char *xmlFilename,
                                const char *defaultSkin, bool forceFallback,
                                bool asDialog)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    RESOLUTION_INFO res;
    std::string strSkinPath;
    if (!forceFallback)
    {
      /* Check to see if the XML file exists in current skin. If not use
         fallback path to find a skin for the addon */
      strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res);

      if (!XFILE::CFile::Exists(strSkinPath))
      {
        /* Check for the matching folder for the skin in the fallback skins folder */
        std::string basePath = URIUtils::AddFileToFolder(guiHelper->GetAddon()->Path(), "resources");
        basePath = URIUtils::AddFileToFolder(basePath, "skins");
        basePath = URIUtils::AddFileToFolder(basePath, URIUtils::GetFileName(g_SkinInfo->Path()));
        strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res, basePath);
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          /* Finally fallback to the DefaultSkin as it didn't exist in either the
             Kodi Skin folder or the fallback skin folder */
          forceFallback = true;
        }
      }
    }

    if (forceFallback)
    {
      //FIXME make this static method of current skin?
      std::string str("none");
      AddonProps props(str, ADDON_SKIN, str, str);
      std::string basePath = URIUtils::AddFileToFolder(guiHelper->GetAddon()->Path(), "resources");
      basePath = URIUtils::AddFileToFolder(basePath, "skins");
      basePath = URIUtils::AddFileToFolder(basePath, defaultSkin);
      props.path = basePath;

      CSkinInfo skinInfo(props);
      skinInfo.Start();
      strSkinPath = skinInfo.GetSkinPath(xmlFilename, &res, basePath);

      if (!XFILE::CFile::Exists(strSkinPath))
      {
        throw ADDON::WrongValueException("CAddOnWindow - New: %s/%s - XML File '%s' for Window is missing, contact Developer '%s' of this AddOn",
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str(), strSkinPath.c_str(), guiHelper->GetAddon()->Author().c_str());
      }
    }
    // window id's 14000 - 14100 are reserved for addons
    // get first window id that is not in use
    int id = WINDOW_ADDON_START;
    // if window 14099 is in use it means addon can't create more windows
    CAddOnGUIGeneral::Lock();
    if (g_windowManager.GetWindow(WINDOW_ADDON_END))
    {
      CAddOnGUIGeneral::Unlock();
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - maximum number of windows reached",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }
    while(id < WINDOW_ADDON_END && g_windowManager.GetWindow(id) != nullptr) id++;
    CAddOnGUIGeneral::Unlock();

    CGUIWindow *window;
    if (!asDialog)
      window = new CGUIAddonWindow(id, strSkinPath, guiHelper->GetAddon());
    else
      window = new CGUIAddonWindowDialog(id, strSkinPath, guiHelper->GetAddon());

    CAddOnGUIGeneral::Lock();
    g_windowManager.Add(window);
    CAddOnGUIGeneral::Unlock();

    window->SetCoordsRes(res);
    return window;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnWindow::Delete(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    CAddOnGUIGeneral::Lock();
    // first change to an existing window
    if (g_windowManager.GetActiveWindow() == pAddonWindow->m_iWindowId && !g_application.m_bStop)
    {
      if(g_windowManager.GetWindow(pAddonWindow->m_iOldWindowId))
        g_windowManager.ActivateWindow(pAddonWindow->m_iOldWindowId);
      else // old window does not exist anymore, switch to home
        g_windowManager.ActivateWindow(WINDOW_HOME);
    }
    // Free any window properties
    pAddonWindow->ClearProperties();
    // free the window's resources and unload it (free all guicontrols)
    pAddonWindow->FreeResources(true);

    g_windowManager.Remove(pAddonWindow->GetID());
    delete pAddonWindow;
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle,
                                    bool (*initCB)(GUIHANDLE), bool (*clickCB)(GUIHANDLE, int),
                                    bool (*focusCB)(GUIHANDLE, int), bool (*onActionCB)(GUIHANDLE handle, int))
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

    CAddOnGUIGeneral::Lock();
    pAddonWindow->m_clientHandle  = clienthandle;
    pAddonWindow->CBOnInit        = initCB;
    pAddonWindow->CBOnClick       = clickCB;
    pAddonWindow->CBOnFocus       = focusCB;
    pAddonWindow->CBOnAction      = onActionCB;
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnWindow::Show(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return false;

    if (pAddonWindow->m_iOldWindowId != pAddonWindow->m_iWindowId && pAddonWindow->m_iWindowId != g_windowManager.GetActiveWindow())
      pAddonWindow->m_iOldWindowId = g_windowManager.GetActiveWindow();

    CAddOnGUIGeneral::Lock();
    if (pAddonWindow->IsDialog())
      ((CGUIAddonWindowDialog*)pAddonWindow)->Show();
    else
      g_windowManager.ActivateWindow(pAddonWindow->m_iWindowId);
    CAddOnGUIGeneral::Unlock();

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnWindow::Close(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return false;

    pAddonWindow->m_bModal = false;
    if (pAddonWindow->IsDialog())
      ((CGUIAddonWindowDialog*)pAddonWindow)->PulseActionEvent();
    else
      ((CGUIAddonWindow*)pAddonWindow)->PulseActionEvent();

    CAddOnGUIGeneral::Lock();
    // if it's a dialog, we have to close it a bit different
    if (pAddonWindow->IsDialog())
      ((CGUIAddonWindowDialog*)pAddonWindow)->Show(false);
    else
      g_windowManager.ActivateWindow(pAddonWindow->m_iOldWindowId);
    pAddonWindow->m_iOldWindowId = 0;

    CAddOnGUIGeneral::Unlock();

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnWindow::DoModal(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return false;

    pAddonWindow->m_bModal = true;

    if (pAddonWindow->m_iWindowId != g_windowManager.GetActiveWindow())
      Show(addonData, handle);

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnWindow::SetFocusId(void *addonData, GUIHANDLE handle, int iControlId)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return false;

    if (!pWindow->GetControl(iControlId))
    {
      CLog::Log(LOGERROR, "CAddOnWindow - %s: %s/%s - Control does not exist in window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
      return false;
    }

    CAddOnGUIGeneral::Lock();
    CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS, pAddonWindow->m_iWindowId, iControlId);
    pWindow->OnMessage(msg);
    CAddOnGUIGeneral::Unlock();

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

int CAddOnWindow::GetFocusId(void *addonData, GUIHANDLE handle)
{
  int iControlId = -1;

  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return iControlId;

    CAddOnGUIGeneral::Lock();
    iControlId = pWindow->GetFocusedControlID();
    CAddOnGUIGeneral::Unlock();

    if (iControlId == -1)
    {
      CLog::Log(LOGERROR, "CAddOnWindow - %s: %s/%s - No control in this window has focus",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
      return iControlId;
    }

    return iControlId;
  }
  HANDLE_ADDON_EXCEPTION

  return iControlId;
}

void CAddOnWindow::SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key || !value)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key or value",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    pWindow->SetProperty(lowerKey, value);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    pWindow->SetProperty(lowerKey, value);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    pWindow->SetProperty(lowerKey, value);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    pWindow->SetProperty(lowerKey, value);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::GetProperty(void *addonData, GUIHANDLE handle, const char *key, char &property, unsigned int &iMaxStringSize)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    std::string value = pWindow->GetProperty(lowerKey).asString();
    CAddOnGUIGeneral::Unlock();

    strncpy(&property, value.c_str(), iMaxStringSize);
    iMaxStringSize = value.length();
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnWindow::GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return -1;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    int value = (int)pWindow->GetProperty(lowerKey).asInteger();
    CAddOnGUIGeneral::Unlock();

    return value;
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

bool CAddOnWindow::GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return false;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    bool value = pWindow->GetProperty(lowerKey).asBoolean();
    CAddOnGUIGeneral::Unlock();

    return value;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

double CAddOnWindow::GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle || !key)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window or nullptr key",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return 0.0;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    double value = pWindow->GetProperty(lowerKey).asDouble();
    CAddOnGUIGeneral::Unlock();

    return value;
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0;
}

void CAddOnWindow::ClearProperties(void *addonData, GUIHANDLE handle)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    CAddOnGUIGeneral::Lock();
    pWindow->ClearProperties();
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::ClearProperty(void *addonData, GUIHANDLE handle, const char *key)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());

    if (!handle)
    {
      throw ADDON::WrongValueException("CAddOnWindow - %s: %s/%s - No Window",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
    }

    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
    CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
    if (!pWindow)
      return;

    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);
    CAddOnGUIGeneral::Lock();
    pWindow->SetProperty(lowerKey, "");
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnWindow::GetListSize(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddOnGUIGeneral::Lock();
    int listSize = static_cast<CGUIAddonWindow*>(handle)->GetListSize();
    CAddOnGUIGeneral::Unlock();

    return listSize;
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

void CAddOnWindow::ClearList(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddOnGUIGeneral::Lock();
    static_cast<CGUIAddonWindow*>(handle)->ClearList();
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

GUIHANDLE CAddOnWindow::AddItem(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition)
{
  try
  {
    if (!handle || !item)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CFileItemPtr pItem((CFileItem*)item);
    CAddOnGUIGeneral::Lock();
    static_cast<CGUIAddonWindow*>(handle)->AddItem(pItem, itemPosition);
    CAddOnGUIGeneral::Unlock();

    return item;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::AddStringItem(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition)
{
  try
  {
    if (!handle || !itemName)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CFileItemPtr item(new CFileItem(itemName));
    CAddOnGUIGeneral::Lock();
    static_cast<CGUIAddonWindow*>(handle)->AddItem(item, itemPosition);
    CAddOnGUIGeneral::Unlock();

    return item.get();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnWindow::RemoveItem(void *addonData, GUIHANDLE handle, int itemPosition)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddOnGUIGeneral::Lock();
    static_cast<CGUIAddonWindow*>(handle)->RemoveItem(itemPosition);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::RemoveItemFile(void *addonData, GUIHANDLE handle, GUIHANDLE fileItem)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddOnGUIGeneral::Lock();
    static_cast<CGUIAddonWindow*>(handle)->RemoveItem((CFileItem*)fileItem);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

GUIHANDLE CAddOnWindow::GetListItem(void *addonData, GUIHANDLE handle, int listPos)
{
  try
  {
    CAddonCallbacks* helper = static_cast<CAddonCallbacks *>(addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CAddonCallbacksAddon* guiHelper = static_cast<V2::KodiAPI::CAddonCallbacksAddon*>(helper->AddOnLib_GetHelper());
    CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

    CAddOnGUIGeneral::Lock();
    CFileItemPtr fi = pAddonWindow->GetListItem(listPos);
    if (fi == nullptr)
    {
      CAddOnGUIGeneral::Unlock();
      CLog::Log(LOGERROR, "CAddOnWindow - %s: %s/%s - Index out of range",
                  __FUNCTION__,
                  TranslateType(guiHelper->GetAddon()->Type()).c_str(),
                  guiHelper->GetAddon()->Name().c_str());
      return nullptr;
    }
    CAddOnGUIGeneral::Unlock();

    return fi.get();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnWindow::SetCurrentListPosition(void *addonData, GUIHANDLE handle, int listPos)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CGUIAddonWindow *pAddonWindow = static_cast<CGUIAddonWindow*>(handle);

    CAddOnGUIGeneral::Lock();
    pAddonWindow->SetCurrentListPosition(listPos);
    CAddOnGUIGeneral::Unlock();
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnWindow::GetCurrentListPosition(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CGUIAddonWindow *pAddonWindow = static_cast<CGUIAddonWindow*>(handle);

    CAddOnGUIGeneral::Lock();
    int listPos = pAddonWindow->GetCurrentListPosition();
    CAddOnGUIGeneral::Unlock();

    return listPos;
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

void CAddOnWindow::SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CGUIAddonWindow *pAddonWindow = static_cast<CGUIAddonWindow*>(handle);

    CGUIMessage msg(GUI_MSG_LABEL_SET, pAddonWindow->m_iWindowId, controlId);
    msg.SetLabel(label);
    pAddonWindow->OnMessage(msg);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnWindow::MarkDirtyRegion(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    static_cast<CGUIAddonWindow*>(handle)->MarkDirtyRegion();
  }
  HANDLE_ADDON_EXCEPTION
}

GUIHANDLE CAddOnWindow::GetControl_Button(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_BUTTON, "button");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_FadeLabel(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_FADELABEL, "fade label");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Label(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_LABEL, "label");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Image(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_IMAGE, "image");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_RadioButton(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_RADIO, "radio button");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Edit(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_EDIT, "edit");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Progress(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_PROGRESS, "progress");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Spin(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_SPINEX, "spin");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_RenderAddon(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    CGUIControl* pGUIControl = static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_RENDERADDON, "renderaddon");
    if (!pGUIControl)
      return nullptr;

    CGUIAddonRenderingControl *pRenderControl = new CGUIAddonRenderingControl((CGUIRenderingControl*)pGUIControl);
    return pRenderControl;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_Slider(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_SLIDER, "slider");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_SettingsSlider(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_SETTINGS_SLIDER, "settings slider");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

GUIHANDLE CAddOnWindow::GetControl_TextBox(void *addonData, GUIHANDLE handle, int controlId)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnWindow - %s - invalid add-on data", __FUNCTION__);

    return static_cast<CGUIAddonWindow*>(handle)->GetAddonControl(controlId, CGUIControl::GUICONTROL_TEXTBOX, "textbox");
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

CGUIAddonWindow::CGUIAddonWindow(int id, const std::string& strXML, CAddon* addon)
 : CGUIMediaWindow(id, strXML.c_str()),
   CBOnInit{nullptr},
   CBOnFocus{nullptr},
   CBOnClick{nullptr},
   CBOnAction{nullptr},
   m_clientHandle{nullptr},
   m_iWindowId(id),
   m_iOldWindowId(0),
   m_bModal(false),
   m_bIsDialog(false),
   m_actionEvent(true),
   m_addon(addon)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIAddonWindow::~CGUIAddonWindow(void)
{
}

CGUIControl* CGUIAddonWindow::GetAddonControl(int controlId, CGUIControl::GUICONTROLTYPES type, std::string typeName)
{
  CGUIControl* pGUIControl = dynamic_cast<CGUIControl*>(GetControl(controlId));
  if (!pGUIControl)
  {
    CLog::Log(LOGERROR, "CGUIAddonGUI_Window::%s: %s/%s - Requested GUI control Id '%i' for '%s' not present!",
                __FUNCTION__,
                TranslateType(m_addon->Type()).c_str(),
                m_addon->Name().c_str(),
                controlId, typeName.c_str());
    return nullptr;
  }
  else if (pGUIControl->GetControlType() != type)
  {
    CLog::Log(LOGERROR, "CGUIAddonGUI_Window::%s: %s/%s - Requested GUI control Id '%i' not the type '%s'!",
                __FUNCTION__,
                TranslateType(m_addon->Type()).c_str(),
                m_addon->Name().c_str(),
                controlId, typeName.c_str());
    return nullptr;
  }

  return pGUIControl;
}

bool CGUIAddonWindow::OnAction(const CAction &action)
{
  try
  {
    // Let addon decide whether it wants to hande action first
    return (CBOnAction && CBOnAction(m_clientHandle, action.GetID()));
  }
  HANDLE_ADDON_EXCEPTION

  return CGUIWindow::OnAction(action);
}

bool CGUIAddonWindow::OnMessage(CGUIMessage& message)
{
  // TODO: We shouldn't be dropping down to CGUIWindow in any of this ideally.
  //       We have to make up our minds about what python should be doing and
  //       what this side of things should be doing
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      return CGUIMediaWindow::OnMessage(message);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIMediaWindow::OnMessage(message);
      if (CBOnInit)
      {
        try { CBOnInit(m_clientHandle); }
        HANDLE_ADDON_EXCEPTION
      }
      return true;
    }
    break;

    case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != (int)message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
      // check if our focused control is one of our category buttons
      int iControl = message.GetControlId();
      if (CBOnFocus)
      {
        try { CBOnFocus(m_clientHandle, iControl); }
        HANDLE_ADDON_EXCEPTION
      }
    }
    break;

    case GUI_MSG_FOCUSED:
    {
      if (HasID(message.GetSenderId()) && CBOnFocus)
      {
        try { CBOnFocus(m_clientHandle, message.GetControlId()); }
        HANDLE_ADDON_EXCEPTION
      }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      // Handle Sort/View internally. Scripters shouldn't use ID 2, 3 or 4.
      if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        CLog::Log(LOGINFO, "WindowXML: Internal asc/dsc button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();*/
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        CLog::Log(LOGINFO, "WindowXML: Internal sort button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();*/
        return true;
      }

      if (CBOnClick && iControl && iControl != (int)this->GetID())
      {
        CGUIControl* controlClicked = (CGUIControl*)this->GetControl(iControl);

        // The old python way used to check list AND SELECITEM method or if its a button, checkmark.
        // Its done this way for now to allow other controls without a python version like togglebutton to still raise a onAction event
        if (controlClicked) // Will get problems if we the id is not on the window and we try to do GetControlType on it. So check to make sure it exists
        {
          if ((controlClicked->IsContainer() && (message.GetParam1() == ACTION_SELECT_ITEM ||
                                                 message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) ||
                                                 !controlClicked->IsContainer())
          {
            try { return CBOnClick(m_clientHandle, iControl); }
            HANDLE_ADDON_EXCEPTION
          }
          else if (controlClicked->IsContainer() && message.GetParam1() == ACTION_MOUSE_RIGHT_CLICK)
          {
//            PyXBMCAction* inf = new PyXBMCAction;
//            inf->pObject = Action_FromAction(CAction(ACTION_CONTEXT_MENU));
//            inf->pCallbackWindow = pCallbackWindow;
//
//            // aquire lock?
//            PyXBMC_AddPendingCall(Py_XBMC_Event_OnAction, inf);
//            PulseActionEvent();
          }
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIAddonWindow::AllocResources(bool forceLoad /*= FALSE */)
{
  std::string tmpDir = URIUtils::GetDirectory(GetProperty("xmlfile").asString());
  std::string fallbackMediaPath;
  URIUtils::GetParentPath(tmpDir, fallbackMediaPath);
  URIUtils::RemoveSlashAtEnd(fallbackMediaPath);
  m_mediaDir = fallbackMediaPath;

  //CLog::Log(LOGDEBUG, "CGUIPythonWindowXML::AllocResources called: %s", fallbackMediaPath.c_str());
  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::AllocResources(forceLoad);
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::FreeResources(bool forceUnLoad /*= FALSE */)
{
  CGUIMediaWindow::FreeResources(forceUnLoad);
}

void CGUIAddonWindow::Render()
{
  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::Render();
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::Update()
{
}

void CGUIAddonWindow::AddItem(CFileItemPtr fileItem, int itemPosition)
{
  if (itemPosition == -1 || itemPosition > m_vecItems->Size())
  {
    m_vecItems->Add(fileItem);
  }
  else if (itemPosition <  -1 &&  !(itemPosition-1 < m_vecItems->Size()))
  {
    m_vecItems->AddFront(fileItem,0);
  }
  else
  {
    m_vecItems->AddFront(fileItem, itemPosition);
  }
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIAddonWindow::RemoveItem(int itemPosition)
{
  m_vecItems->Remove(itemPosition);
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIAddonWindow::RemoveItem(CFileItem* fileItem)
{
  m_vecItems->Remove(fileItem);
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

int CGUIAddonWindow::GetCurrentListPosition()
{
  return m_viewControl.GetSelectedItem();
}

void CGUIAddonWindow::SetCurrentListPosition(int item)
{
  m_viewControl.SetSelectedItem(item);
}

int CGUIAddonWindow::GetListSize()
{
  return m_vecItems->Size();
}

CFileItemPtr CGUIAddonWindow::GetListItem(int position)
{
  if (position < 0 || position >= m_vecItems->Size()) return CFileItemPtr();
  return m_vecItems->Get(position);
}

void CGUIAddonWindow::ClearList()
{
  ClearFileItems();

  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIAddonWindow::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  // maybe on day we can make an easy way to do this context menu
  // with out this method overriding the MediaWindow version, it will display 'Add to Favorites'
}

void CGUIAddonWindow::WaitForActionEvent(unsigned int timeout)
{
  m_actionEvent.WaitMSec(timeout);
  m_actionEvent.Reset();
}

void CGUIAddonWindow::PulseActionEvent()
{
  m_actionEvent.Set();
}

bool CGUIAddonWindow::OnClick(int iItem, const std::string &player)
{
  // Hook Over calling  CGUIMediaWindow::OnClick(iItem) results in it trying to PLAY the file item
  // which if its not media is BAD and 99 out of 100 times undesireable.
  return false;
}

// SetupShares();
/*
 CGUIMediaWindow::OnWindowLoaded() calls SetupShares() so override it
and just call UpdateButtons();
*/
void CGUIAddonWindow::SetupShares()
{
  UpdateButtons();
}


CGUIAddonWindowDialog::CGUIAddonWindowDialog(int id, const std::string& strXML, CAddon* addon)
: CGUIAddonWindow(id,strXML,addon)
{
  m_bRunning = false;
  m_bIsDialog = true;
}

CGUIAddonWindowDialog::~CGUIAddonWindowDialog(void)
{
}

bool CGUIAddonWindowDialog::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
    return CGUIWindow::OnMessage(message);

  return CGUIAddonWindow::OnMessage(message);
}

void CGUIAddonWindowDialog::Show(bool show /* = true */)
{
  unsigned int iCount = g_graphicsContext.exit();
  CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ADDON_DIALOG, CAddonCallbacksAddon::APILevel(), show ? 1 : 0, static_cast<void*>(this));
  g_graphicsContext.restore(iCount);
}

void CGUIAddonWindowDialog::Show_Internal(bool show /* = true */)
{
  if (show)
  {
    m_bModal = true;
    m_bRunning = true;
    g_windowManager.RegisterDialog(this);

    // active this window...
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowId);
    OnMessage(msg);

    // this dialog is derived from GUiMediaWindow
    // make sure it is rendered last
    m_renderOrder = RENDER_ORDER_DIALOG;
    while (m_bRunning && !g_application.m_bStop)
    {
      g_windowManager.ProcessRenderLoop();
    }
  }
  else // hide
  {
    m_bRunning = false;

    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
    OnMessage(msg);

    g_windowManager.RemoveDialog(GetID());
  }
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
