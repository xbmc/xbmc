/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Keyboard.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/Keyboard.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogKeyboard::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogKeyboard =
      new AddonToKodiFuncTable_kodi_gui_dialogKeyboard();

  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_input_with_head =
      show_and_get_input_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_input = show_and_get_input;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_new_password_with_head =
      show_and_get_new_password_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_new_password =
      show_and_get_new_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_new_password_with_head =
      show_and_verify_new_password_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_new_password =
      show_and_verify_new_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_password =
      show_and_verify_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_filter = show_and_get_filter;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->send_text_to_active_keyboard =
      send_text_to_active_keyboard;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->is_keyboard_activated = is_keyboard_activated;
}

void Interface_GUIDialogKeyboard::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogKeyboard;
}

bool Interface_GUIDialogKeyboard::show_and_get_input_with_head(KODI_HANDLE kodiBase,
                                                               const char* text_in,
                                                               char** text_out,
                                                               const char* heading,
                                                               bool allow_empty_result,
                                                               bool hidden_input,
                                                               unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!text_in || !text_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (text_in='{}', "
              "text_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(text_in), static_cast<void*>(text_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, CVariant{heading}, allow_empty_result,
                                                   hidden_input, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_input(KODI_HANDLE kodiBase,
                                                     const char* text_in,
                                                     char** text_out,
                                                     bool allow_empty_result,
                                                     unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!text_in || !text_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (text_in='{}', "
              "text_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(text_in), static_cast<void*>(text_out),
              addon->ID());
    return false;
  }

  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, allow_empty_result, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_new_password_with_head(KODI_HANDLE kodiBase,
                                                                      const char* password_in,
                                                                      char** password_out,
                                                                      const char* heading,
                                                                      bool allow_empty_result,
                                                                      unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!password_in || !password_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (password_in='{}', "
              "password_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(password_in), static_cast<void*>(password_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = password_in;
  bool bRet =
      CGUIKeyboardFactory::ShowAndGetNewPassword(str, heading, allow_empty_result, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_new_password(KODI_HANDLE kodiBase,
                                                            const char* password_in,
                                                            char** password_out,
                                                            unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!password_in || !password_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (password_in='{}', "
              "password_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(password_in), static_cast<void*>(password_out),
              addon->ID());
    return false;
  }

  std::string str = password_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetNewPassword(str, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_verify_new_password_with_head(KODI_HANDLE kodiBase,
                                                                         char** password_out,
                                                                         const char* heading,
                                                                         bool allowEmpty,
                                                                         unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!password_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (password_out='{}', "
              "heading='{}') on addon '{}'",
              __func__, static_cast<void*>(password_out), static_cast<const void*>(heading),
              addon->ID());
    return false;
  }

  std::string str;
  bool bRet =
      CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, heading, allowEmpty, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_verify_new_password(KODI_HANDLE kodiBase,
                                                               char** password_out,
                                                               unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!password_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (password_out='{}') on "
              "addon '{}'",
              __func__, static_cast<void*>(password_out), addon->ID());
    return false;
  }

  std::string str;
  bool bRet = CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

int Interface_GUIDialogKeyboard::show_and_verify_password(KODI_HANDLE kodiBase,
                                                          const char* password_in,
                                                          char** password_out,
                                                          const char* heading,
                                                          int retries,
                                                          unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!password_in || !password_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (password_in='{}', "
              "password_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(password_in), static_cast<void*>(password_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = password_in;
  int iRet = CGUIKeyboardFactory::ShowAndVerifyPassword(str, heading, retries, auto_close_ms);
  if (iRet)
    *password_out = strdup(str.c_str());
  return iRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_filter(KODI_HANDLE kodiBase,
                                                      const char* text_in,
                                                      char** text_out,
                                                      bool searching,
                                                      unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  if (!text_in || !text_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogKeyboard::{} - invalid handler data (text_in='{}', "
              "text_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(text_in), static_cast<void*>(text_out),
              addon->ID());
    return false;
  }


  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetFilter(str, searching, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::send_text_to_active_keyboard(KODI_HANDLE kodiBase,
                                                               const char* text,
                                                               bool close_keyboard)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  return CGUIKeyboardFactory::SendTextToActiveKeyboard(text, close_keyboard);
}

bool Interface_GUIDialogKeyboard::is_keyboard_activated(KODI_HANDLE kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::{} - invalid data", __func__);
    return false;
  }

  return CGUIKeyboardFactory::isKeyboardActivated();
}

} /* namespace ADDON */
