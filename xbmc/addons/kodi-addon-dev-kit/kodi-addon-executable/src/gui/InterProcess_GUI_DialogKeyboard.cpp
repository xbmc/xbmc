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

#include "InterProcess_GUI_DialogKeyboard.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInputWithHead(
          std::string&            strText,
          const std::string&      strHeading,
          bool                    allowEmptyResult,
          bool                    hiddenInput,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* text;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndGetInputWithHead, session);
      vrp.push(API_STRING,        strHeading.c_str());
      vrp.push(API_BOOLEAN,      &allowEmptyResult);
      vrp.push(API_BOOLEAN,      &hiddenInput);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &text);
      if (retCode != API_SUCCESS)
        throw retCode;
      strText = text;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInput(
          std::string&            strText,
          bool                    allowEmptyResult,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* text;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndGetInput, session);
      vrp.push(API_BOOLEAN,      &allowEmptyResult);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &text);
      if (retCode != API_SUCCESS)
        throw retCode;
      strText = text;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(
          std::string&            strNewPassword,
          const std::string&      strHeading,
          bool                    allowEmptyResult,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* newPassword;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPasswordWithHead, session);
      vrp.push(API_STRING,        strHeading.c_str());
      vrp.push(API_BOOLEAN,      &allowEmptyResult);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &newPassword);
      if (retCode != API_SUCCESS)
        throw retCode;
      strNewPassword = newPassword;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPassword(
          std::string&            strNewPassword,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* newPassword;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPasswordWithHead, session);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &newPassword);
      if (retCode != API_SUCCESS)
        throw retCode;
      strNewPassword = newPassword;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(
          std::string&            strNewPassword,
          const std::string&      strHeading,
          bool                    allowEmptyResult,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* newPassword;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead, session);
      vrp.push(API_STRING,        strHeading.c_str());
      vrp.push(API_BOOLEAN,      &allowEmptyResult);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &newPassword);
      if (retCode != API_SUCCESS)
        throw retCode;
      strNewPassword = newPassword;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPassword(
          std::string&            strNewPassword,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* newPassword;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPassword, session);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &newPassword);
      if (retCode != API_SUCCESS)
        throw retCode;
      strNewPassword = newPassword;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  int CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyPassword(
          std::string&            strPassword,
          const std::string&      strHeading,
          int                     iRetries,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* password;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyPassword, session);
      vrp.push(API_STRING,        strHeading.c_str());
      vrp.push(API_INT,          &iRetries);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &password);
      if (retCode != API_SUCCESS)
        throw retCode;
      strPassword = password;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetFilter(
          std::string&            strText,
          bool                    searching,
          unsigned int            autoCloseMs)
  {
    try
    {
      bool ret;
      char* text;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_ShowAndGetFilter, session);
      vrp.push(API_BOOLEAN,      &searching);
      vrp.push(API_UNSIGNED_INT, &autoCloseMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &text);
      if (retCode != API_SUCCESS)
        throw retCode;
      strText = text;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_SendTextToActiveKeyboard(
          const std::string&      aTextString,
          bool                    closeKeyboard)
  {
    try
    {
      bool ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_SendTextToActiveKeyboard, session);
      vrp.push(API_STRING,        aTextString.c_str());
      vrp.push(API_BOOLEAN,      &closeKeyboard);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogKeyboard::Dialogs_Keyboard_isKeyboardActivated()
  {
    try
    {
      bool ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Keyboard_isKeyboardActivated, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }


}; /* extern "C" */
