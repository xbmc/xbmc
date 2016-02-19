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

#include "InterProcess.h"
#include "kodi/api2/gui/DialogNumeric.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
namespace DialogNumeric
{

  bool ShowAndVerifyNewPassword(
          std::string&            strNewPassword)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndVerifyNewPassword(strNewPassword);
  }

  int ShowAndVerifyPassword(
          std::string&            strPassword,
          const std::string&      strHeading,
          int                     iRetries)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndVerifyPassword(strPassword, strHeading, iRetries);
  }

  bool ShowAndVerifyInput(
          std::string&            strToVerify,
          const std::string&      strHeading,
          bool                    bVerifyInput)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndVerifyInput(strToVerify, strHeading, bVerifyInput);
  }

  bool ShowAndGetTime(
          tm&                     time,
          const std::string&      strHeading)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndGetTime(time, strHeading);
  }

  bool ShowAndGetDate(
          tm&                     date,
          const std::string&      strHeading)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndGetDate(date, strHeading);
  }

  bool ShowAndGetIPAddress(
          std::string&            strIPAddress,
          const std::string&      strHeading)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndGetIPAddress(strIPAddress, strHeading);
  }

  bool ShowAndGetNumber(
          std::string&            strInput,
          const std::string&      strHeading,
          unsigned int            iAutoCloseTimeoutMs)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndGetNumber(strInput, strHeading, iAutoCloseTimeoutMs);
  }

  bool ShowAndGetSeconds(
          std::string&            strTime,
          const std::string&      strHeading)
  {
    return g_interProcess.Dialogs_Numeric_ShowAndGetSeconds(strTime, strHeading);
  }

}; /* namespace DialogNumeric */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
