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

#include "DialogKeyboard.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/DialogKeyboard.h"

#include "addons/AddonDll.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/log.h"
#include "utils/Variant.h"

namespace ADDON
{
extern "C"
{

void Interface_GUIDialogKeyboard::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogKeyboard = static_cast<AddonToKodiFuncTable_kodi_gui_dialogKeyboard*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogKeyboard)));

  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_input_with_head = show_and_get_input_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_input = show_and_get_input;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_new_password_with_head = show_and_get_new_password_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_new_password = show_and_get_new_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_new_password_with_head = show_and_verify_new_password_with_head;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_new_password = show_and_verify_new_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_verify_password = show_and_verify_password;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->show_and_get_filter = show_and_get_filter;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->send_text_to_active_keyboard = send_text_to_active_keyboard;
  addonInterface->toKodi->kodi_gui->dialogKeyboard->is_keyboard_activated = is_keyboard_activated;
}

void Interface_GUIDialogKeyboard::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogKeyboard);
}

bool Interface_GUIDialogKeyboard::show_and_get_input_with_head(void* kodiBase, const char* text_in, char** text_out,
                                                               const char* heading, bool allow_empty_result,
                                                               bool hidden_input, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!text_in || !text_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (text_in='%p', text_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, text_in, text_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, CVariant{heading}, allow_empty_result, hidden_input, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_input(void* kodiBase, const char* text_in, char** text_out, bool allow_empty_result, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!text_in || !text_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (text_in='%p', text_out='%p') on addon '%s'",
                          __FUNCTION__, text_in, text_out, addon->ID().c_str());
    return false;
  }

  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetInput(str, allow_empty_result, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_new_password_with_head(void* kodiBase, const char* password_in, char** password_out,
                                                                      const char* heading, bool allow_empty_result, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!password_in || !password_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (password_in='%p', password_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, password_in, password_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = password_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetNewPassword(str, heading, allow_empty_result, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_new_password(void* kodiBase, const char* password_in, char** password_out, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!password_in || !password_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (password_in='%p', password_out='%p') on addon '%s'",
                          __FUNCTION__, password_in, password_out, addon->ID().c_str());
    return false;
  }

  std::string str = password_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetNewPassword(str, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_verify_new_password_with_head(void* kodiBase, char** password_out, const char* heading,
                                                                         bool allowEmpty, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!password_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (password_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, password_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str;
  bool bRet = CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, heading, allowEmpty, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::show_and_verify_new_password(void* kodiBase, char** password_out, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!password_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (password_out='%p') on addon '%s'",
                          __FUNCTION__, password_out, addon->ID().c_str());
    return false;
  }

  std::string str;
  bool bRet = CGUIKeyboardFactory::ShowAndVerifyNewPassword(str, auto_close_ms);
  if (bRet)
    *password_out = strdup(str.c_str());
  return bRet;
}

int Interface_GUIDialogKeyboard::show_and_verify_password(void* kodiBase, const char* password_in, char** password_out, const char* heading, int retries, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!password_in || !password_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (password_in='%p', password_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, password_in, password_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = password_in;
  int iRet = CGUIKeyboardFactory::ShowAndVerifyPassword(str, heading, retries, auto_close_ms);
  if (iRet)
    *password_out = strdup(str.c_str());
  return iRet;
}

bool Interface_GUIDialogKeyboard::show_and_get_filter(void* kodiBase, const char* text_in, char** text_out, bool searching, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!text_in || !text_out)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid handler data (text_in='%p', text_out='%p') on addon '%s'",
                          __FUNCTION__, text_in, text_out, addon->ID().c_str());
    return false;
  }


  std::string str = text_in;
  bool bRet = CGUIKeyboardFactory::ShowAndGetFilter(str, searching, auto_close_ms);
  if (bRet)
    *text_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogKeyboard::send_text_to_active_keyboard(void* kodiBase, const char* text, bool close_keyboard)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  return CGUIKeyboardFactory::SendTextToActiveKeyboard(text, close_keyboard);
}

bool Interface_GUIDialogKeyboard::is_keyboard_activated(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogKeyboard::%s - invalid data", __FUNCTION__);
    return false;
  }

  return CGUIKeyboardFactory::isKeyboardActivated();
}

} /* extern "C" */
} /* namespace ADDON */
