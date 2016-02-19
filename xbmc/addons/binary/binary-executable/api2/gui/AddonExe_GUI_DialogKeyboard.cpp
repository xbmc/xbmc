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

#include "AddonExe_GUI_DialogKeyboard.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogKeyboard.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInputWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strHeading;
  bool  allowEmptyResult;
  bool  hiddenInput;
  unsigned int autoCloseMs;
  req.pop(API_STRING,       &strHeading);
  req.pop(API_BOOLEAN,      &allowEmptyResult);
  req.pop(API_BOOLEAN,      &hiddenInput);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strText = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndGetInputWithHead(*strText, iMaxStringSize, strHeading, allowEmptyResult, hiddenInput, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strText);
  free(strText);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInput(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool allowEmptyResult;
  unsigned int autoCloseMs;
  req.pop(API_BOOLEAN,      &allowEmptyResult);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strText = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndGetInput(*strText, iMaxStringSize, allowEmptyResult, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strText);
  free(strText);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strHeading;
  bool  allowEmptyResult;
  unsigned int autoCloseMs;
  req.pop(API_STRING,       &strHeading);
  req.pop(API_BOOLEAN,      &allowEmptyResult);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndGetNewPasswordWithHead(*strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int autoCloseMs;
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndGetNewPassword(*strNewPassword, iMaxStringSize, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strHeading;
  bool  allowEmptyResult;
  unsigned int autoCloseMs;
  req.pop(API_STRING,       &strHeading);
  req.pop(API_BOOLEAN,      &allowEmptyResult);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndVerifyNewPasswordWithHead(*strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int autoCloseMs;
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strNewPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndVerifyNewPassword(*strNewPassword, iMaxStringSize, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strNewPassword);
  free(strNewPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strHeading;
  int   iRetries;
  unsigned int autoCloseMs;
  req.pop(API_STRING,       &strHeading);
  req.pop(API_INT,          &iRetries);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndVerifyPassword(*strPassword, iMaxStringSize, strHeading, iRetries, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strPassword);
  free(strPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetFilter(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool searching;
  unsigned int autoCloseMs;
  req.pop(API_BOOLEAN,      &searching);
  req.pop(API_UNSIGNED_INT, &autoCloseMs);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* strPassword = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_Keyboard::ShowAndGetFilter(*strPassword, iMaxStringSize, searching, autoCloseMs);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, strPassword);
  free(strPassword);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_SendTextToActiveKeyboard(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* aTextString;
  bool  closeKeyboard;
  req.pop(API_STRING,  &aTextString);
  req.pop(API_BOOLEAN, &closeKeyboard);
  bool ret = GUI::CAddOnDialog_Keyboard::SendTextToActiveKeyboard(aTextString, closeKeyboard);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &ret);
  return true;
}

bool CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_isKeyboardActivated(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool ret = GUI::CAddOnDialog_Keyboard::isKeyboardActivated();
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &ret);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
