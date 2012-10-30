/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Application.h"
#include "ApplicationMessenger.h"
#include "Addon.h"
#include "AddonCallbacksGUI.h"
#include "utils/log.h"
#include "Skin.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/TimeUtils.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "settings/GUISettings.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIProgressControl.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_LABELFILES      12

using namespace std;

namespace ADDON
{

static int iXBMCGUILockRef = 0;

CAddonCallbacksGUI::CAddonCallbacksGUI(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_GUILib;

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

  m_callbacks->Window_SetControlLabel         = CAddonCallbacksGUI::Window_SetControlLabel;

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return NULL;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  RESOLUTION_INFO res;
  CStdString strSkinPath;
  if (!forceFallback)
  {
    /* Check to see if the XML file exists in current skin. If not use
       fallback path to find a skin for the addon */
    strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res);

    if (!XFILE::CFile::Exists(strSkinPath))
    {
      /* Check for the matching folder for the skin in the fallback skins folder */
      CStdString basePath;
      URIUtils::AddFileToFolder(guiHelper->m_addon->Path(), "resources", basePath);
      URIUtils::AddFileToFolder(basePath, "skins", basePath);
      URIUtils::AddFileToFolder(basePath, URIUtils::GetFileName(g_SkinInfo->Path()), basePath);
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
    CStdString str("none");
    AddonProps props(str, ADDON_SKIN, str, str);
    CSkinInfo skinInfo(props);
    CStdString basePath;
    URIUtils::AddFileToFolder(guiHelper->m_addon->Path(), "resources", basePath);
    URIUtils::AddFileToFolder(basePath, "skins", basePath);
    URIUtils::AddFileToFolder(basePath, defaultSkin, basePath);
    props.path = basePath;

    skinInfo.Start();
    strSkinPath = skinInfo.GetSkinPath(xmlFilename, &res, basePath);

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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

  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return iControlId;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_SetProperty: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  CStdString lowerKey = key;

  Lock();
  pWindow->SetProperty(lowerKey.ToLower(), value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyInt: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  CStdString lowerKey = key;

  Lock();
  pWindow->SetProperty(lowerKey.ToLower(), value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyBool: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  CStdString lowerKey = key;

  Lock();
  pWindow->SetProperty(lowerKey.ToLower(), value);
  Unlock();
}

void CAddonCallbacksGUI::Window_SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_SetPropertyDouble: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return;

  CStdString lowerKey = key;

  Lock();
  pWindow->SetProperty(lowerKey.ToLower(), value);
  Unlock();
}

const char* CAddonCallbacksGUI::Window_GetProperty(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return NULL;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_GetProperty: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return NULL;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return NULL;

  Lock();
  CStdString lowerKey = key;
  string value = pWindow->GetProperty(lowerKey.ToLower()).asString();
  Unlock();

  return strdup(value.c_str());
}

int CAddonCallbacksGUI::Window_GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return -1;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyInt: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return -1;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return -1;

  Lock();
  CStdString lowerKey = key;
  int value = (int)pWindow->GetProperty(lowerKey.ToLower()).asInteger();
  Unlock();

  return value;
}

bool CAddonCallbacksGUI::Window_GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return false;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyBool: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return false;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return false;

  Lock();
  CStdString lowerKey = key;
  bool value = pWindow->GetProperty(lowerKey.ToLower()).asBoolean();
  Unlock();

  return value;
}

double CAddonCallbacksGUI::Window_GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return 0.0;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

  if (!handle)
  {
    CLog::Log(LOGERROR, "Window_GetPropertyDouble: %s/%s - No Window", TranslateType(guiHelper->m_addon->Type()).c_str(), guiHelper->m_addon->Name().c_str());
    return 0.0;
  }

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIWindow      *pWindow      = (CGUIWindow*)g_windowManager.GetWindow(pAddonWindow->m_iWindowId);
  if (!pWindow)
    return 0.0;

  Lock();
  CStdString lowerKey = key;
  double value = pWindow->GetProperty(lowerKey.ToLower()).asDouble();
  Unlock();

  return value;
}

void CAddonCallbacksGUI::Window_ClearProperties(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();

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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  CAddonCallbacksGUI* guiHelper = helper->GetHelperGUI();
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;
  CGUIControl* pGUIControl = (CGUIControl*)pAddonWindow->GetControl(controlId);
  if (pGUIControl && pGUIControl->GetControlType() != CGUIControl::GUICONTROL_PROGRESS)
    return NULL;

  return pGUIControl;
}

void CAddonCallbacksGUI::Window_SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIAddonWindow *pAddonWindow = (CGUIAddonWindow*)handle;

  CGUIMessage msg(GUI_MSG_LABEL_SET, pAddonWindow->m_iWindowId, controlId);
  msg.SetLabel(label);
  pAddonWindow->OnMessage(msg);
}

void CAddonCallbacksGUI::Control_Spin_SetVisible(void *addonData, GUIHANDLE spinhandle, bool yesNo)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetVisible(yesNo);
}

void CAddonCallbacksGUI::Control_Spin_SetText(void *addonData, GUIHANDLE spinhandle, const char *label)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetText(label);
}

void CAddonCallbacksGUI::Control_Spin_Clear(void *addonData, GUIHANDLE spinhandle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->Clear();
}

void CAddonCallbacksGUI::Control_Spin_AddLabel(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->AddLabel(label, iValue);
}

int CAddonCallbacksGUI::Control_Spin_GetValue(void *addonData, GUIHANDLE spinhandle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return -1;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  return pSpin->GetValue();
}

void CAddonCallbacksGUI::Control_Spin_SetValue(void *addonData, GUIHANDLE spinhandle, int iValue)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !spinhandle)
    return;

  CGUISpinControlEx *pSpin = (CGUISpinControlEx*)spinhandle;
  pSpin->SetValue(iValue);
}

void CAddonCallbacksGUI::Control_RadioButton_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetVisible(yesNo);
}

void CAddonCallbacksGUI::Control_RadioButton_SetText(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetLabel(label);
}

void CAddonCallbacksGUI::Control_RadioButton_SetSelected(void *addonData, GUIHANDLE handle, bool yesNo)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  pRadioButton->SetSelected(yesNo);
}

bool CAddonCallbacksGUI::Control_RadioButton_IsSelected(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return false;

  CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl*)handle;
  return pRadioButton->IsSelected();
}

void CAddonCallbacksGUI::Control_Progress_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  pControl->SetPercentage(fPercent);
}

float CAddonCallbacksGUI::Control_Progress_GetPercentage(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return 0.0;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  return pControl->GetPercentage();
}

void CAddonCallbacksGUI::Control_Progress_SetInfo(void *addonData, GUIHANDLE handle, int iInfo)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  pControl->SetInfo(iInfo);
}

int CAddonCallbacksGUI::Control_Progress_GetInfo(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return -1;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  return pControl->GetInfo();
}

const char* CAddonCallbacksGUI::Control_Progress_GetDescription(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  CGUIProgressControl *pControl = (CGUIProgressControl*)handle;
  CStdString string = pControl->GetDescription();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

GUIHANDLE CAddonCallbacksGUI::ListItem_Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  CStdString string = ((CFileItem*)handle)->GetLabel();
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetLabel(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetLabel(label);
}

const char* CAddonCallbacksGUI::ListItem_GetLabel2(void *addonData, GUIHANDLE handle)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  CStdString string = ((CFileItem*)handle)->GetLabel2();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetLabel2(void *addonData, GUIHANDLE handle, const char *label)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetLabel2(label);
}

void CAddonCallbacksGUI::ListItem_SetIconImage(void *addonData, GUIHANDLE handle, const char *image)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetIconImage(image);
}

void CAddonCallbacksGUI::ListItem_SetThumbnailImage(void *addonData, GUIHANDLE handle, const char *image)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetArt("thumb", image);
}

void CAddonCallbacksGUI::ListItem_SetInfo(void *addonData, GUIHANDLE handle, const char *info)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

}

void CAddonCallbacksGUI::ListItem_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetProperty(key, value);
}

const char* CAddonCallbacksGUI::ListItem_GetProperty(void *addonData, GUIHANDLE handle, const char *key)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return NULL;

  string string = ((CFileItem*)handle)->GetProperty(key).asString();
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

void CAddonCallbacksGUI::ListItem_SetPath(void *addonData, GUIHANDLE handle, const char *path)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || !handle)
    return;

  ((CFileItem*)handle)->SetPath(path);
}







CGUIAddonWindow::CGUIAddonWindow(int id, CStdString strXML, CAddon* addon)
 : CGUIMediaWindow(id, strXML)
 , m_iWindowId(id)
 , m_iOldWindowId(0)
 , m_bModal(false)
 , m_bIsDialog(false)
 , m_actionEvent(true)
 , m_addon(addon)
{
  m_loadType = LOAD_ON_GUI_INIT;
  CBOnInit        = NULL;
  CBOnFocus       = NULL;
  CBOnClick       = NULL;
  CBOnAction      = NULL;
}

CGUIAddonWindow::~CGUIAddonWindow(void)
{
}

bool CGUIAddonWindow::OnAction(const CAction &action)
{
  // do the base class window first, and the call to python after this
  bool ret = CGUIWindow::OnAction(action);  // we don't currently want the mediawindow actions here
  if (CBOnAction)
  {
    CBOnAction(m_clientHandle, action.GetID());
  }
  return ret;
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
        CBOnInit(m_clientHandle);

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
        CBOnFocus(m_clientHandle, iControl);
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
            CBOnClick(m_clientHandle, iControl);
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
          return true;
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIAddonWindow::AllocResources(bool forceLoad /*= FALSE */)
{
  CStdString tmpDir;
  URIUtils::GetDirectory(GetProperty("xmlfile").asString(), tmpDir);
  CStdString fallbackMediaPath;
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
  // Unload temporary language strings
  ClearAddonStrings();

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
    m_vecItems->AddFront(fileItem,itemPosition);
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

void CGUIAddonWindow::ClearAddonStrings()
{
  // Unload temporary language strings
  g_localizeStrings.ClearBlock(m_addon->Path());
}

bool CGUIAddonWindow::OnClick(int iItem)
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


CGUIAddonWindowDialog::CGUIAddonWindowDialog(int id, CStdString strXML, CAddon* addon)
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
  {
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    if (pWindow)
      g_windowManager.ShowOverlay(pWindow->GetOverlayState());
    return CGUIWindow::OnMessage(message);
  }
  return CGUIAddonWindow::OnMessage(message);
}

void CGUIAddonWindowDialog::Show(bool show /* = true */)
{
  unsigned int iCount = g_graphicsContext.exit();
  ThreadMessage tMsg = {TMSG_GUI_ADDON_DIALOG, 1, show ? 1u : 0u};
  tMsg.lpVoid = this;
  CApplicationMessenger::Get().SendMessage(tMsg, true);
  g_graphicsContext.restore(iCount);
}

void CGUIAddonWindowDialog::Show_Internal(bool show /* = true */)
{
  if (show)
  {
    m_bModal = true;
    m_bRunning = true;
    g_windowManager.RouteToWindow(this);

    // active this window...
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowId);
    OnMessage(msg);

    while (m_bRunning && !g_application.m_bStop)
    {
      g_windowManager.Process(CTimeUtils::GetFrameTime());
    }
  }
  else // hide
  {
    m_bRunning = false;

    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
    OnMessage(msg);

    g_windowManager.RemoveDialog(GetID());
  }
}

}; /* namespace ADDON */
