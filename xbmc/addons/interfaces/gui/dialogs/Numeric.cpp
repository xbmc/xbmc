/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Numeric.h"

#include "XBDateTime.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/Numeric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogNumeric::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogNumeric =
      new AddonToKodiFuncTable_kodi_gui_dialogNumeric();

  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_new_password =
      show_and_verify_new_password;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_password =
      show_and_verify_password;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_input = show_and_verify_input;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_time = show_and_get_time;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_date = show_and_get_date;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_ip_address =
      show_and_get_ip_address;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_number = show_and_get_number;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_seconds = show_and_get_seconds;
}

void Interface_GUIDialogNumeric::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogNumeric;
}

bool Interface_GUIDialogNumeric::show_and_verify_new_password(KODI_HANDLE kodiBase, char** password)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  std::string str;
  bool bRet = CGUIDialogNumeric::ShowAndVerifyNewPassword(str);
  if (bRet)
    *password = strdup(str.c_str());
  return bRet;
}

int Interface_GUIDialogNumeric::show_and_verify_password(KODI_HANDLE kodiBase,
                                                         const char* password,
                                                         const char* heading,
                                                         int retries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return -1;
  }

  if (!password || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (password='{}', heading='{}') "
              "on addon '{}'",
              __func__, static_cast<const void*>(password), static_cast<const void*>(heading),
              addon->ID());
    return -1;
  }

  std::string pw(password);
  return CGUIDialogNumeric::ShowAndVerifyPassword(pw, heading, retries);
}

bool Interface_GUIDialogNumeric::show_and_verify_input(KODI_HANDLE kodiBase,
                                                       const char* verify_in,
                                                       char** verify_out,
                                                       const char* heading,
                                                       bool verify_input)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!verify_in || !verify_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (verify_in='{}', "
              "verify_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(verify_in), static_cast<void*>(verify_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = verify_in;
  if (CGUIDialogNumeric::ShowAndVerifyInput(str, heading, verify_input) ==
      InputVerificationResult::SUCCESS)
  {
    *verify_out = strdup(str.c_str());
    return true;
  }
  return false;
}

bool Interface_GUIDialogNumeric::show_and_get_time(KODI_HANDLE kodiBase,
                                                   tm* time,
                                                   const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!time || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (time='{}', heading='{}') on "
              "addon '{}'",
              __func__, static_cast<void*>(time), static_cast<const void*>(heading), addon->ID());
    return false;
  }

  KODI::TIME::SystemTime systemTime;
  CDateTime dateTime(*time);
  dateTime.GetAsSystemTime(systemTime);
  if (CGUIDialogNumeric::ShowAndGetTime(systemTime, heading))
  {
    dateTime = systemTime;
    dateTime.GetAsTm(*time);
    return true;
  }
  return false;
}

bool Interface_GUIDialogNumeric::show_and_get_date(KODI_HANDLE kodiBase,
                                                   tm* date,
                                                   const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!date || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (date='{}', heading='{}') on "
              "addon '{}'",
              __func__, static_cast<void*>(date), static_cast<const void*>(heading), addon->ID());
    return false;
  }

  KODI::TIME::SystemTime systemTime;
  CDateTime dateTime(*date);
  dateTime.GetAsSystemTime(systemTime);
  if (CGUIDialogNumeric::ShowAndGetDate(systemTime, heading))
  {
    dateTime = systemTime;
    dateTime.GetAsTm(*date);
    return true;
  }
  return false;
}

bool Interface_GUIDialogNumeric::show_and_get_ip_address(KODI_HANDLE kodiBase,
                                                         const char* ip_address_in,
                                                         char** ip_address_out,
                                                         const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!ip_address_in || !ip_address_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (ip_address_in='{}', "
              "ip_address_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(ip_address_in), static_cast<void*>(ip_address_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string strIP = ip_address_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetIPAddress(strIP, heading);
  if (bRet)
    *ip_address_out = strdup(strIP.c_str());
  return bRet;
}

bool Interface_GUIDialogNumeric::show_and_get_number(KODI_HANDLE kodiBase,
                                                     const char* number_in,
                                                     char** number_out,
                                                     const char* heading,
                                                     unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!number_in || !number_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (number_in='{}', "
              "number_out='{}', heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(number_in), static_cast<void*>(number_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = number_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetNumber(str, heading, auto_close_ms);
  if (bRet)
    *number_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogNumeric::show_and_get_seconds(KODI_HANDLE kodiBase,
                                                      const char* time_in,
                                                      char** time_out,
                                                      const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::{} - invalid data", __func__);
    return false;
  }

  if (!time_in || !time_out || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogNumeric::{} - invalid handler data (time_in='{}', time_out='{}', "
              "heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(time_in), static_cast<void*>(time_out),
              static_cast<const void*>(heading), addon->ID());
    return false;
  }

  std::string str = time_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetSeconds(str, heading);
  if (bRet)
    *time_out = strdup(str.c_str());
  return bRet;
}

} /* namespace ADDON */
