/*
 *      Copyright (C) 2016 Team KODI
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

#include "AddonExe_GUI_DialogNumeric.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogNumeric.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyNewPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndVerifyNewPassword(*strNewPassword, iMaxStringSize);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strPassword;
  char* strHeading;
  int iRetries;
  req.pop(API_STRING, &strPassword);
  req.pop(API_STRING, &strHeading);
  req.pop(API_INT,    &iRetries);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  strncpy(strNewPassword, strPassword, iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndVerifyPassword(*strNewPassword, iMaxStringSize, strHeading, iRetries);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyInput(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strPassword;
  char* strHeading;
  bool bVerifyInput;
  req.pop(API_STRING,   &strPassword);
  req.pop(API_STRING,   &strHeading);
  req.pop(API_BOOLEAN,  &bVerifyInput);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strInput = (char*)malloc(iMaxStringSize);
  strncpy(strInput, strPassword, iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndVerifyInput(*strInput, iMaxStringSize, strHeading, bVerifyInput);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strInput);
  free(strInput);
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  tm time;
  char* strHeading;
  uint32_t retValue = API_SUCCESS;
  req.pop(API_STRING, &strHeading);
  for (unsigned int i = 0; i < sizeof(tm); ++i)
    req.pop(API_UINT8_T, &time+i);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndGetTime(time, strHeading);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_BOOLEAN,  &ret);
  for (unsigned int i = 0; i < sizeof(tm); ++i)
    resp.push(API_UINT8_T, &time+i);
  resp.finalise();
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetDate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  tm date;
  char* strHeading;
  uint32_t retValue = API_SUCCESS;
  req.pop(API_STRING, &strHeading);
  for (unsigned int i = 0; i < sizeof(tm); ++i)
    req.pop(API_UINT8_T, &date+i);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndGetDate(date, strHeading);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_BOOLEAN,  &ret);
  for (unsigned int i = 0; i < sizeof(tm); ++i)
    resp.push(API_UINT8_T, &date+i);
  resp.finalise();
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetIPAddress(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strIP;
  char* strHeading;
  req.pop(API_STRING, &strIP);
  req.pop(API_STRING, &strHeading);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strInput = (char*)malloc(iMaxStringSize);
  strncpy(strInput, strIP, iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndGetIPAddress(*strInput, iMaxStringSize, strHeading);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strInput);
  free(strInput);
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetNumber(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strNumber;
  char* strHeading;
  unsigned int iAutoCloseTimeoutMs;
  req.pop(API_STRING,        &strNumber);
  req.pop(API_STRING,        &strHeading);
  req.pop(API_UNSIGNED_INT,  &iAutoCloseTimeoutMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strInput = (char*)malloc(iMaxStringSize);
  strncpy(strInput, strNumber, iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndGetNumber(*strInput, iMaxStringSize, strHeading, iAutoCloseTimeoutMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strInput);
  free(strInput);
  return true;
}

bool CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetSeconds(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strTime;
  char* strHeading;
  req.pop(API_STRING,   &strTime);
  req.pop(API_STRING,   &strHeading);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strInput = (char*)malloc(iMaxStringSize);
  strncpy(strInput, strTime, iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Numeric::ShowAndGetSeconds(*strInput, iMaxStringSize, strHeading);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strInput);
  free(strInput);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
