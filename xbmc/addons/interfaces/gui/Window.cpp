/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/kodi-dev-kit/include/kodi/gui/Window.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUITranslator.h"
#include "General.h"
#include "ServiceBroker.h"
#include "Window.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/AddonDll.h"
#include "application/Application.h"
#include "controls/Rendering.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIRenderingControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIWindow::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->window = new AddonToKodiFuncTable_kodi_gui_window();

  /* Window creation functions */
  addonInterface->toKodi->kodi_gui->window->create = create;
  addonInterface->toKodi->kodi_gui->window->destroy = destroy;
  addonInterface->toKodi->kodi_gui->window->set_callbacks = set_callbacks;
  addonInterface->toKodi->kodi_gui->window->show = show;
  addonInterface->toKodi->kodi_gui->window->close = close;
  addonInterface->toKodi->kodi_gui->window->do_modal = do_modal;

  /* Window control functions */
  addonInterface->toKodi->kodi_gui->window->set_focus_id = set_focus_id;
  addonInterface->toKodi->kodi_gui->window->get_focus_id = get_focus_id;
  addonInterface->toKodi->kodi_gui->window->set_control_label = set_control_label;
  addonInterface->toKodi->kodi_gui->window->set_control_visible = set_control_visible;
  addonInterface->toKodi->kodi_gui->window->set_control_selected = set_control_selected;

  /* Window property functions */
  addonInterface->toKodi->kodi_gui->window->set_property = set_property;
  addonInterface->toKodi->kodi_gui->window->set_property_int = set_property_int;
  addonInterface->toKodi->kodi_gui->window->set_property_bool = set_property_bool;
  addonInterface->toKodi->kodi_gui->window->set_property_double = set_property_double;
  addonInterface->toKodi->kodi_gui->window->get_property = get_property;
  addonInterface->toKodi->kodi_gui->window->get_property_int = get_property_int;
  addonInterface->toKodi->kodi_gui->window->get_property_bool = get_property_bool;
  addonInterface->toKodi->kodi_gui->window->get_property_double = get_property_double;
  addonInterface->toKodi->kodi_gui->window->clear_properties = clear_properties;
  addonInterface->toKodi->kodi_gui->window->clear_property = clear_property;

  /* List item functions */
  addonInterface->toKodi->kodi_gui->window->clear_item_list = clear_item_list;
  addonInterface->toKodi->kodi_gui->window->add_list_item = add_list_item;
  addonInterface->toKodi->kodi_gui->window->remove_list_item_from_position =
      remove_list_item_from_position;
  addonInterface->toKodi->kodi_gui->window->remove_list_item = remove_list_item;
  addonInterface->toKodi->kodi_gui->window->get_list_item = get_list_item;
  addonInterface->toKodi->kodi_gui->window->set_current_list_position = set_current_list_position;
  addonInterface->toKodi->kodi_gui->window->get_current_list_position = get_current_list_position;
  addonInterface->toKodi->kodi_gui->window->get_list_size = get_list_size;
  addonInterface->toKodi->kodi_gui->window->set_container_property = set_container_property;
  addonInterface->toKodi->kodi_gui->window->set_container_content = set_container_content;
  addonInterface->toKodi->kodi_gui->window->get_current_container_id = get_current_container_id;

  /* Various functions */
  addonInterface->toKodi->kodi_gui->window->mark_dirty_region = mark_dirty_region;

  /* GUI control access functions */
  addonInterface->toKodi->kodi_gui->window->get_control_button = get_control_button;
  addonInterface->toKodi->kodi_gui->window->get_control_edit = get_control_edit;
  addonInterface->toKodi->kodi_gui->window->get_control_fade_label = get_control_fade_label;
  addonInterface->toKodi->kodi_gui->window->get_control_image = get_control_image;
  addonInterface->toKodi->kodi_gui->window->get_control_label = get_control_label;
  addonInterface->toKodi->kodi_gui->window->get_control_progress = get_control_progress;
  addonInterface->toKodi->kodi_gui->window->get_control_radio_button = get_control_radio_button;
  addonInterface->toKodi->kodi_gui->window->get_control_render_addon = get_control_render_addon;
  addonInterface->toKodi->kodi_gui->window->get_control_settings_slider =
      get_control_settings_slider;
  addonInterface->toKodi->kodi_gui->window->get_control_slider = get_control_slider;
  addonInterface->toKodi->kodi_gui->window->get_control_spin = get_control_spin;
  addonInterface->toKodi->kodi_gui->window->get_control_text_box = get_control_text_box;
}

void Interface_GUIWindow::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->window;
}

/*!
 * Window creation functions
 */
//@{
KODI_GUI_WINDOW_HANDLE Interface_GUIWindow::create(KODI_HANDLE kodiBase,
                                                   const char* xml_filename,
                                                   const char* default_skin,
                                                   bool as_dialog,
                                                   bool is_media)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !xml_filename || !default_skin)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (xml_filename='{}', "
              "default_skin='{}') on addon '{}'",
              __func__, static_cast<const void*>(xml_filename),
              static_cast<const void*>(default_skin), addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (as_dialog && is_media)
  {
    CLog::Log(LOGWARNING,
              "Interface_GUIWindow::{}: {}/{} - addon tries to create dialog as media window who "
              "not allowed, contact Developer '{}' of this addon",
              __func__, CAddonInfo::TranslateType(addon->Type()), addon->Name(), addon->Author());
  }

  RESOLUTION_INFO res;
  std::string strSkinPath = g_SkinInfo->GetSkinPath(xml_filename, &res);

  if (!CFileUtils::Exists(strSkinPath))
  {
    std::string str("none");
    ADDON::AddonInfoPtr addonInfo =
        std::make_shared<ADDON::CAddonInfo>(str, ADDON::AddonType::SKIN);

    // Check for the matching folder for the skin in the fallback skins folder
    std::string fallbackPath = URIUtils::AddFileToFolder(addon->Path(), "resources", "skins");
    std::string basePath = URIUtils::AddFileToFolder(fallbackPath, g_SkinInfo->ID());

    strSkinPath = g_SkinInfo->GetSkinPath(xml_filename, &res, basePath);

    // Check for the matching folder for the skin in the fallback skins folder (if it exists)
    if (CFileUtils::Exists(basePath))
    {
      addonInfo->SetPath(basePath);
      const std::shared_ptr<ADDON::CSkinInfo> skinInfo =
          std::make_shared<ADDON::CSkinInfo>(addonInfo, res);
      skinInfo->Start();
      strSkinPath = skinInfo->GetSkinPath(xml_filename, &res);
    }

    if (!CFileUtils::Exists(strSkinPath))
    {
      // Finally fallback to the DefaultSkin as it didn't exist in either the Kodi Skin folder or the fallback skin folder
      addonInfo->SetPath(URIUtils::AddFileToFolder(fallbackPath, default_skin));
      const std::shared_ptr<ADDON::CSkinInfo> skinInfo =
          std::make_shared<ADDON::CSkinInfo>(addonInfo, res);

      skinInfo->Start();
      strSkinPath = skinInfo->GetSkinPath(xml_filename, &res);
      if (!CFileUtils::Exists(strSkinPath))
      {
        CLog::Log(LOGERROR,
                  "Interface_GUIWindow::{}: {}/{} - XML File '{}' for Window is missing, contact "
                  "Developer '{}' of this addon",
                  __func__, CAddonInfo::TranslateType(addon->Type()), addon->Name(), strSkinPath,
                  addon->Author());
        return nullptr;
      }
    }
  }

  int id = GetNextAvailableWindowId();
  if (id < 0)
    return nullptr;

  CGUIWindow* window;
  if (!as_dialog)
    window = new CGUIAddonWindow(id, strSkinPath, addon, is_media);
  else
    window = new CGUIAddonWindowDialog(id, strSkinPath, addon);

  Interface_GUIGeneral::lock();
  CServiceBroker::GetGUI()->GetWindowManager().Add(window);
  Interface_GUIGeneral::unlock();

  if (!CServiceBroker::GetGUI()->GetWindowManager().GetWindow(id))
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - Requested window id '{}' does not exist for addon '{}'",
              __func__, id, addon->ID());
    delete window;
    return nullptr;
  }
  window->SetCoordsRes(res);
  return window;
}

void Interface_GUIWindow::destroy(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (handle='{}') on addon '{}'",
              __func__, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  CGUIWindow* pWindow =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow(pAddonWindow->GetID());
  if (pWindow)
  {
    // first change to an existing window
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == pAddonWindow->GetID() &&
        !g_application.m_bStop)
    {
      if (CServiceBroker::GetGUI()->GetWindowManager().GetWindow(pAddonWindow->m_oldWindowId))
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(pAddonWindow->m_oldWindowId);
      else // old window does not exist anymore, switch to home
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
    }
    // Free any window properties
    pAddonWindow->ClearProperties();
    // free the window's resources and unload it (free all guicontrols)
    pAddonWindow->FreeResources(true);

    CServiceBroker::GetGUI()->GetWindowManager().Remove(pAddonWindow->GetID());
  }
  delete pAddonWindow;
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_callbacks(
    KODI_HANDLE kodiBase,
    KODI_GUI_WINDOW_HANDLE handle,
    KODI_GUI_CLIENT_HANDLE clienthandle,
    bool (*CBOnInit)(KODI_GUI_CLIENT_HANDLE),
    bool (*CBOnFocus)(KODI_GUI_CLIENT_HANDLE, int),
    bool (*CBOnClick)(KODI_GUI_CLIENT_HANDLE, int),
    bool (*CBOnAction)(KODI_GUI_CLIENT_HANDLE, ADDON_ACTION),
    void (*CBGetContextButtons)(KODI_GUI_CLIENT_HANDLE, int, gui_context_menu_pair*, unsigned int*),
    bool (*CBOnContextButton)(KODI_GUI_CLIENT_HANDLE, int, unsigned int))
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !clienthandle)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (handle='{}', clienthandle='{}') "
              "on addon '{}'",
              __func__, handle, clienthandle, addon ? addon->ID() : "unknown");
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

bool Interface_GUIWindow::show(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (handle='{}') on addon '{}'",
              __func__, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  if (pAddonWindow->m_oldWindowId != pAddonWindow->m_windowId &&
      pAddonWindow->m_windowId != CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow())
    pAddonWindow->m_oldWindowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();

  Interface_GUIGeneral::lock();
  if (pAddonWindow->IsDialog())
    dynamic_cast<CGUIAddonWindowDialog*>(pAddonWindow)->Show(true, false);
  else
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(pAddonWindow->GetID());
  Interface_GUIGeneral::unlock();

  return true;
}

bool Interface_GUIWindow::close(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (handle='{}') on addon '{}'",
              __func__, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  pAddonWindow->PulseActionEvent();

  Interface_GUIGeneral::lock();

  // if it's a dialog, we have to close it a bit different
  if (pAddonWindow->IsDialog())
    dynamic_cast<CGUIAddonWindowDialog*>(pAddonWindow)->Show(false);
  else
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(pAddonWindow->m_oldWindowId);
  pAddonWindow->m_oldWindowId = 0;

  Interface_GUIGeneral::unlock();

  return true;
}

bool Interface_GUIWindow::do_modal(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (handle='{}') on addon '{}'",
              __func__, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  if (pAddonWindow->GetID() == CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow())
    return true;

  if (pAddonWindow->m_oldWindowId != pAddonWindow->m_windowId &&
      pAddonWindow->m_windowId != CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow())
    pAddonWindow->m_oldWindowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();

  Interface_GUIGeneral::lock();
  if (pAddonWindow->IsDialog())
    dynamic_cast<CGUIAddonWindowDialog*>(pAddonWindow)->Show(true, true);
  else
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(pAddonWindow->GetID());
  Interface_GUIGeneral::unlock();

  return true;
}
//@}

/*!
 * Window control functions
 */
//@{
bool Interface_GUIWindow::set_focus_id(KODI_HANDLE kodiBase,
                                       KODI_GUI_WINDOW_HANDLE handle,
                                       int control_id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  if (!pAddonWindow->GetControl(control_id))
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow - {}: {} - Control does not exist in window", __func__,
              addon->Name());
    return false;
  }

  Interface_GUIGeneral::lock();
  CGUIMessage msg(GUI_MSG_SETFOCUS, pAddonWindow->m_windowId, control_id);
  pAddonWindow->OnMessage(msg);
  Interface_GUIGeneral::unlock();

  return true;
}

int Interface_GUIWindow::get_focus_id(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  Interface_GUIGeneral::lock();
  int control_id = pAddonWindow->GetFocusedControlID();
  Interface_GUIGeneral::unlock();

  if (control_id == -1)
    CLog::Log(LOGERROR, "Interface_GUIWindow - {}: {} - No control in this window has focus",
              __func__, addon->Name());

  return control_id;
}

void Interface_GUIWindow::set_control_label(KODI_HANDLE kodiBase,
                                            KODI_GUI_WINDOW_HANDLE handle,
                                            int control_id,
                                            const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  CGUIMessage msg(GUI_MSG_LABEL_SET, pAddonWindow->m_windowId, control_id);
  msg.SetLabel(label);
  pAddonWindow->OnMessage(msg);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_control_visible(KODI_HANDLE kodiBase,
                                              KODI_GUI_WINDOW_HANDLE handle,
                                              int control_id,
                                              bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  CGUIMessage msg(visible ? GUI_MSG_VISIBLE : GUI_MSG_HIDDEN, pAddonWindow->m_windowId, control_id);
  pAddonWindow->OnMessage(msg);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_control_selected(KODI_HANDLE kodiBase,
                                               KODI_GUI_WINDOW_HANDLE handle,
                                               int control_id,
                                               bool selected)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  CGUIMessage msg(selected ? GUI_MSG_SET_SELECTED : GUI_MSG_SET_DESELECTED,
                  pAddonWindow->m_windowId, control_id);
  pAddonWindow->OnMessage(msg);
  Interface_GUIGeneral::unlock();
}
//@}

/*!
 * Window property functions
 */
//@{
void Interface_GUIWindow::set_property(KODI_HANDLE kodiBase,
                                       KODI_GUI_WINDOW_HANDLE handle,
                                       const char* key,
                                       const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}', value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              static_cast<const void*>(value), addon ? addon->ID() : "unknown");
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  pAddonWindow->SetProperty(lowerKey, value);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_property_int(KODI_HANDLE kodiBase,
                                           KODI_GUI_WINDOW_HANDLE handle,
                                           const char* key,
                                           int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  pAddonWindow->SetProperty(lowerKey, value);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_property_bool(KODI_HANDLE kodiBase,
                                            KODI_GUI_WINDOW_HANDLE handle,
                                            const char* key,
                                            bool value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  pAddonWindow->SetProperty(lowerKey, value);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_property_double(KODI_HANDLE kodiBase,
                                              KODI_GUI_WINDOW_HANDLE handle,
                                              const char* key,
                                              double value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  pAddonWindow->SetProperty(lowerKey, value);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIWindow::get_property(KODI_HANDLE kodiBase,
                                        KODI_GUI_WINDOW_HANDLE handle,
                                        const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return nullptr;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  std::string value = pAddonWindow->GetProperty(lowerKey).asString();
  Interface_GUIGeneral::unlock();

  return strdup(value.c_str());
}

int Interface_GUIWindow::get_property_int(KODI_HANDLE kodiBase,
                                          KODI_GUI_WINDOW_HANDLE handle,
                                          const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return -1;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  int value = static_cast<int>(pAddonWindow->GetProperty(lowerKey).asInteger());
  Interface_GUIGeneral::unlock();

  return value;
}

bool Interface_GUIWindow::get_property_bool(KODI_HANDLE kodiBase,
                                            KODI_GUI_WINDOW_HANDLE handle,
                                            const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return false;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  bool value = pAddonWindow->GetProperty(lowerKey).asBoolean();
  Interface_GUIGeneral::unlock();

  return value;
}

double Interface_GUIWindow::get_property_double(KODI_HANDLE kodiBase,
                                                KODI_GUI_WINDOW_HANDLE handle,
                                                const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return 0.0;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  double value = pAddonWindow->GetProperty(lowerKey).asDouble();
  Interface_GUIGeneral::unlock();

  return value;
}

void Interface_GUIWindow::clear_properties(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->ClearProperties();
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::clear_property(KODI_HANDLE kodiBase,
                                         KODI_GUI_WINDOW_HANDLE handle,
                                         const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  pAddonWindow->SetProperty(lowerKey, "");
  Interface_GUIGeneral::unlock();
}
//@}

/*!
 * List item functions
 */
//@{
void Interface_GUIWindow::clear_item_list(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->ClearList();
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::add_list_item(KODI_HANDLE kodiBase,
                                        KODI_GUI_WINDOW_HANDLE handle,
                                        KODI_GUI_LISTITEM_HANDLE item,
                                        int list_position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "item='{}') on addon '{}'",
              __func__, kodiBase, handle, item, addon ? addon->ID() : "unknown");
    return;
  }

  CFileItemPtr* pItem(static_cast<CFileItemPtr*>(item));
  if (pItem->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::{} - empty list item called on addon '{}'", __func__,
              addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->AddItem(pItem, list_position);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::remove_list_item_from_position(KODI_HANDLE kodiBase,
                                                         KODI_GUI_WINDOW_HANDLE handle,
                                                         int list_position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->RemoveItem(list_position);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::remove_list_item(KODI_HANDLE kodiBase,
                                           KODI_GUI_WINDOW_HANDLE handle,
                                           KODI_GUI_LISTITEM_HANDLE item)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "item='{}') on addon '{}'",
              __func__, kodiBase, handle, item, addon ? addon->ID() : "unknown");
    return;
  }

  CFileItemPtr* pItem(static_cast<CFileItemPtr*>(item));
  if (pItem->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIWindow::{} - empty list item called on addon '{}'", __func__,
              addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->RemoveItem(pItem);
  Interface_GUIGeneral::unlock();
}

KODI_GUI_LISTITEM_HANDLE Interface_GUIWindow::get_list_item(KODI_HANDLE kodiBase,
                                                            KODI_GUI_WINDOW_HANDLE handle,
                                                            int list_position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  Interface_GUIGeneral::lock();
  CFileItemPtr* pItem(pAddonWindow->GetListItem(list_position));
  if (pItem == nullptr || pItem->get() == nullptr)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIWindow - {}: {} - Index out of range", __func__,
              addon->Name());

    if (pItem)
    {
      delete pItem;
      pItem = nullptr;
    }
  }
  Interface_GUIGeneral::unlock();

  return pItem;
}

void Interface_GUIWindow::set_current_list_position(KODI_HANDLE kodiBase,
                                                    KODI_GUI_WINDOW_HANDLE handle,
                                                    int list_position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->SetCurrentListPosition(list_position);
  Interface_GUIGeneral::unlock();
}

int Interface_GUIWindow::get_current_list_position(KODI_HANDLE kodiBase,
                                                   KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  Interface_GUIGeneral::lock();
  int listPos = pAddonWindow->GetCurrentListPosition();
  Interface_GUIGeneral::unlock();

  return listPos;
}

int Interface_GUIWindow::get_list_size(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  Interface_GUIGeneral::lock();
  int listSize = pAddonWindow->GetListSize();
  Interface_GUIGeneral::unlock();

  return listSize;
}

void Interface_GUIWindow::set_container_property(KODI_HANDLE kodiBase,
                                                 KODI_GUI_WINDOW_HANDLE handle,
                                                 const char* key,
                                                 const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !key || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}', value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              static_cast<const void*>(value), addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->SetContainerProperty(key, value);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIWindow::set_container_content(KODI_HANDLE kodiBase,
                                                KODI_GUI_WINDOW_HANDLE handle,
                                                const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(value),
              addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->SetContainerContent(value);
  Interface_GUIGeneral::unlock();
}

int Interface_GUIWindow::get_current_container_id(KODI_HANDLE kodiBase,
                                                  KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  Interface_GUIGeneral::lock();
  int id = pAddonWindow->GetCurrentContainerControlId();
  Interface_GUIGeneral::unlock();

  return id;
}
//@}

/*!
 * Various functions
 */
//@{
void Interface_GUIWindow::mark_dirty_region(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  Interface_GUIGeneral::lock();
  pAddonWindow->MarkDirtyRegion();
  Interface_GUIGeneral::unlock();
}
//@}

/*!
 * GUI control access functions
 */
//@{
KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_button(KODI_HANDLE kodiBase,
                                                                KODI_GUI_WINDOW_HANDLE handle,
                                                                int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_BUTTON,
                    "button");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_edit(KODI_HANDLE kodiBase,
                                                              KODI_GUI_WINDOW_HANDLE handle,
                                                              int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_EDIT, "edit");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_fade_label(KODI_HANDLE kodiBase,
                                                                    KODI_GUI_WINDOW_HANDLE handle,
                                                                    int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_FADELABEL,
                    "fade label");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_image(KODI_HANDLE kodiBase,
                                                               KODI_GUI_WINDOW_HANDLE handle,
                                                               int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_IMAGE, "image");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_label(KODI_HANDLE kodiBase,
                                                               KODI_GUI_WINDOW_HANDLE handle,
                                                               int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_LABEL, "label");
}
KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_progress(KODI_HANDLE kodiBase,
                                                                  KODI_GUI_WINDOW_HANDLE handle,
                                                                  int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_PROGRESS,
                    "progress");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_radio_button(KODI_HANDLE kodiBase,
                                                                      KODI_GUI_WINDOW_HANDLE handle,
                                                                      int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_RADIO,
                    "radio button");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_render_addon(KODI_HANDLE kodiBase,
                                                                      KODI_GUI_WINDOW_HANDLE handle,
                                                                      int control_id)
{
  CGUIControl* pGUIControl = static_cast<CGUIControl*>(GetControl(
      kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_RENDERADDON, "renderaddon"));
  if (!pGUIControl)
    return nullptr;

  CGUIAddonRenderingControl* pRenderControl =
      new CGUIAddonRenderingControl(dynamic_cast<CGUIRenderingControl*>(pGUIControl));
  return pRenderControl;
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_settings_slider(
    KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_SETTINGS_SLIDER,
                    "settings slider");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_slider(KODI_HANDLE kodiBase,
                                                                KODI_GUI_WINDOW_HANDLE handle,
                                                                int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_SLIDER,
                    "slider");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_spin(KODI_HANDLE kodiBase,
                                                              KODI_GUI_WINDOW_HANDLE handle,
                                                              int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_SPINEX, "spin");
}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::get_control_text_box(KODI_HANDLE kodiBase,
                                                                  KODI_GUI_WINDOW_HANDLE handle,
                                                                  int control_id)
{
  return GetControl(kodiBase, handle, control_id, __func__, CGUIControl::GUICONTROL_TEXTBOX,
                    "textbox");
}
//@}

KODI_GUI_CONTROL_HANDLE Interface_GUIWindow::GetControl(KODI_HANDLE kodiBase,
                                                        KODI_GUI_WINDOW_HANDLE handle,
                                                        int control_id,
                                                        const char* function,
                                                        CGUIControl::GUICONTROLTYPES type,
                                                        const std::string& typeName)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIAddonWindow* pAddonWindow = static_cast<CGUIAddonWindow*>(handle);
  if (!addon || !pAddonWindow)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return pAddonWindow->GetAddonControl(control_id, type, typeName);
}

int Interface_GUIWindow::GetNextAvailableWindowId()
{
  Interface_GUIGeneral::lock();

  // if window WINDOW_ADDON_END is in use it means addon can't create more windows
  if (CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_ADDON_END))
  {
    Interface_GUIGeneral::unlock();
    CLog::Log(LOGERROR,
              "Interface_GUIWindow::{} - Maximum number of windows for binary addons reached",
              __func__);
    return -1;
  }

  // window id's WINDOW_ADDON_START - WINDOW_ADDON_END are reserved for addons
  // get first window id that is not in use
  int id = WINDOW_ADDON_START;
  while (id < WINDOW_ADDON_END &&
         CServiceBroker::GetGUI()->GetWindowManager().GetWindow(id) != nullptr)
    id++;

  Interface_GUIGeneral::unlock();
  return id;
}

CGUIAddonWindow::CGUIAddonWindow(int id, const std::string& strXML, CAddonDll* addon, bool isMedia)
  : CGUIMediaWindow(id, strXML.c_str()),
    m_windowId(id),
    m_actionEvent(true),
    m_addon(addon),
    m_isMedia(isMedia)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIControl* CGUIAddonWindow::GetAddonControl(int controlId,
                                              CGUIControl::GUICONTROLTYPES type,
                                              const std::string& typeName)
{
  // Load window resources, if not already done, to have related xml content
  // present and to let control find it
  if (!m_windowLoaded)
  {
    if (!Initialize())
    {
      CLog::Log(LOGERROR,
                "CGUIAddonGUI_Window::{}: {} - Window initialize failed by control id '{}' request "
                "for '{}'",
                __func__, m_addon->Name(), controlId, typeName);
      return nullptr;
    }
  }

  CGUIControl* pGUIControl = GetControl(controlId);
  if (!pGUIControl)
  {
    CLog::Log(LOGERROR,
              "CGUIAddonGUI_Window::{}: {} - Requested GUI control Id '{}' for '{}' not present!",
              __func__, m_addon->Name(), controlId, typeName);
    return nullptr;
  }
  else if (pGUIControl->GetControlType() != type)
  {
    CLog::Log(LOGERROR,
              "CGUIAddonGUI_Window::{}: {} - Requested GUI control Id '{}' not the type '{}'!",
              __func__, m_addon->Name(), controlId, typeName);
    return nullptr;
  }

  return pGUIControl;
}

bool CGUIAddonWindow::OnAction(const CAction& action)
{
  // Let addon decide whether it wants to handle action first
  if (CBOnAction &&
      CBOnAction(m_clientHandle, CAddonGUITranslator::TranslateActionIdToAddon(action.GetID())))
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
          m_viewControl.GetCurrentControl() != message.GetControlId())
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
      if (message.GetParam1() == GUI_MSG_PAGE_CHANGE ||
          message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
        return CGUIMediaWindow::OnMessage(message);
      return true;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl && iControl != this->GetID())
      {
        CGUIControl* controlClicked = this->GetControl(iControl);

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
          else if (controlClicked->IsContainer() &&
                   (message.GetParam1() == ACTION_MOUSE_RIGHT_CLICK ||
                    message.GetParam1() == ACTION_CONTEXT_MENU))
          {
            if (CBOnAction)
            {
              // Check addon want to handle right click for a context menu, if
              // not used from addon becomes "GetContextButtons(...)" called.
              if (CBOnAction(m_clientHandle, ADDON_ACTION_CONTEXT_MENU))
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

  CServiceBroker::GetGUI()->GetTextureManager().AddTexturePath(m_mediaDir);
  CGUIMediaWindow::AllocResources(forceLoad);
  CServiceBroker::GetGUI()->GetTextureManager().RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::Render()
{
  CServiceBroker::GetGUI()->GetTextureManager().AddTexturePath(m_mediaDir);
  CGUIMediaWindow::Render();
  CServiceBroker::GetGUI()->GetTextureManager().RemoveTexturePath(m_mediaDir);
}

void CGUIAddonWindow::AddItem(CFileItemPtr* fileItem, int itemPosition)
{
  if (itemPosition == -1 || itemPosition > m_vecItems->Size())
  {
    m_vecItems->Add(*fileItem);
  }
  else if (itemPosition < -1 && !(itemPosition - 1 < m_vecItems->Size()))
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

void CGUIAddonWindow::SetContainerProperty(const std::string& key, const std::string& value)
{
  m_vecItems->SetProperty(key, value);
}

void CGUIAddonWindow::SetContainerContent(const std::string& value)
{
  m_vecItems->SetContent(value);
}

int CGUIAddonWindow::GetCurrentContainerControlId()
{
  return m_viewControl.GetCurrentControl();
}

void CGUIAddonWindow::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  gui_context_menu_pair c_buttons[ADDON_MAX_CONTEXT_ENTRIES] = {};
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
  m_actionEvent.Wait(std::chrono::milliseconds(timeout));
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
  : CGUIAddonWindow(id, strXML, addon, false)
{
}

void CGUIAddonWindowDialog::Show(bool show /* = true */, bool modal /* = true*/)
{
  if (modal)
  {
    unsigned int count = CServiceBroker::GetWinSystem()->GetGfxContext().exit();
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ADDON_DIALOG, 0, show ? 1 : 0,
                                               static_cast<void*>(this));
    CServiceBroker::GetWinSystem()->GetGfxContext().restore(count);
  }
  else
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ADDON_DIALOG, 0, show ? 1 : 0,
                                               static_cast<void*>(this));
}

void CGUIAddonWindowDialog::Show_Internal(bool show /* = true */)
{
  if (show)
  {
    m_bRunning = true;
    CServiceBroker::GetGUI()->GetWindowManager().RegisterDialog(this);

    // activate this window...
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetID());
    OnMessage(msg);

    // this dialog is derived from GUIMediaWindow
    // make sure it is rendered last
    m_renderOrder = RENDER_ORDER_DIALOG;
    while (m_bRunning)
    {
      if (!ProcessRenderLoop(false))
        break;
    }
  }
  else // hide
  {
    m_bRunning = false;

    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
    OnMessage(msg);

    CServiceBroker::GetGUI()->GetWindowManager().RemoveDialog(GetID());
  }
}

} /* namespace ADDON */
