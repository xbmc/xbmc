/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "addons/kodi-addon-dev-kit/include/kodi/gui/Window.h"

#include "Window.h"
#include "General.h"

#include "Application.h"
#include "FileItem.h"
#include "addons/AddonDll.h"
#include "addons/Skin.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace ADDON;
using namespace KODI::MESSAGING;

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::gui; // addon-dev-kit namespace

extern "C"
{
namespace ADDON
{

void Interface_GUIWindow::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->window = static_cast<AddonToKodiFuncTable_kodi_gui_window*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_window)));

  addonInterface->toKodi->kodi_gui->window->create = create;
  addonInterface->toKodi->kodi_gui->window->destroy = destroy;
  addonInterface->toKodi->kodi_gui->window->set_callbacks = set_callbacks;
  addonInterface->toKodi->kodi_gui->window->show = show;
  addonInterface->toKodi->kodi_gui->window->close = close;
  addonInterface->toKodi->kodi_gui->window->do_modal = do_modal;
}

void Interface_GUIWindow::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->window);
}

void* Interface_GUIWindow::create(void* kodiBase, const char* xml_filename,
                                  const char* default_skin, bool as_dialog, bool is_media)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !xml_filename || !default_skin)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (xml_filename='%p', default_skin='%p') on addon '%s'",
                          __FUNCTION__, xml_filename, default_skin, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (as_dialog && is_media)
  {
    CLog::Log(LOGWARNING, "Interface_GUIWindow::%s: %s/%s - addon tries to create dialog as media window who not allowed, contact Developer '%s' of this addon",
                __FUNCTION__, TranslateType(addon->Type()).c_str(), addon->Name().c_str(), addon->Author().c_str());
  }

  RESOLUTION_INFO res;
  std::string strSkinPath = g_SkinInfo->GetSkinPath(xml_filename, &res);

  if (!XFILE::CFile::Exists(strSkinPath))
  {
    std::string str("none");
    ADDON::AddonProps props(str, ADDON::ADDON_SKIN);

    // Check for the matching folder for the skin in the fallback skins folder
    std::string fallbackPath = URIUtils::AddFileToFolder(addon->Path(), "resources", "skins");
    std::string basePath = URIUtils::AddFileToFolder(fallbackPath, g_SkinInfo->ID());

    strSkinPath = g_SkinInfo->GetSkinPath(xml_filename, &res, basePath);

    // Check for the matching folder for the skin in the fallback skins folder (if it exists)
    if (XFILE::CFile::Exists(basePath))
    {
      props.path = basePath;
      ADDON::CSkinInfo skinInfo(props, res);
      skinInfo.Start();
      strSkinPath = skinInfo.GetSkinPath(xml_filename, &res);
    }

    if (!XFILE::CFile::Exists(strSkinPath))
    {
      // Finally fallback to the DefaultSkin as it didn't exist in either the Kodi Skin folder or the fallback skin folder
      props.path = URIUtils::AddFileToFolder(fallbackPath, default_skin);
      ADDON::CSkinInfo skinInfo(props, res);

      skinInfo.Start();
      strSkinPath = skinInfo.GetSkinPath(xml_filename, &res);
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        CLog::Log(LOGERROR, "Interface_GUIWindow::%s: %s/%s - XML File '%s' for Window is missing, contact Developer '%s' of this addon",
                    __FUNCTION__, TranslateType(addon->Type()).c_str(), addon->Name().c_str(), strSkinPath.c_str(), addon->Author().c_str());
        return nullptr;
      }
    }
  }

  int id = GetNextAvailableWindowId();
  if (id < 0)
    return nullptr;

  CGUIWindow *window;
  if (!as_dialog)
    window = new CGUIAddonWindow(id, strSkinPath, addon, is_media);
  else
    window = new CGUIAddonWindowDialog(id, strSkinPath, addon);

  Interface_GUIGeneral::lock();
  g_windowManager.Add(window);
  Interface_GUIGeneral::unlock();

  if (!dynamic_cast<CGUIWindow*>(g_windowManager.GetWindow(id)))
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - Requested window id '%i' does not exist for addon '%s'",
                          __FUNCTION__, id, addon->ID().c_str());
    delete window;
    return nullptr;
  }
  window->SetCoordsRes(res);
  return window;
}

void Interface_GUIWindow::destroy(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (handle='%p') on addon '%s'",
                          __FUNCTION__, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  CGUIWindow *pWindow = dynamic_cast<CGUIWindow*>(g_windowManager.GetWindow(pAddonWindow->GetID()));
  if (pWindow)
  {
    // first change to an existing window
    if (g_windowManager.GetActiveWindow() == pAddonWindow->GetID() && !g_application.m_bStop)
    {
      if(g_windowManager.GetWindow(pAddonWindow->m_oldWindowId))
        g_windowManager.ActivateWindow(pAddonWindow->m_oldWindowId);
      else // old window does not exist anymore, switch to home
        g_windowManager.ActivateWindow(WINDOW_HOME);
    }
    // Free any window properties
    pAddonWindow->ClearProperties();
    // free the window's resources and unload it (free all guicontrols)
    pAddonWindow->FreeResources(true);

    g_windowManager.Remove(pAddonWindow->GetID());
  }
  delete pAddonWindow;
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_callbacks(void* kodiBase, void* handle, void* clienthandle,
                                        bool (*CBOnInit)(void*),
                                        bool (*CBOnFocus)(void*, int),
                                        bool (*CBOnClick)(void*, int),
                                        bool (*CBOnAction)(void*, int),
                                        void (*CBGetContextButtons)(void* , int, gui_context_menu_pair*, unsigned int*),
                                        bool (*CBOnContextButton)(void*, int, unsigned int))
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !clienthandle)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (handle='%p', clienthandle='%p') on addon '%s'",
                          __FUNCTION__, handle, clienthandle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->m_clientHandle = clienthandle;
  pAddonWindow->CBOnInit = CBOnInit;
  pAddonWindow->CBOnClick = CBOnClick;
  pAddonWindow->CBOnFocus = CBOnFocus;
  pAddonWindow->CBOnAction = CBOnAction;
  pAddonWindow->CBGetContextButtons = CBGetContextButtons;
  pAddonWindow->CBOnContextButton = CBOnContextButton;
  Interface_GUIGeneral::unlock();
}

bool Interface_GUIWindow::show(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (handle='%p') on addon '%s'",
                          __FUNCTION__, handle, addon ? addon->ID().c_str() : "unknown");
    return false;
  }

  if (pAddonWindow->m_oldWindowId != pAddonWindow->m_windowId && 
      pAddonWindow->m_windowId != g_windowManager.GetActiveWindow())
    pAddonWindow->m_oldWindowId = g_windowManager.GetActiveWindow();

  Interface_GUIGeneral::lock();
  if (pAddonWindow->IsDialog())
    dynamic_cast<CGUIAddonWindowDialog*>(pAddonWindow)->Show();
  else
    g_windowManager.ActivateWindow(pAddonWindow->GetID());
  Interface_GUIGeneral::unlock();
  
  return true;
}

bool Interface_GUIWindow::close(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (handle='%p') on addon '%s'",
                          __FUNCTION__, handle, addon ? addon->ID().c_str() : "unknown");
    return false;
  }

  pAddonWindow->PulseActionEvent();

  Interface_GUIGeneral::lock();

  // if it's a dialog, we have to close it a bit different
  if (pAddonWindow->IsDialog())
    dynamic_cast<CGUIAddonWindowDialog*>(pAddonWindow)->Show(false);
  else
    g_windowManager.ActivateWindow(pAddonWindow->m_oldWindowId);
  pAddonWindow->m_oldWindowId = 0;

  Interface_GUIGeneral::unlock();

  return true;
}

bool Interface_GUIWindow::do_modal(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - invalid handler data (handle='%p') on addon '%s'",
                          __FUNCTION__, handle, addon ? addon->ID().c_str() : "unknown");
    return false;
  }

  if (pAddonWindow->GetID() != g_windowManager.GetActiveWindow())
    show(kodiBase, handle);

  return true;
}

int Interface_GUIWindow::GetNextAvailableWindowId()
{
  Interface_GUIGeneral::lock();

  // if window WINDOW_ADDON_END is in use it means addon can't create more windows
  if (g_windowManager.GetWindow(WINDOW_ADDON_END))
  {
    Interface_GUIGeneral::unlock();
    CLog::Log(LOGERROR, "Interface_GUIWindow::%s - Maximum number of windows for binary addons reached", __FUNCTION__);
    return -1;
  }

  // window id's WINDOW_ADDON_START - WINDOW_ADDON_END are reserved for addons
  // get first window id that is not in use
  int id = WINDOW_ADDON_START;
  while (id < WINDOW_ADDON_END && g_windowManager.GetWindow(id) != nullptr)
    id++;

  Interface_GUIGeneral::unlock();
  return id;
}

CGUIAddonWindow::CGUIAddonWindow(int id, const std::string& strXML, CAddonDll* addon, bool isMedia)
 : CGUIMediaWindow(id, strXML.c_str()),
   m_clientHandle{nullptr},
   CBOnInit{nullptr},
   CBOnFocus{nullptr},
   CBOnClick{nullptr},
   CBOnAction{nullptr},
   CBGetContextButtons{nullptr},
   CBOnContextButton{nullptr},
   m_windowId(id),
   m_oldWindowId(0),
   m_actionEvent(true),
   m_addon(addon),
   m_isMedia(isMedia)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIControl* CGUIAddonWindow::GetAddonControl(int controlId, CGUIControl::GUICONTROLTYPES type, std::string typeName)
{
  CGUIControl* pGUIControl = dynamic_cast<CGUIControl*>(GetControl(controlId));
  if (!pGUIControl)
  {
    CLog::Log(LOGERROR, "CGUIAddonGUI_Window::%s: %s - Requested GUI control Id '%i' for '%s' not present!",
                __FUNCTION__,
                m_addon->Name().c_str(),
                controlId, typeName.c_str());
    return nullptr;
  }
  else if (pGUIControl->GetControlType() != type)
  {
    CLog::Log(LOGERROR, "CGUIAddonGUI_Window::%s: %s - Requested GUI control Id '%i' not the type '%s'!",
                __FUNCTION__,
                m_addon->Name().c_str(),
                controlId, typeName.c_str());
    return nullptr;
  }

  return pGUIControl;
}

bool CGUIAddonWindow::OnAction(const CAction& action)
{
  // Let addon decide whether it wants to handle action first
  if (CBOnAction && CBOnAction(m_clientHandle, action.GetID()))
    return true;

  return CGUIWindow::OnAction(action);
}

bool CGUIAddonWindow::OnMessage(CGUIMessage& message)
{
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

    case GUI_MSG_FOCUSED:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && 
          m_viewControl.GetCurrentControl() != (int)message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
      // check if our focused control is one of our category buttons
      int iControl = message.GetControlId();
      if (CBOnFocus)
        CBOnFocus(m_clientHandle, iControl);
    }
    break;

    case GUI_MSG_NOTIFY_ALL:
    {
      // most messages from GUI_MSG_NOTIFY_ALL break container content, whitelist working ones.
      if (message.GetParam1() == GUI_MSG_PAGE_CHANGE || message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
        return CGUIMediaWindow::OnMessage(message);
      return true;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl && iControl != static_cast<int>(this->GetID()))
      {
        CGUIControl* controlClicked = dynamic_cast<CGUIControl*>(this->GetControl(iControl));

        // The old python way used to check list AND SELECITEM method or if its a button, checkmark.
        // Its done this way for now to allow other controls without a python version like togglebutton to still raise a onAction event
        if (controlClicked) // Will get problems if we the id is not on the window and we try to do GetControlType on it. So check to make sure it exists
        {
          if ((controlClicked->IsContainer() && (message.GetParam1() == ACTION_SELECT_ITEM ||
                                                 message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) ||
                                                 !controlClicked->IsContainer())
          {
            if (CBOnClick)
              return CBOnClick(m_clientHandle, iControl);
          }
          else if (controlClicked->IsContainer() && (message.GetParam1() == ACTION_MOUSE_RIGHT_CLICK ||
                                                     message.GetParam1() == ACTION_CONTEXT_MENU))
          {
            if (CBOnAction)
            {
              // Check addon want to handle right click for a context menu, if
              // not used from addon becomes "GetContextButtons(...)" called.
              if (CBOnAction(m_clientHandle, ACTION_CONTEXT_MENU))
                return true;
            }
          }
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIAddonWindow::AllocResources(bool forceLoad /*= false */)
{
  std::string tmpDir = URIUtils::GetDirectory(GetProperty("xmlfile").asString());
  std::string fallbackMediaPath;
  URIUtils::GetParentPath(tmpDir, fallbackMediaPath);
  URIUtils::RemoveSlashAtEnd(fallbackMediaPath);
  m_mediaDir = fallbackMediaPath;

  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::AllocResources(forceLoad);
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::Render()
{
  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::Render();
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::AddItem(CFileItemPtr* fileItem, int itemPosition)
{
  if (itemPosition == -1 || itemPosition > m_vecItems->Size())
  {
    m_vecItems->Add(*fileItem);
  }
  else if (itemPosition <  -1 && !(itemPosition-1 < m_vecItems->Size()))
  {
    m_vecItems->AddFront(*fileItem, 0);
  }
  else
  {
    m_vecItems->AddFront(*fileItem, itemPosition);
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

void CGUIAddonWindow::RemoveItem(CFileItemPtr* fileItem)
{
  m_vecItems->Remove(fileItem->get());
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

CFileItemPtr* CGUIAddonWindow::GetListItem(int position)
{
  if (position < 0 || position >= m_vecItems->Size())
    return nullptr;
  return new CFileItemPtr(m_vecItems->Get(position));
}

void CGUIAddonWindow::ClearList()
{
  ClearFileItems();

  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIAddonWindow::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  gui_context_menu_pair c_buttons[ADDON_MAX_CONTEXT_ENTRIES] = {0};
  unsigned int size = ADDON_MAX_CONTEXT_ENTRIES;
  if (CBGetContextButtons)
  {
    CBGetContextButtons(m_clientHandle, itemNumber, c_buttons, &size);
    for (unsigned int i = 0; i < size; ++i)
      buttons.push_back(std::pair<unsigned int, std::string>(c_buttons[i].id, c_buttons[i].name));
  }
}

bool CGUIAddonWindow::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (CBOnContextButton)
    return CBOnContextButton(m_clientHandle, itemNumber, static_cast<unsigned int>(button));
  return false;
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

void CGUIAddonWindow::SetupShares()
{
  UpdateButtons();
}


CGUIAddonWindowDialog::CGUIAddonWindowDialog(int id, const std::string& strXML, CAddonDll* addon)
  : CGUIAddonWindow(id, strXML, addon, false),
    m_bRunning(false)
{
}

void CGUIAddonWindowDialog::Show(bool show /* = true */)
{
  unsigned int count = g_graphicsContext.exit();
  CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ADDON_DIALOG, 0, show ? 1 : 0, static_cast<void*>(this));
  g_graphicsContext.restore(count);
}

void CGUIAddonWindowDialog::Show_Internal(bool show /* = true */)
{
  if (show)
  {
    m_bRunning = true;
    g_windowManager.RegisterDialog(this);

    // activate this window...
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetID());
    OnMessage(msg);

    // this dialog is derived from GUIMediaWindow
    // make sure it is rendered last
    m_renderOrder = RENDER_ORDER_DIALOG;
    while (m_bRunning && !g_application.m_bStop)
    {
      ProcessRenderLoop();
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

} /* namespace ADDON */
} /* extern "C" */
