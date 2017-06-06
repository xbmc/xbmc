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

#include "Numeric.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Numeric.h"

#include "XBDateTime.h"
#include "addons/AddonDll.h"
#include "dialogs/GUIDialogNumeric.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogNumeric::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogNumeric = static_cast<AddonToKodiFuncTable_kodi_gui_dialogNumeric*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogNumeric)));

  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_new_password = show_and_verify_new_password;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_password = show_and_verify_password;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_verify_input = show_and_verify_input;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_time = show_and_get_time;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_date = show_and_get_date;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_ip_address = show_and_get_ip_address;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_number = show_and_get_number;
  addonInterface->toKodi->kodi_gui->dialogNumeric->show_and_get_seconds = show_and_get_seconds;
}

void Interface_GUIDialogNumeric::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogNumeric);
}

bool Interface_GUIDialogNumeric::show_and_verify_new_password(void* kodiBase, char** password)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  std::string str;
  bool bRet = CGUIDialogNumeric::ShowAndVerifyNewPassword(str);
  if (bRet)
    *password = strdup(str.c_str());
  return bRet;
}

int Interface_GUIDialogNumeric::show_and_verify_password(void* kodiBase, const char* password, const char* heading, int retries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return -1;
  }

  if (!password || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (password='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, password, heading, addon->ID().c_str());
    return -1;
  }

  std::string pw(password);
  return CGUIDialogNumeric::ShowAndVerifyPassword(pw, heading, retries);
}

bool Interface_GUIDialogNumeric::show_and_verify_input(void* kodiBase, const char* verify_in, char** verify_out, const char* heading, bool verify_input)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!verify_in || !verify_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (verify_in='%p', verify_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, verify_in, verify_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = verify_in;
  bool bRet = CGUIDialogNumeric::ShowAndVerifyInput(str, heading, verify_input);
  if (bRet)
    *verify_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogNumeric::show_and_get_time(void* kodiBase, tm* time, const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!time || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (time='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, time, heading, addon->ID().c_str());
    return false;
  }

  SYSTEMTIME systemTime;
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

bool Interface_GUIDialogNumeric::show_and_get_date(void* kodiBase, tm *date, const char *heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!date || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (date='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, date, heading, addon->ID().c_str());
    return false;
  }

  SYSTEMTIME systemTime;
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

bool Interface_GUIDialogNumeric::show_and_get_ip_address(void* kodiBase, const char* ip_address_in, char** ip_address_out, const char *heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!ip_address_in || !ip_address_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (ip_address_in='%p', ip_address_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, ip_address_in, ip_address_out, heading, addon->ID().c_str());
    return false;
  }

  std::string strIP = ip_address_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetIPAddress(strIP, heading);
  if (bRet)
    *ip_address_out = strdup(strIP.c_str());
  return bRet;
}

bool Interface_GUIDialogNumeric::show_and_get_number(void* kodiBase, const char* number_in, char** number_out, const char *heading, unsigned int auto_close_ms)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!number_in || !number_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (number_in='%p', number_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, number_in, number_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = number_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetNumber(str, heading, auto_close_ms);
  if (bRet)
    *number_out = strdup(str.c_str());
  return bRet;
}

bool Interface_GUIDialogNumeric::show_and_get_seconds(void* kodiBase, const char* time_in, char** time_out, const char *heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!time_in || !time_out || !heading)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogNumeric::%s - invalid handler data (time_in='%p', time_out='%p', heading='%p') on addon '%s'",
                          __FUNCTION__, time_in, time_out, heading, addon->ID().c_str());
    return false;
  }

  std::string str = time_in;
  bool bRet = CGUIDialogNumeric::ShowAndGetSeconds(str, heading);
  if (bRet)
    *time_out = strdup(str.c_str());
  return bRet;
}

} /* namespace ADDON */
} /* extern "C" */
