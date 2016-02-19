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

#include "InterProcess_GUI_DialogNumeric.h"
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

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyNewPassword(
            std::string&            strNewPassword)
  {
    try
    {
      bool ret;
      char* password;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndVerifyNewPassword, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &password);
      if (retCode != API_SUCCESS)
        throw retCode;
      strNewPassword = password;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  int CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyPassword(
            std::string&            strPassword,
            const std::string&      strHeading,
            int                     iRetries)
  {
    try
    {
      bool ret;
      char* password;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndVerifyPassword, session);
      vrp.push(API_STRING, strPassword.c_str());
      vrp.push(API_STRING, strHeading.c_str());
      vrp.push(API_INT,   &iRetries);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &password);
      if (retCode != API_SUCCESS)
        throw retCode;
      strPassword = password;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyInput(
            std::string&            strToVerify,
            const std::string&      strHeading,
            bool                    bVerifyInput)
  {
    try
    {
      bool ret;
      char* password;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndVerifyInput, session);
      vrp.push(API_STRING,    strToVerify.c_str());
      vrp.push(API_STRING,    strHeading.c_str());
      vrp.push(API_BOOLEAN,  &bVerifyInput);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &password);
      if (retCode != API_SUCCESS)
        throw retCode;
      strToVerify = password;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetTime(
            tm&                     time,
            const std::string&      strHeading)
  {
    try
    {
      bool ret;
      char* retAddress;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndGetTime, session);
      vrp.push(API_STRING, strHeading.c_str());
      for (unsigned int i = 0; i < sizeof(tm); ++i)
        vrp.push(API_UINT8_T, &time+i);

      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T, &retCode);
      vresp->pop(API_BOOLEAN,  &ret);
      for (unsigned int i = 0; i < sizeof(tm); ++i)
        vresp->pop(API_UINT8_T, &time+i);

      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetDate(
            tm&                     date,
            const std::string&      strHeading)
  {
    try
    {
      bool ret;
      char* retAddress;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndGetTime, session);
      vrp.push(API_STRING, strHeading.c_str());
      for (unsigned int i = 0; i < sizeof(tm); ++i)
        vrp.push(API_UINT8_T, &date+i);

      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T, &retCode);
      vresp->pop(API_BOOLEAN,  &ret);
      for (unsigned int i = 0; i < sizeof(tm); ++i)
        vresp->pop(API_UINT8_T, &date+i);

      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetIPAddress(
            std::string&            strIPAddress,
            const std::string&      strHeading)
  {
    try
    {
      bool ret;
      char* retAddress;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndGetIPAddress, session);
      vrp.push(API_STRING,    strIPAddress.c_str());
      vrp.push(API_STRING,    strHeading.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &retAddress);
      if (retCode != API_SUCCESS)
        throw retCode;
      strIPAddress = retAddress;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetNumber(
            std::string&            strInput,
            const std::string&      strHeading,
            unsigned int            iAutoCloseTimeoutMs)
  {
    try
    {
      bool ret;
      char* retNumber;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndGetNumber, session);
      vrp.push(API_STRING,        strInput.c_str());
      vrp.push(API_STRING,        strHeading.c_str());
      vrp.push(API_UNSIGNED_INT, &iAutoCloseTimeoutMs);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &retNumber);
      if (retCode != API_SUCCESS)
        throw retCode;
      strInput = retNumber;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetSeconds(
            std::string&            strTime,
            const std::string&      strHeading)
  {
    try
    {
      bool ret;
      char* retTime;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Numeric_ShowAndGetSeconds, session);
      vrp.push(API_STRING,    strTime.c_str());
      vrp.push(API_STRING,    strHeading.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &retTime);
      if (retCode != API_SUCCESS)
        throw retCode;
      strTime = retTime;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

}; /* extern "C" */
