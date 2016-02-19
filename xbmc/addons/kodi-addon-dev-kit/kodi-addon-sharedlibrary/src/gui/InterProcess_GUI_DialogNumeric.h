#pragma once
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_DialogNumeric
  {
    bool Dialogs_Numeric_ShowAndVerifyNewPassword(
          std::string&            strNewPassword);
    int Dialogs_Numeric_ShowAndVerifyPassword(
          std::string&            strPassword,
          const std::string&      strHeading,
          int                     iRetries);
    bool Dialogs_Numeric_ShowAndVerifyInput(
          std::string&            strToVerify,
          const std::string&      strHeading,
          bool                    bVerifyInput);
    bool Dialogs_Numeric_ShowAndGetTime(
          tm&                     time,
          const std::string&      strHeading);
    bool Dialogs_Numeric_ShowAndGetDate(
          tm&                     date,
          const std::string&      strHeading);
    bool Dialogs_Numeric_ShowAndGetIPAddress(
          std::string&            strIPAddress,
          const std::string&      strHeading);
    bool Dialogs_Numeric_ShowAndGetNumber(
          std::string&            strInput,
          const std::string&      strHeading,
          unsigned int            iAutoCloseTimeoutMs);
    bool Dialogs_Numeric_ShowAndGetSeconds(
          std::string&            strTime,
          const std::string&      strHeading);
  };

}; /* extern "C" */
