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

extern "C"
{

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyNewPassword(
          std::string&            strNewPassword)
{
  strNewPassword.reserve(1024);
  unsigned int size = (unsigned int)strNewPassword.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndVerifyNewPassword(strNewPassword[0], size);
  strNewPassword.resize(size);
  strNewPassword.shrink_to_fit();
  return ret;
}

int CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyPassword(
          std::string&            strPassword,
          const std::string&      strHeading,
          int                     iRetries)
{
  strPassword.resize(1024);
  unsigned int size = (unsigned int)strPassword.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndVerifyPassword(strPassword[0], size, strHeading.c_str(), iRetries);
  strPassword.resize(size);
  strPassword.shrink_to_fit();
  return ret;
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyInput(
          std::string&            strToVerify,
          const std::string&      strHeading,
          bool                    bVerifyInput)
{
  strToVerify.resize(1024);
  unsigned int size = (unsigned int)strToVerify.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndVerifyInput(strToVerify[0], size, strHeading.c_str(), bVerifyInput);
  strToVerify.resize(size);
  strToVerify.shrink_to_fit();
  return ret;
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetTime(
          tm&                     time,
          const std::string&      strHeading)
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndGetTime(time, strHeading.c_str());
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetDate(
          tm&                     date,
          const std::string&      strHeading)
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndGetDate(date, strHeading.c_str());
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetIPAddress(
          std::string&            strIPAddress,
          const std::string&      strHeading)
{
  strIPAddress.resize(1024);
  unsigned int size = (unsigned int)strIPAddress.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndGetIPAddress(strIPAddress[0], size, strHeading.c_str());
  strIPAddress.resize(size);
  strIPAddress.shrink_to_fit();
  return ret;
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetNumber(
          std::string&            strInput,
          const std::string&      strHeading,
          unsigned int            iAutoCloseTimeoutMs)
{
  strInput.resize(1024);
  unsigned int size = (unsigned int)strInput.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndGetNumber(strInput[0], size, strHeading.c_str(), iAutoCloseTimeoutMs);
  strInput.resize(size);
  strInput.shrink_to_fit();
  return ret;
}

bool CKODIAddon_InterProcess_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetSeconds(
          std::string&            strTime,
          const std::string&      strHeading)
{
  strTime.resize(1024);
  unsigned int size = (unsigned int)strTime.capacity();
  bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Numeric.ShowAndGetSeconds(strTime[0], size, strHeading.c_str());
  strTime.resize(size);
  strTime.shrink_to_fit();
  return ret;
}

}; /* extern "C" */
