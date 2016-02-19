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

#include "XBDateTime.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "dialogs/GUIDialogNumeric.h"

#include "AddonGUIDialogNumeric.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_Numeric::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Dialogs.Numeric.ShowAndVerifyNewPassword      = CAddOnDialog_Numeric::ShowAndVerifyNewPassword;
  callbacks->GUI.Dialogs.Numeric.ShowAndVerifyPassword         = CAddOnDialog_Numeric::ShowAndVerifyPassword;
  callbacks->GUI.Dialogs.Numeric.ShowAndVerifyInput            = CAddOnDialog_Numeric::ShowAndVerifyInput;
  callbacks->GUI.Dialogs.Numeric.ShowAndGetTime                = CAddOnDialog_Numeric::ShowAndGetTime;
  callbacks->GUI.Dialogs.Numeric.ShowAndGetDate                = CAddOnDialog_Numeric::ShowAndGetDate;
  callbacks->GUI.Dialogs.Numeric.ShowAndGetIPAddress           = CAddOnDialog_Numeric::ShowAndGetIPAddress;
  callbacks->GUI.Dialogs.Numeric.ShowAndGetNumber              = CAddOnDialog_Numeric::ShowAndGetNumber;
  callbacks->GUI.Dialogs.Numeric.ShowAndGetSeconds             = CAddOnDialog_Numeric::ShowAndGetSeconds;
}

bool CAddOnDialog_Numeric::ShowAndVerifyNewPassword(char &strNewPassword, unsigned int &iMaxStringSize)
{
  try
  {
    std::string str = &strNewPassword;
    bool bRet = CGUIDialogNumeric::ShowAndVerifyNewPassword(str);
    if (bRet)
      strncpy(&strNewPassword, str.c_str(), iMaxStringSize);
    iMaxStringSize = str.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

int CAddOnDialog_Numeric::ShowAndVerifyPassword(char &strPassword, unsigned int &iMaxStringSize, const char *strHeading, int iRetries)
{
  try
  {
    std::string str = &strPassword;
    int bRet = CGUIDialogNumeric::ShowAndVerifyPassword(str, strHeading, iRetries);
    if (bRet)
      strncpy(&strPassword, str.c_str(), iMaxStringSize);
    iMaxStringSize = str.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

bool CAddOnDialog_Numeric::ShowAndVerifyInput(char &strToVerify, unsigned int &iMaxStringSize, const char *strHeading, bool bVerifyInput)
{
  try
  {
    std::string str = &strToVerify;
    bool bRet = CGUIDialogNumeric::ShowAndVerifyInput(str, strHeading, bVerifyInput);
    if (bRet)
      strncpy(&strToVerify, str.c_str(), iMaxStringSize);
    iMaxStringSize = str.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_Numeric::ShowAndGetTime(tm &time, const char *strHeading)
{
  try
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
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_Numeric::ShowAndGetDate(tm &date, const char *strHeading)
{
  try
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
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_Numeric::ShowAndGetIPAddress(char &strIPAddress, unsigned int &iMaxStringSize, const char *strHeading)
{
  try
  {
    std::string strIP = &strIPAddress;
    bool bRet = CGUIDialogNumeric::ShowAndGetIPAddress(strIP, strHeading);
    if (bRet)
      strncpy(&strIPAddress, strIP.c_str(), iMaxStringSize);
    iMaxStringSize = strIP.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_Numeric::ShowAndGetNumber(char &strInput, unsigned int &iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs)
{
  try
  {
    std::string str = &strInput;
    bool bRet = CGUIDialogNumeric::ShowAndGetNumber(str, strHeading, iAutoCloseTimeoutMs);
    if (bRet)
      strncpy(&strInput, str.c_str(), iMaxStringSize);
    iMaxStringSize = str.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_Numeric::ShowAndGetSeconds(char &timeString, unsigned int &iMaxStringSize, const char *strHeading)
{
  try
  {
    std::string str = &timeString;
    bool bRet = CGUIDialogNumeric::ShowAndGetSeconds(str, strHeading);
    if (bRet)
      strncpy(&timeString, str.c_str(), iMaxStringSize);
    iMaxStringSize = str.length();
    return bRet;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
