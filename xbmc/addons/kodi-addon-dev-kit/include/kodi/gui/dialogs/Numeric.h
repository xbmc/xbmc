/*
 *      Copyright (C) 2005-2017 Team KODI
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

#pragma once

#include "../definitions.h"
#include "../../AddonBase.h"

namespace kodi
{
namespace gui
{
namespace dialogs
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_dialogs_Numeric Dialog Numeric
  /// \ingroup cpp_kodi_gui
  /// @{
  /// @brief \cpp_namespace{ kodi::gui::dialogs::Numeric }
  /// **Numeric dialogs**
  ///
  /// The functions listed below have to be permitted by the user for the
  /// representation of a numeric keyboard around an input.
  ///
  /// The class supports several kinds, from an easy number choice up to the
  /// passport Word production and their confirmation for add-on.
  ///
  /// It has the header \ref Numeric.h "#include <kodi/gui/dialogs/Numeric.h>"
  /// be included to enjoy it.
  ///
  namespace Numeric
  {
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to get numeric new password
    ///
    /// @param[out] newPassword        String to preload into the keyboard
    ///                                accumulator. Overwritten with user input
    ///                                if return=true. Returned in MD5 format.
    /// @return                        true if successful display and user
    ///                                input entry/re-entry.
    ///                                false if unsuccessful display, no user
    ///                                input, or canceled editing.
    ///
    inline bool ShowAndVerifyNewPassword(std::string& newPassword)
    {
      using namespace ::kodi::addon;
      char* pw = nullptr;
      bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_verify_new_password(CAddonBase::m_interface->toKodi->kodiBase, &pw);
      if (pw != nullptr)
      {
        newPassword = pw;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, pw);
      }
      return ret;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to verify numeric password.
    ///
    /// @param[in] password             Password to compare with user input, need
    ///                                 in MD5 format.
    /// @param[in] heading              Heading to display
    /// @param[in] retries              If greater than 0, shows "Incorrect
    ///                                 password, %d retries left" on dialog
    ///                                 line 2, else line 2 is blank.
    /// @return                         Possible values:
    ///                                 - 0 if successful display and user input.
    ///                                 - 1 if unsuccessful input.
    ///                                 - -1 if no user input or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* fprintf */
    /// #include <kodi/General.h>
    /// #include <kodi/gui/dialogs/Numeric.h>
    ///
    /// /*
    ///  * The example below shows the complete use of keyboard dialog for password
    ///  * check. If only one check from add-on needed can be function with retries
    ///  * set to '0' called alone.
    ///  *
    ///  * The use of MD5 translated password is always required for the check on Kodi!
    ///  */
    ///
    /// int maxretries = 3;
    ///
    /// /*
    ///  * Password names need to be send as md5 sum to kodi.
    ///  */
    /// std::string password = kodi::GetMD5("1234");
    ///
    /// /*
    ///  * To the loop about password checks.
    ///  */
    /// int ret;
    /// for (unsigned int i = 0; i < maxretries; i++)
    /// {
    ///   /*
    ///    * Ask the user about the password.
    ///    */
    ///   ret = kodi::gui::dialogs::Numeric::ShowAndVerifyPassword(password, "Demo numeric password call for PW '1234'", i);
    ///   if (ret == 0)
    ///   {
    ///     fprintf(stderr, "Numeric password successfull confirmed after '%i' tries\n", i+1);
    ///     break;
    ///   }
    ///   else if (ret < 0)
    ///   {
    ///     fprintf(stderr, "Canceled editing on try '%i'\n", i+1);
    ///     break;
    ///   }
    ///   else /* if (ret > 0) */
    ///   {
    ///     fprintf(stderr, "Wrong numeric password entered on try '%i'\n", i+1);
    ///   }
    /// }
    /// ~~~~~~~~~~~~~
    ///
    inline int ShowAndVerifyPassword(const std::string& password, const std::string& heading, int retries)
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_verify_password(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                password.c_str(), heading.c_str(), retries);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to verify numeric password
    ///
    /// @param[in,out] toVerify         Value to compare against user input.
    /// @param[in] heading              Heading to display
    /// @param[in] verifyInput          If set as true we verify the users input
    ///                                 versus toVerify.
    /// @return                         true if successful display and user
    ///                                 input. false if unsuccessful display, no
    ///                                 user input, or canceled editing.
    ///
    inline bool ShowAndVerifyInput(std::string& toVerify, const std::string& heading, bool verifyInput)
    {
      using namespace ::kodi::addon;
      char* retString = nullptr;
      bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_verify_input(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                 toVerify.c_str(), &retString, heading.c_str(), verifyInput);
      if (retString != nullptr)
      {
        toVerify = retString;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, retString);
      }
      return ret;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to get time value.
    ///
    /// @param[out] time                Overwritten with user input if
    ///                                 return=true and time inserted.
    /// @param[in] heading              Heading to display.
    /// @return                         true if successful display and user
    ///                                 input. false if unsuccessful display, no
    ///                                 user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* printf */
    /// #include <time.h>       /* time_t, struct tm, time, localtime, strftime */
    /// #include <kodi/gui/dialogs/Numeric.h>
    ///
    /// time_t rawtime;
    /// struct tm * timeinfo;
    /// char buffer [10];
    ///
    /// time (&rawtime);
    /// timeinfo = localtime(&rawtime);
    /// bool bRet = kodi::gui::dialogs::Numeric::ShowAndGetTime(*timeinfo, "Selected time test call");
    /// strftime(buffer, sizeof(buffer), "%H:%M.", timeinfo);
    /// printf("Selected time it's %s and was on Dialog %s\n", buffer, bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetTime(tm& time, const std::string& heading)
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_get_time(CAddonBase::m_interface->toKodi->kodiBase, &time, heading.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to get date value.
    ///
    /// @param[in,out] date             Overwritten with user input if
    ///                                 return=true and date inserted.
    /// @param[in] heading              Heading to display
    /// @return                         true if successful display and user
    ///                                 input. false if unsuccessful display, no
    ///                                 user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* printf */
    /// #include <time.h>       /* time_t, struct tm, time, localtime, strftime */
    /// #include <kodi/gui/dialogs/Numeric.h>
    ///
    /// time_t rawtime;
    /// struct tm * timeinfo;
    /// char buffer [20];
    ///
    /// time (&rawtime);
    /// timeinfo = localtime(&rawtime);
    /// bool bRet = kodi::gui::dialogs::Numeric::ShowAndGetDate(*timeinfo, "Selected date test call");
    /// strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    /// printf("Selected date it's %s and was on Dialog %s\n", buffer, bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetDate(tm& date, const std::string& heading)
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_get_date(CAddonBase::m_interface->toKodi->kodiBase, &date, heading.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to get a IP
    ///
    /// @param[in,out] ipAddress        Overwritten with user input if
    ///                                 return=true and IP address inserted.
    /// @param[in] heading              Heading to display.
    /// @return                         true if successful display and
    ///                                 user input. false if unsuccessful
    ///                                 display, no user input, or canceled
    ///                                 editing.
    ///
    inline bool ShowAndGetIPAddress(std::string& ipAddress, const std::string& heading)
    {
      using namespace ::kodi::addon;
      char* retString = nullptr;
      bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_get_ip_address(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                   ipAddress.c_str(), &retString, heading.c_str());
      if (retString != nullptr)
      {
        ipAddress = retString;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, retString);
      }
      return ret;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Use dialog to get normal number.
    ///
    /// @param[in,out] input            Overwritten with user input if
    ///                                 return=true and time in seconds inserted
    /// @param[in] heading              Heading to display
    /// @param[in] autoCloseTimeoutMs   To close the dialog after a specified
    ///                                 time, in milliseconds, default is 0
    ///                                 which keeps the dialog open
    ///                                 indefinitely.
    /// @return                         true if successful display and user
    ///                                 input. false if unsuccessful display, no
    ///                                 user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    ///   #include <stdio.h>      /* printf */
    ///   #include <stdlib.h>     /* strtoull (C++11) */
    ///   #include <kodi/gui/dialogs/Numeric.h>
    ///
    ///   std::string number;
    ///   bool bRet = kodi::gui::dialogs::Numeric::ShowAndGetNumber(number, "Number test call");
    ///   printf("Written number input is : %llu and was %s\n",
    ///                  strtoull(number.c_str(), nullptr, 0), bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetNumber(std::string& input, const std::string& heading, unsigned int autoCloseTimeoutMs = 0)
    {
      using namespace ::kodi::addon;
      char* retString = nullptr;
      bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_get_number(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                               input.c_str(), &retString, heading.c_str(), autoCloseTimeoutMs);
      if (retString != nullptr)
      {
        input = retString;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, retString);
      }
      return ret;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_Numeric
    /// @brief Show numeric keypad to get seconds.
    ///
    /// @param[in,out] time     Overwritten with user input if return=true and
    ///                         time in seconds inserted.
    /// @param[in] heading      Heading to display
    /// @return                 true if successful display and user input. false
    ///                         if unsuccessful display, no user input, or
    ///                         canceled editing.
    ///
    inline bool ShowAndGetSeconds(std::string& time, const std::string& heading)
    {
      using namespace ::kodi::addon;
      char* retString = nullptr;
      bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogNumeric->show_and_get_seconds(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                time.c_str(), &retString, heading.c_str());
      if (retString != nullptr)
      {
        time = retString;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, retString);
      }
      return ret;
    }
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */
