/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "AddonCallbacksGUI.h"
#include "AddonGUIRenderingControl.h"
#include "AddonGUIWindow.h"

#include "Application.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "addons/Skin.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIRenderingControl.h"
#include "guilib/GUIKeyboardFactory.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

using namespace ADDON;

namespace V1
{
namespace KodiAPI
{
namespace GUI
{

static int iXBMCGUILockRef = 0;

CAddonCallbacksGUI::CAddonCallbacksGUI(CAddon* addon)
  : ADDON::IAddonInterface(addon, 1, KODI_GUILIB_API_VERSION),
    m_callbacks(new CB_GUILib)
{
  /* GUI Helper functions */
  m_callbacks->Lock                           = CAddonCallbacksGUI::Lock;
  m_callbacks->Unlock                         = CAddonCallbacksGUI::Unlock;
  m_callbacks->GetScreenHeight                = CAddonCallbacksGUI::GetScreenHeight;
  m_callbacks->GetScreenWidth                 = CAddonCallbacksGUI::GetScreenWidth;
  m_callbacks->GetVideoResolution             = CAddonCallbacksGUI::GetVideoResolution;
  m_callbacks->Window_New                     = CAddonCallbacksGUI::Window_New;
  m_callbacks->Window_Delete                  = CAddonCallbacksGUI::Window_Delete;
  m_callbacks->Window_SetCallbacks            = CAddonCallbacksGUI::Window_SetCallbacks;
  m_callbacks->Window_Show                    = CAddonCallbacksGUI::Window_Show;
  m_callbacks->Window_Close                   = CAddonCallbacksGUI::Window_Close;
  m_callbacks->Window_DoModal                 = CAddonCallbacksGUI::Window_DoModal;
  m_callbacks->Window_SetFocusId              = CAddonCallbacksGUI::Window_SetFocusId;
  m_callbacks->Window_GetFocusId              = CAddonCallbacksGUI::Window_GetFocusId;
  m_callbacks->Window_SetCoordinateResolution = CAddonCallbacksGUI::Window_SetCoordinateResolution;
  m_callbacks->Window_SetProperty             = CAddonCallbacksGUI::Window_SetProperty;
  m_callbacks->Window_SetPropertyInt          = CAddonCallbacksGUI::Window_SetPropertyInt;
  m_callbacks->Window_SetPropertyBool         = CAddonCallbacksGUI::Window_SetPropertyBool;
  m_callbacks->Window_SetPropertyDouble       = CAddonCallbacksGUI::Window_SetPropertyDouble;
  m_callbacks->Window_GetProperty             = CAddonCallbacksGUI::Window_GetProperty;
  m_callbacks->Window_GetPropertyInt          = CAddonCallbacksGUI::Window_GetPropertyInt;
  m_callbacks->Window_GetPropertyBool         = CAddonCallbacksGUI::Window_GetPropertyBool;
  m_callbacks->Window_GetPropertyDouble       = CAddonCallbacksGUI::Window_GetPropertyDouble;
  m_callbacks->Window_ClearProperties         = CAddonCallbacksGUI::Window_ClearProperties;

  m_callbacks->Window_GetListSize             = CAddonCallbacksGUI::Window_GetListSize;
  m_callbacks->Window_ClearList               = CAddonCallbacksGUI::Window_ClearList;
  m_callbacks->Window_AddItem                 = CAddonCallbacksGUI::Window_AddItem;
  m_callbacks->Window_AddStringItem           = CAddonCallbacksGUI::Window_AddStringItem;
  m_callbacks->Window_RemoveItem              = CAddonCallbacksGUI::Window_RemoveItem;
  m_callbacks->Window_GetListItem             = CAddonCallbacksGUI::Window_GetListItem;
  m_callbacks->Window_SetCurrentListPosition  = CAddonCallbacksGUI::Window_SetCurrentListPosition;
  m_callbacks->Window_GetCurrentListPosition  = CAddonCallbacksGUI::Window_GetCurrentListPosition;

  m_callbacks->Window_GetControl_Spin         = CAddonCallbacksGUI::Window_GetControl_Spin;
  m_callbacks->Window_GetControl_Button       = CAddonCallbacksGUI::Window_GetControl_Button;
  m_callbacks->Window_GetControl_RadioButton  = CAddonCallbacksGUI::Window_GetControl_RadioButton;
  m_callbacks->Window_GetControl_Edit         = CAddonCallbacksGUI::Window_GetControl_Edit;
  m_callbacks->Window_GetControl_Progress     = CAddonCallbacksGUI::Window_GetControl_Progress;
  m_callbacks->Window_GetControl_RenderAddon  = CAddonCallbacksGUI::Window_GetControl_RenderAddon;
  m_callbacks->Window_GetControl_Slider       = CAddonCallbacksGUI::Window_GetControl_Slider;
  m_callbacks->Window_GetControl_SettingsSlider= CAddonCallbacksGUI::Window_GetControl_SettingsSlider;

  m_callbacks->Window_SetControlLabel         = CAddonCallbacksGUI::Window_SetControlLabel;
  m_callbacks->Window_MarkDirtyRegion         = CAddonCallbacksGUI::Window_MarkDirtyRegion;

  m_callbacks->Control_Spin_SetVisible        = CAddonCallbacksGUI::Control_Spin_SetVisible;
  m_callbacks->Control_Spin_SetText           = CAddonCallbacksGUI::Control_Spin_SetText;
  m_callbacks->Control_Spin_Clear             = CAddonCallbacksGUI::Control_Spin_Clear;
  m_callbacks->Control_Spin_AddLabel          = CAddonCallbacksGUI::Control_Spin_AddLabel;
  m_callbacks->Control_Spin_GetValue          = CAddonCallbacksGUI::Control_Spin_GetValue;
  m_callbacks->Control_Spin_SetValue          = CAddonCallbacksGUI::Control_Spin_SetValue;

  m_callbacks->Control_RadioButton_SetVisible = CAddonCallbacksGUI::Control_RadioButton_SetVisible;
  m_callbacks->Control_RadioButton_SetText    = CAddonCallbacksGUI::Control_RadioButton_SetText;
  m_callbacks->Control_RadioButton_SetSelected= CAddonCallbacksGUI::Control_RadioButton_SetSelected;
  m_callbacks->Control_RadioButton_IsSelected = CAddonCallbacksGUI::Control_RadioButton_IsSelected;

  m_callbacks->Control_Progress_SetPercentage = CAddonCallbacksGUI::Control_Progress_SetPercentage;
  m_callbacks->Control_Progress_GetPercentage = CAddonCallbacksGUI::Control_Progress_GetPercentage;
  m_callbacks->Control_Progress_SetInfo       = CAddonCallbacksGUI::Control_Progress_SetInfo;
  m_callbacks->Control_Progress_GetInfo       = CAddonCallbacksGUI::Control_Progress_GetInfo;
  m_callbacks->Control_Progress_GetDescription= CAddonCallbacksGUI::Control_Progress_GetDescription;

  m_callbacks->ListItem_Create                = CAddonCallbacksGUI::ListItem_Create;
  m_callbacks->ListItem_GetLabel              = CAddonCallbacksGUI::ListItem_GetLabel;
  m_callbacks->ListItem_SetLabel              = CAddonCallbacksGUI::ListItem_SetLabel;
  m_callbacks->ListItem_GetLabel2             = CAddonCallbacksGUI::ListItem_GetLabel2;
  m_callbacks->ListItem_SetLabel2             = CAddonCallbacksGUI::ListItem_SetLabel2;
  m_callbacks->ListItem_SetIconImage          = CAddonCallbacksGUI::ListItem_SetIconImage;
  m_callbacks->ListItem_SetThumbnailImage     = CAddonCallbacksGUI::ListItem_SetThumbnailImage;
  m_callbacks->ListItem_SetInfo               = CAddonCallbacksGUI::ListItem_SetInfo;
  m_callbacks->ListItem_SetProperty           = CAddonCallbacksGUI::ListItem_SetProperty;
  m_callbacks->ListItem_GetProperty           = CAddonCallbacksGUI::ListItem_GetProperty;
  m_callbacks->ListItem_SetPath               = CAddonCallbacksGUI::ListItem_SetPath;

  m_callbacks->RenderAddon_SetCallbacks       = CAddonCallbacksGUI::RenderAddon_SetCallbacks;
  m_callbacks->RenderAddon_Delete             = CAddonCallbacksGUI::RenderAddon_Delete;

  m_callbacks->Control_Slider_SetVisible                    = CAddonCallbacksGUI::Control_Slider_SetVisible;
  m_callbacks->Control_Slider_GetDescription                = CAddonCallbacksGUI::Control_Slider_GetDescription;
  m_callbacks->Control_Slider_SetIntRange                   = CAddonCallbacksGUI::Control_Slider_SetIntRange;
  m_callbacks->Control_Slider_SetIntValue                   = CAddonCallbacksGUI::Control_Slider_SetIntValue;
  m_callbacks->Control_Slider_GetIntValue                   = CAddonCallbacksGUI::Control_Slider_GetIntValue;
  m_callbacks->Control_Slider_SetIntInterval                = CAddonCallbacksGUI::Control_Slider_SetIntInterval;
  m_callbacks->Control_Slider_SetPercentage                 = CAddonCallbacksGUI::Control_Slider_SetPercentage;
  m_callbacks->Control_Slider_GetPercentage                 = CAddonCallbacksGUI::Control_Slider_GetPercentage;
  m_callbacks->Control_Slider_SetFloatRange                 = CAddonCallbacksGUI::Control_Slider_SetFloatRange;
  m_callbacks->Control_Slider_SetFloatValue                 = CAddonCallbacksGUI::Control_Slider_SetFloatValue;
  m_callbacks->Control_Slider_GetFloatValue                 = CAddonCallbacksGUI::Control_Slider_GetFloatValue;
  m_callbacks->Control_Slider_SetFloatInterval              = CAddonCallbacksGUI::Control_Slider_SetFloatInterval;

  m_callbacks->Control_SettingsSlider_SetVisible            = CAddonCallbacksGUI::Control_SettingsSlider_SetVisible;
  m_callbacks->Control_SettingsSlider_SetText               = CAddonCallbacksGUI::Control_SettingsSlider_SetText;
  m_callbacks->Control_SettingsSlider_GetDescription        = CAddonCallbacksGUI::Control_SettingsSlider_GetDescription;
  m_callbacks->Control_SettingsSlider_SetIntRange           = CAddonCallbacksGUI::Control_SettingsSlider_SetIntRange;
  m_callbacks->Control_SettingsSlider_SetIntValue           = CAddonCallbacksGUI::Control_SettingsSlider_SetIntValue;
  m_callbacks->Control_SettingsSlider_GetIntValue           = CAddonCallbacksGUI::Control_SettingsSlider_GetIntValue;
  m_callbacks->Control_SettingsSlider_SetIntInterval        = CAddonCallbacksGUI::Control_SettingsSlider_SetIntInterval;
  m_callbacks->Control_SettingsSlider_SetPercentage         = CAddonCallbacksGUI::Control_SettingsSlider_SetPercentage;
  m_callbacks->Control_SettingsSlider_GetPercentage         = CAddonCallbacksGUI::Control_SettingsSlider_GetPercentage;
  m_callbacks->Control_SettingsSlider_SetFloatRange         = CAddonCallbacksGUI::Control_SettingsSlider_SetFloatRange;
  m_callbacks->Control_SettingsSlider_SetFloatValue         = CAddonCallbacksGUI::Control_SettingsSlider_SetFloatValue;
  m_callbacks->Control_SettingsSlider_GetFloatValue         = CAddonCallbacksGUI::Control_SettingsSlider_GetFloatValue;
  m_callbacks->Control_SettingsSlider_SetFloatInterval      = CAddonCallbacksGUI::Control_SettingsSlider_SetFloatInterval;

  m_callbacks->Dialog_Keyboard_ShowAndGetInputWithHead      = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetInputWithHead;
  m_callbacks->Dialog_Keyboard_ShowAndGetInput              = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetInput;
  m_callbacks->Dialog_Keyboard_ShowAndGetNewPasswordWithHead = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetNewPasswordWithHead;
  m_callbacks->Dialog_Keyboard_ShowAndGetNewPassword        = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetNewPassword;
  m_callbacks->Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead;
  m_callbacks->Dialog_Keyboard_ShowAndVerifyNewPassword     = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyNewPassword;
  m_callbacks->Dialog_Keyboard_ShowAndVerifyPassword        = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyPassword;
  m_callbacks->Dialog_Keyboard_ShowAndGetFilter             = CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetFilter;
  m_callbacks->Dialog_Keyboard_SendTextToActiveKeyboard     = CAddonCallbacksGUI::Dialog_Keyboard_SendTextToActiveKeyboard;
  m_callbacks->Dialog_Keyboard_isKeyboardActivated          = CAddonCallbacksGUI::Dialog_Keyboard_isKeyboardActivated;

  m_callbacks->Dialog_Numeric_ShowAndVerifyNewPassword      = CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyNewPassword;
  m_callbacks->Dialog_Numeric_ShowAndVerifyPassword         = CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyPassword;
  m_callbacks->Dialog_Numeric_ShowAndVerifyInput            = CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyInput;
  m_callbacks->Dialog_Numeric_ShowAndGetTime                = CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetTime;
  m_callbacks->Dialog_Numeric_ShowAndGetDate                = CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetDate;
  m_callbacks->Dialog_Numeric_ShowAndGetIPAddress           = CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetIPAddress;
  m_callbacks->Dialog_Numeric_ShowAndGetNumber              = CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetNumber;
  m_callbacks->Dialog_Numeric_ShowAndGetSeconds             = CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetSeconds;

  m_callbacks->Dialog_FileBrowser_ShowAndGetFile            = CAddonCallbacksGUI::Dialog_FileBrowser_ShowAndGetFile;

  m_callbacks->Dialog_OK_ShowAndGetInputSingleText          = CAddonCallbacksGUI::Dialog_OK_ShowAndGetInputSingleText;
  m_callbacks->Dialog_OK_ShowAndGetInputLineText            = CAddonCallbacksGUI::Dialog_OK_ShowAndGetInputLineText;

  m_callbacks->Dialog_YesNo_ShowAndGetInputSingleText       = CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputSingleText;
  m_callbacks->Dialog_YesNo_ShowAndGetInputLineText         = CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputLineText;
  m_callbacks->Dialog_YesNo_ShowAndGetInputLineButtonText   = CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputLineButtonText;

  m_callbacks->Dialog_TextViewer                            = CAddonCallbacksGUI::Dialog_TextViewer;

  m_callbacks->Dialog_Select                                = CAddonCallbacksGUI::Dialog_Select;
}

CAddonCallbacksGUI::~CAddonCallbacksGUI()
{
  delete m_callbacks;
}

void CAddonCallbacksGUI::Lock()
{
  if (iXBMCGUILockRef == 0) g_graphicsContext.Lock();
  iXBMCGUILockRef++;
}

void CAddonCallbacksGUI::Unlock()
{
  if (iXBMCGUILockRef > 0)
  {
    iXBMCGUILockRef--;
    if (iXBMCGUILockRef == 0) g_graphicsContext.Unlock();
  }
}

int CAddonCallbacksGUI::GetScreenHeight()
{
  return g_graphicsContext.GetHeight();
}

int CAddonCallbacksGUI::GetScreenWidth()
{
  return g_graphicsContext.GetWidth();
}

int CAddonCallbacksGUI::GetVideoResolution()
{
  return (int)g_graphicsContext.GetVideoResolution();
}

GUIHANDLE CAddonCallbacksGUI::Window_New(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return NULL;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

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
      std::string basePath = URIUtils::AddFileToFolder(
        guiHelper->m_addon->Path(),
        "resources",
        "skins",
        URIUtils::GetFileName(g_SkinInfo->Path()));
      strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res, basePath);
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        /* Finally fallback to the DefaultSkin as it didn't exist in either the
           XBMC Skin folder or the fallback skin folder */
        forceFallback = true;
      }
    }
  }

  if (forceFallback)
  {
    //FIXME make this static method of current skin?
    std::string str("none");
    AddonProps props(str, ADDON_SKIN);
    props.path = URIUtils::AddFileToFolder(
      guiHelper->m_addon->Path(),
      "resources",
      "skins",
      defaultSkin);

    CSkinInfo skinInfo(props);
    skinInfo.Start();
    strSkinPath = skinInfo.GetSkinPath(xmlFilename, &res, props.path);

    if (!XFILE::CFile::Exists(strSkinPath))
    {
      CLog::Log(LOGERROR, "Window_New: %s/%s - XML File '%s' for Window is missing, contact Developer '%s' of this AddOn", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str(), strSkinPath.c_str(), guiHelper->m_addon->Author().c_str());
      return NULL;
    }
  }
  // window id's 14000 - 14100 are reserved for addons
  // get first window id that is not in use
  int id = WINDOW_ADDON_START;
  // if window 14099 is in use it means addon can't create more windows
  Lock();
  if (g_windowManager.GetWindow(WINDOW_ADDON_END))
  {
    Unlock();
    CLog::Log(LOGERROR, "Window_New: %s/%s - maximum number of windows reached", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return NULL;
  }
  while(id < WINDOW_ADDON_END && g_windowManager.GetWindow(id) != NULL) id++;
  Unlock();

  CGUIWindow *window;
  if (!asDialog)
    window = new CGUIAddonWindow(id, strSkinPath, guiHelper->m_addon);
  else
    window = new CGUIAddonWindowDialog(id, strSkinPath, guiHelper->m_addon);

  Lock();
  g_windowManager.Add(window);
  Unlock();

  window->SetCoordsRes(res);

  return window;
}

void CAddonCallbacksGUI::Window_Delete(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_Show: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  Lock();
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
  Unlock();
}

void CAddonCallbacksGUI::Window_SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*initCB)(GUIHANDLE), bool (*clickCB)(GUIHANDLE, int), bool (*focusCB)(GUIHANDLE, int), bool (*onActionCB)(GUIHANDLE handle, int))
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  pAddonWindow->m_clientHandle  = clienthandle;
  pAddonWindow->CBOnInit        = initCB;
  pAddonWindow->CBOnClick       = clickCB;
  pAddonWindow->CBOnFocus       = focusCB;
  pAddonWindow->CBOnAction      = onActionCB;
  Unlock();
}

bool CAddonCallbacksGUI::Window_Show(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_Show: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  if (pAddonWindow->m_iOldWindowId != pAddonWindow->m_iWindowId && pAddonWindow->m_iWindowId != g_windowManager.GetActiveWindow())
    pAddonWindow->m_iOldWindowId = g_windowManager.GetActiveWindow();

  Lock();
  if (pAddonWindow->IsDialog())
    ((CGUIAddonWindowDialog*)pAddonWindow)->Show();
  else
    g_windowManager.ActivateWindow(pAddonWindow->m_iWindowId);
  Unlock();

  return true;
}

bool CAddonCallbacksGUI::Window_Close(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_Close: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
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

  Lock();
  // if it's a dialog, we have to close it a bit different
  if (pAddonWindow->IsDialog())
    ((CGUIAddonWindowDialog*)pAddonWindow)->Show(false);
  else
    g_windowManager.ActivateWindow(pAddonWindow->m_iOldWindowId);
  pAddonWindow->m_iOldWindowId = 0;

  Unlock();

  return true;
}

bool CAddonCallbacksGUI::Window_DoModal(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_DoModal: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  pAddonWindow->m_bModal = true;

  if (pAddonWindow->m_iWindowId != g_windowManager.GetActiveWindow())
    Window_Show(addonData, handle);

  return true;
}

bool CAddonCallbacksGUI::Window_SetFocusId(void *addonData, GUIHANDLE handle, int iControlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_SetFocusId: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  if(!pWindow->GetControl(iControlId))
  {
    CLog::Log(LOGERROR, "Window_SetFocusId: %s/%s - Control does not exist in window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  Lock();
  CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS, pAddonWindow->m_iWindowId, iControlId);
  pWindow->OnMessage(msg);
  Unlock();

  return true;
}

int CAddonCallbacksGUI::Window_GetFocusId(void *addonData, GUIHANDLE handle)
{
  int iControlId = -1;

  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return iControlId;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_GetFocusId: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return iControlId;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return iControlId;

  Lock();
  iControlId = pWindow->GetFocusedControlID();
  Unlock();

  if (iControlId == -1)
  {
    CLog::Log(LOGERROR, "Window_GetFocusId: %s/%s - No control in this window has focus", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return iControlId;
  }

  return iControlId;
}

bool CAddonCallbacksGUI::Window_SetCoordinateResolution(void *addonData, GUIHANDLE handle, int res)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "SetCoordinateResolution: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  if (res < RES_HDTV_1080i || res > RES_AUTORES)
  {
    CLog::Log(LOGERROR, "SetCoordinateResolution: %s/%s - Invalid resolution", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  pWindow->SetCoordsRes((RESOLUTION)res);

  return true;
}

void CAddonCallbacksGUI::Window_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key || !value)
  {
    CLog::Log(LOGERROR, "Window_SetProperty: %s/%s - No Window or NULL key or value", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  pWindow->SetProperty(lowerKey, value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyInt: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  pWindow->SetProperty(lowerKey, value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyBool: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  pWindow->SetProperty(lowerKey, value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyDouble: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  pWindow->SetProperty(lowerKey, value);
  Unlock();
}

const char* CAddonCallbacksGUI::Window_GetProperty(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return NULL;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_GetProperty: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return NULL;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return NULL;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  std::string value = pWindow->GetProperty(lowerKey).asString();
  Unlock();

  return strdup(value.c_str());
}

int CAddonCallbacksGUI::Window_GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return -1;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyInt: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return -1;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return -1;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  int value = (int)pWindow->GetProperty(lowerKey).asInteger();
  Unlock();

  return value;
}

bool CAddonCallbacksGUI::Window_GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyBool: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  bool value = pWindow->GetProperty(lowerKey).asBoolean();
  Unlock();

  return value;
}

double CAddonCallbacksGUI::Window_GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return 0.0;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle || !key)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyDouble: %s/%s - No Window or NULL key", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return 0.0;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return 0.0;

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Lock();
  double value = pWindow->GetProperty(lowerKey).asDouble();
  Unlock();

  return value;
}

void CAddonCallbacksGUI::Window_ClearProperties(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_ClearProperties: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  Lock();
  pWindow->ClearProperties();
  Unlock();
}

int CAddonCallbacksGUI::Window_GetListSize(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return -1;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  int listSize = pAddonWindow->GetListSize();
  Unlock();

  return listSize;
}

void CAddonCallbacksGUI::Window_ClearList(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  pAddonWindow->ClearList();
  Unlock();

  return;
}

GUIHANDLE CAddonCallbacksGUI::Window_AddItem(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle || !item)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CFileItemPtr pItem((CFileItem*)item);
  Lock();
  pAddonWindow->AddItem(pItem, itemPosition);
  Unlock();

  return item;
}

GUIHANDLE CAddonCallbacksGUI::Window_AddStringItem(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle || !itemName)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CFileItemPtr item(new CFileItem(itemName));
  Lock();
  pAddonWindow->AddItem(item, itemPosition);
  Unlock();

  return item.get();
}

void CAddonCallbacksGUI::Window_RemoveItem(void *addonData, GUIHANDLE handle, int itemPosition)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  pAddonWindow->RemoveItem(itemPosition);
  Unlock();

  return;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetListItem(void *addonData, GUIHANDLE handle, int listPos)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CAddonCallbacksGUI* guiHelper = static_cast<CAddonCallbacksGUI*>(helper->GUILib_GetHelper());
  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  CFileItemPtr fi = pAddonWindow->GetListItem(listPos);
  if (fi == NULL)
  {
    Unlock();
    CLog::Log(LOGERROR, "Window_GetListItem: %s/%s - Index out of range", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return NULL;
  }
  Unlock();

  return fi.get();
}

void CAddonCallbacksGUI::Window_SetCurrentListPosition(void *addonData, GUIHANDLE handle, int listPos)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  pAddonWindow->SetCurrentListPosition(listPos);
  Unlock();

  return;
}

int CAddonCallbacksGUI::Window_GetCurrentListPosition(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return -1;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  Lock();
  int listPos = pAddonWindow->GetCurrentListPosition();
  Unlock();

  return listPos;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_Spin(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_SPINEX)
    return NULL;

  return pGUIControl;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_Button(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_BUTTON)
    return NULL;

  return pGUIControl;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_RadioButton(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_RADIO)
    return NULL;

  return pGUIControl;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_Edit(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_EDIT)
    return NULL;

  return pGUIControl;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_Progress(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_PROGRESS)
    return NULL;

  return pGUIControl;
}

GUIHANDLE CAddonCallbacksGUI::Window_GetControl_RenderAddon(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_RENDERADDON)
    return NULL;

  CGUIAddonRenderingControl *pProxyControl;
  pProxyControl = new CGUIAddonRenderingControl((CGUIRenderingControl*)pGUIControl);
  return pProxyControl;
}

void CAddonCallbacksGUI::Window_SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  CGUIMessage msg(GUI_MSG_LABEL_SET, pAddonWindow->m_iWindowId, controlId);
  msg.SetLabel(label);
  pAddonWindow->OnMessage(msg);
}

void CAddonCallbacksGUI::Window_MarkDirtyRegion(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  pAddonWindow->MarkDirtyRegion();
}

void CAddonCallbacksGUI::Control_Spin_SetVisible(void *addonData, GUIHANDLE spinhandle, bool yesNo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetVisible(yesNo);
}

void CAddonCallbacksGUI::Control_Spin_SetText(void *addonData, GUIHANDLE spinhandle, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetText(label);
}

void CAddonCallbacksGUI::Control_Spin_Clear(void *addonData, GUIHANDLE spinhandle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->Clear();
}

void CAddonCallbacksGUI::Control_Spin_AddLabel(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->AddLabel(label, iValue);
}

int CAddonCallbacksGUI::Control_Spin_GetValue(void *addonData, GUIHANDLE spinhandle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return -1;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  return pSpin->GetValue();
}

void CAddonCallbacksGUI::Control_Spin_SetValue(void *addonData, GUIHANDLE spinhandle, int iValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetValue(iValue);
}

void CAddonCallbacksGUI::Control_RadioButton_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetVisible(yesNo);
}

void CAddonCallbacksGUI::Control_RadioButton_SetText(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetLabel(label);
}

void CAddonCallbacksGUI::Control_RadioButton_SetSelected(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetSelected(yesNo);
}

bool CAddonCallbacksGUI::Control_RadioButton_IsSelected(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return false;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  return pRadioButton->IsSelected();
}

void CAddonCallbacksGUI::Control_Progress_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  pControl->SetPercentage(fPercent);
}

float CAddonCallbacksGUI::Control_Progress_GetPercentage(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0.0;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  return pControl->GetPercentage();
}

void CAddonCallbacksGUI::Control_Progress_SetInfo(void *addonData, GUIHANDLE handle, int iInfo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  pControl->SetInfo(iInfo);
}

int CAddonCallbacksGUI::Control_Progress_GetInfo(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return -1;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  return pControl->GetInfo();
}

const char* CAddonCallbacksGUI::Control_Progress_GetDescription(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  std::string string = pControl->GetDescription();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

/*
 * GUI slider control callback functions
 */
GUIHANDLE CAddonCallbacksGUI::Window_GetControl_Slider(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_SLIDER)
    return NULL;

  return pGUIControl;
}

void CAddonCallbacksGUI::Control_Slider_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIControl *pControl = (CGUIControl*)handle;
  pControl->SetVisible(yesNo);
}

const char* CAddonCallbacksGUI::Control_Slider_GetDescription(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  std::string string = pControl->GetDescription();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::Control_Slider_SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetRange(iStart, iEnd);
}

void CAddonCallbacksGUI::Control_Slider_SetIntValue(void *addonData, GUIHANDLE handle, int iValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_INT);
  pControl->SetIntValue(iValue);
}

int CAddonCallbacksGUI::Control_Slider_GetIntValue(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  return pControl->GetIntValue();
}

void CAddonCallbacksGUI::Control_Slider_SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetIntInterval(iInterval);
}

void CAddonCallbacksGUI::Control_Slider_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_FLOAT);
  pControl->SetPercentage(fPercent);
}

float CAddonCallbacksGUI::Control_Slider_GetPercentage(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0.0f;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  return pControl->GetPercentage();
}

void CAddonCallbacksGUI::Control_Slider_SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetFloatRange(fStart, fEnd);
}

void CAddonCallbacksGUI::Control_Slider_SetFloatValue(void *addonData, GUIHANDLE handle, float iValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_FLOAT);
  pControl->SetFloatValue(iValue);
}

float CAddonCallbacksGUI::Control_Slider_GetFloatValue(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0.0f;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  return pControl->GetFloatValue();
}

void CAddonCallbacksGUI::Control_Slider_SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISliderControl *pControl = (CGUISliderControl*)handle;
  pControl->SetFloatInterval(fInterval);
}

/*
 * GUI settings slider control callback functions
 */
GUIHANDLE CAddonCallbacksGUI::Window_GetControl_SettingsSlider(void *addonData, GUIHANDLE handle, int controlId)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_SETTINGS_SLIDER)
    return NULL;

  return pGUIControl;
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIControl *pControl = (CGUIControl*)handle;
  pControl->SetVisible(yesNo);
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetText(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetText(label);
}

const char* CAddonCallbacksGUI::Control_SettingsSlider_GetDescription(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  std::string string = pControl->GetDescription();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetRange(iStart, iEnd);
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetIntValue(void *addonData, GUIHANDLE handle, int iValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_INT);
  pControl->SetIntValue(iValue);
}

int CAddonCallbacksGUI::Control_SettingsSlider_GetIntValue(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  return pControl->GetIntValue();
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetIntInterval(iInterval);
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_FLOAT);
  pControl->SetPercentage(fPercent);
}

float CAddonCallbacksGUI::Control_SettingsSlider_GetPercentage(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0.0f;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  return pControl->GetPercentage();
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetFloatRange(fStart, fEnd);
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetFloatValue(void *addonData, GUIHANDLE handle, float fValue)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetType(SPIN_CONTROL_TYPE_FLOAT);
  pControl->SetFloatValue(fValue);
}

float CAddonCallbacksGUI::Control_SettingsSlider_GetFloatValue(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return 0.0f;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  return pControl->GetFloatValue();
}

void CAddonCallbacksGUI::Control_SettingsSlider_SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl*)handle;
  pControl->SetFloatInterval(fInterval);
}

/*
 * GUI list item control callback functions
 */
GUIHANDLE CAddonCallbacksGUI::ListItem_Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper)
    return NULL;

  // create CFileItem
  CFileItem *pItem = new CFileItem();
  if (!pItem)
    return NULL;

  if (label)
    pItem->SetLabel(label);
  if (label2)
    pItem->SetLabel2(label2);
  if (iconImage)
    pItem->SetIconImage(iconImage);
  if (thumbnailImage)
    pItem->SetArt("thumb", thumbnailImage);
  if (path)
    pItem->SetPath(path);

  return pItem;
}

const char* CAddonCallbacksGUI::ListItem_GetLabel(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  std::string string = ((CFileItem*)handle)->GetLabel();
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetLabel(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetLabel(label);
}

const char* CAddonCallbacksGUI::ListItem_GetLabel2(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  std::string string = ((CFileItem*)handle)->GetLabel2();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetLabel2(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetLabel2(label);
}

void CAddonCallbacksGUI::ListItem_SetIconImage(void *addonData, GUIHANDLE handle, const char *image)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetIconImage(image);
}

void CAddonCallbacksGUI::ListItem_SetThumbnailImage(void *addonData, GUIHANDLE handle, const char *image)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetArt("thumb", image);
}

void CAddonCallbacksGUI::ListItem_SetInfo(void *addonData, GUIHANDLE handle, const char *info)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

}

void CAddonCallbacksGUI::ListItem_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetProperty(key, value);
}

const char* CAddonCallbacksGUI::ListItem_GetProperty(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return NULL;

  std::string string = ((CFileItem*)handle)->GetProperty(key).asString();
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetPath(void *addonData, GUIHANDLE handle, const char *path)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetPath(path);
}

void CAddonCallbacksGUI::RenderAddon_SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*createCB)(GUIHANDLE,int,int,int,int,void*), void (*renderCB)(GUIHANDLE), void (*stopCB)(GUIHANDLE), bool (*dirtyCB)(GUIHANDLE))
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonRenderingControl *pAddonControl = (CGUIAddonRenderingControl*)handle;

  Lock();
  pAddonControl->m_clientHandle  = clienthandle;
  pAddonControl->CBCreate        = createCB;
  pAddonControl->CBRender        = renderCB;
  pAddonControl->CBStop          = stopCB;
  pAddonControl->CBDirty         = dirtyCB;
  Unlock();

  pAddonControl->m_pControl->InitCallback(pAddonControl);
}

void CAddonCallbacksGUI::RenderAddon_Delete(void *addonData, GUIHANDLE handle)
{
  CAddonInterfaces* helper = (CAddonInterfaces*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonRenderingControl *pAddonControl = (CGUIAddonRenderingControl*)handle;

  Lock();
  pAddonControl->Delete();
  Unlock();
}

/*! @name GUI Keyboard functions */
//@{
bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetInputWithHead(char &aTextString, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs)
{
  std::string str = &aTextString;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, CVariant{strHeading}, allowEmptyResult, hiddenInput, autoCloseMs);
  if (bRet)
    strncpy(&aTextString, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetInput(char &aTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs)
{
  std::string str = &aTextString;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, allowEmptyResult, autoCloseMs);
  if (bRet)
    strncpy(&aTextString, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetNewPasswordWithHead(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs)
{
  std::string str = &strNewPassword;
  bool bRet = CGUIKeyboardFactory::ShowAndGetNewPassword(str, strHeading, allowEmptyResult, autoCloseMs);
  if (bRet)
    strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs)
{
  std::string str = &strNewPassword;
  bool bRet = CGUIKeyboardFactory::ShowAndGetNewPassword(str, autoCloseMs);
  if (bRet)
    strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs)
{
  std::string str = &strNewPassword;
  bool bRet = CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, strHeading, allowEmptyResult, autoCloseMs);
  if (bRet)
    strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs)
{
  std::string str = &strNewPassword;
  bool bRet = CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, autoCloseMs);
  if (bRet)
    strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

int CAddonCallbacksGUI::Dialog_Keyboard_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs)
{
  std::string str = &strPassword;
  int iRet = CGUIKeyboardFactory::ShowAndVerifyPassword(str, strHeading, iRetries, autoCloseMs);
  if (iRet)
    strncpy(&strPassword, str.c_str(), iMaxStringSize);
  return iRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_ShowAndGetFilter(char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs)
{
  std::string strText = &aTextString;
  bool bRet = CGUIKeyboardFactory::ShowAndGetFilter(strText, searching, autoCloseMs);
  if (bRet)
    strncpy(&aTextString, strText.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Keyboard_SendTextToActiveKeyboard(const char *aTextString, bool closeKeyboard)
{
  return CGUIKeyboardFactory::SendTextToActiveKeyboard(aTextString, closeKeyboard);
}

bool CAddonCallbacksGUI::Dialog_Keyboard_isKeyboardActivated()
{
  return CGUIKeyboardFactory::isKeyboardActivated();
}
//@}

/*! @name GUI Numeric functions */
//@{
bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize)
{
  std::string str = &strNewPassword;
  bool bRet = CGUIDialogNumeric::ShowAndVerifyNewPassword(str);
  if (bRet)
    strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

int CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries)
{
  std::string str = &strPassword;
  int bRet = CGUIDialogNumeric::ShowAndVerifyPassword(str, strHeading, iRetries);
  if (bRet)
    strncpy(&strPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndVerifyInput(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput)
{
  std::string str = &strPassword;
  bool bRet = CGUIDialogNumeric::ShowAndVerifyInput(str, strHeading, bGetUserInput);
  if (bRet)
    strncpy(&strPassword, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetTime(tm &time, const char *strHeading)
{
  SYSTEMTIME systemTime;
  CDateTime dateTime(time);
  dateTime.GetAsSystemTime(systemTime);
  if (CGUIDialogNumeric::ShowAndGetTime(systemTime, strHeading))
  {
    dateTime = systemTime;
    dateTime.GetAsTm(time);
    return true;
  }
  return false;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetDate(tm &date, const char *strHeading)
{
  SYSTEMTIME systemTime;
  CDateTime dateTime(date);
  dateTime.GetAsSystemTime(systemTime);
  if (CGUIDialogNumeric::ShowAndGetDate(systemTime, strHeading))
  {
    dateTime = systemTime;
    dateTime.GetAsTm(date);
    return true;
  }
  return false;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetIPAddress(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading)
{
  std::string strIP = &strIPAddress;
  bool bRet = CGUIDialogNumeric::ShowAndGetIPAddress(strIP, strHeading);
  if (bRet)
    strncpy(&strIPAddress, strIP.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetNumber(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs)
{
  std::string str = &strInput;
  bool bRet = CGUIDialogNumeric::ShowAndGetNumber(str, strHeading, iAutoCloseTimeoutMs);
  if (bRet)
    strncpy(&strInput, str.c_str(), iMaxStringSize);
  return bRet;
}

bool CAddonCallbacksGUI::Dialog_Numeric_ShowAndGetSeconds(char &timeString, unsigned int iMaxStringSize, const char *strHeading)
{
  std::string str = &timeString;
  bool bRet = CGUIDialogNumeric::ShowAndGetSeconds(str, strHeading);
  if (bRet)
    strncpy(&timeString, str.c_str(), iMaxStringSize);
  return bRet;
}
//@}

/*! @name GUI File browser functions */
//@{
bool CAddonCallbacksGUI::Dialog_FileBrowser_ShowAndGetFile(const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList)
{
  std::string strPath = &path;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(directory, mask, heading, strPath, useThumbs, useFileDirectories, singleList);
  if (bRet)
    strncpy(&path, strPath.c_str(), iMaxStringSize);
  return bRet;
}
//@}

/*! @name GUI OK Dialog */
//@{
void CAddonCallbacksGUI::Dialog_OK_ShowAndGetInputSingleText(const char *heading, const char *text)
{
  CGUIDialogOK::ShowAndGetInput(CVariant{heading}, CVariant{text});
}

void CAddonCallbacksGUI::Dialog_OK_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2)
{
  CGUIDialogOK::ShowAndGetInput(CVariant{heading}, CVariant{line0}, CVariant{line1}, CVariant{line2});
}
//@}

/*! @name GUI Yes No Dialog */
//@{
bool CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputSingleText(const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel)
{
  DialogResponse result = HELPERS::ShowYesNoDialogText(heading, text, noLabel, yesLabel);
  bCanceled = result == DialogResponse::CANCELLED;
  return result == DialogResponse::YES;
}

bool CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel)
{
  return HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel) ==
    DialogResponse::YES;
}

bool CAddonCallbacksGUI::Dialog_YesNo_ShowAndGetInputLineButtonText(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel)
{
  DialogResponse result = HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel);
  bCanceled = result == DialogResponse::CANCELLED;
  return result == DialogResponse::YES;
}
//@}

/*! @name GUI Text viewer Dialog */
//@{
void CAddonCallbacksGUI::Dialog_TextViewer(const char *heading, const char *text)
{
  CGUIDialogTextViewer* pDialog = (CGUIDialogTextViewer*)g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);
  pDialog->SetHeading(heading);
  pDialog->SetText(text);
  pDialog->Open();
}
//@}

/*! @name GUI select Dialog */
//@{
int CAddonCallbacksGUI::Dialog_Select(const char *heading, const char *entries[], unsigned int size, int selected)
{
  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetHeading(CVariant{heading});

  for (unsigned int i = 0; i < size; i++)
    pDialog->Add(entries[i]);

  if (selected > 0)
    pDialog->SetSelected(selected);

  pDialog->Open();
  return pDialog->GetSelectedItem();
}
//@}

} /* namespace GUI */
} /* namespace KodiAPI */
} /* namespace V1 */
