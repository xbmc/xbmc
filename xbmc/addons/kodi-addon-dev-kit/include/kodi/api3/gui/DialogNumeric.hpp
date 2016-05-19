#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_DialogNumeric Dialog Numeric
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Numeric dialogs</b>
  ///
  /// The functions listed below have to be permitted by the user for the
  /// representation of a numeric keyboard around an input.
  ///
  /// The class supports several kinds, from an easy number choice up to the
  /// passport Word production and their confirmation for add-on.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogNumeric.hpp "#include <kodi/api3/gui/DialogNumeric.hpp>"
  /// be included to enjoy it.
  ///
  namespace DialogNumeric
  {
    //==========================================================================
     ///
     /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
     /// @brief Use dialog to get numeric new password
     ///
     /// @param[out] strNewPassword String to preload into the keyboard accumulator.
     ///        Overwritten with user input if return=true.
     /// @return true if successful display and user input entry/re-entry.
     ///         false if unsuccessful display, no user input, or canceled editing.
     ///
    bool ShowAndVerifyNewPassword(
      std::string&            strNewPassword);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to verify numeric password.
    ///
    /// @param[out] strPassword Overwritten with user input if return=tru
    /// @param[in] strHeading Heading to display
    /// @param[in] iRetries If greater than 0, shows "Incorrect password, %d retries left"
    ///        on dialog line 2, else line 2 is blank.
    /// @return 0 if successful display and user input.
    ///         1 if unsuccessful input.
    ///        -1 if no user input or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* fprintf */
    /// #include <kodi/api3/addon/General.hpp>
    /// #include <kodi/api3/gui/DialogNumeric.hpp>
    ///
    /// /*
    ///  * The example below shows the complete use of keyboard dialog for password
    ///  * check. If only one check from add-on needed can be function with retries
    ///  * set to '0' called alone.
    ///  *
    ///  * The use of MD5 translated password is always required for the check on Kodi!
    ///  */
    ///
    /// /*
    ///  * Get from Kodi's global settings the maximum allowed retries for passwords.
    ///  */
    /// int maxretries = 0;
    /// if (KodiAPI::AddOn::General::GetSettingInt("masterlock.maxretries", maxretries, true))
    /// {
    ///   /*
    ///    * Password names need to be send as md5 sum to kodi.
    ///    */
    ///   std::string password;
    ///   CAddonLib_General::GetMD5("1234", password);
    ///
    ///   /*
    ///    * To the loop about password checks.
    ///    */
    ///   int ret;
    ///   for (unsigned int i = 0; i < maxretries; i++)
    ///   {
    ///     /*
    ///      * Ask the user about the password.
    ///      */
    ///     ret = KodiAPI::GUI::DialogNumeric::ShowAndVerifyPassword(password, "Demo numeric password call for PW '1234'", i);
    ///     if (ret == 0)
    ///     {
    ///       fprintf(stderr, "Numeric password successfull confirmed after '%i' tries\n", i+1);
    ///       break;
    ///     }
    ///     else if (ret < 0)
    ///     {
    ///       fprintf(stderr, "Canceled editing on try '%i'\n", i+1);
    ///       break;
    ///     }
    ///     else /* if (ret > 0) */
    ///     {
    ///       fprintf(stderr, "Wrong numeric password entered on try '%i'\n", i+1);
    ///      }
    ///   }
    /// }
    /// else
    ///   fprintf(stderr, "Requested global setting value 'masterlock.maxretries' not present!");
    /// ~~~~~~~~~~~~~
    ///
    int ShowAndVerifyPassword(
      std::string&            strPassword,
      const std::string&      strHeading,
      int                     iRetries);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to verify numeric password
    ///
    /// @param[out] strToVerify Value to compare against user input.
    /// @param[in] strHeading Heading to display
    /// @param[in] bVerifyInput If set as true we verify the users input versus strToVerify.
    /// @return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
    ///
    bool ShowAndVerifyInput(
      std::string&            strToVerify,
      const std::string&      strHeading,
      bool                    bVerifyInput);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to get time value.
    ///
    /// @param[out] time Overwritten with user input if return=true and time inserted.
    /// @param[in] strHeading Heading to display.
    /// @return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* printf */
    /// #include <time.h>       /* time_t, struct tm, time, localtime, strftime */
    /// #include <kodi/api3/gui/DialogNumeric.hpp>
    ///
    /// time_t rawtime;
    /// struct tm * timeinfo;
    /// char buffer [10];
    ///
    /// time (&rawtime);
    /// timeinfo = localtime(&rawtime);
    /// bool bRet = KodiAPI::GUI::DialogNumeric::ShowAndGetTime(*timeinfo, "Selected time test call");
    /// strftime(buffer, sizeof(buffer), "%H:%M.", timeinfo);
    /// printf("Selected time it's %s and was on Dialog %s\n", buffer, bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndGetTime(
      tm&                     time,
      const std::string&      strHeading);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to get date value.
    ///
    /// @param[out] date Overwritten with user input if return=true and date inserted.
    /// @param[in] strHeading Heading to display
    /// @return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <stdio.h>      /* printf */
    /// #include <time.h>       /* time_t, struct tm, time, localtime, strftime */
    /// #include <kodi/api3/gui/DialogNumeric.hpp>
    ///
    /// time_t rawtime;
    /// struct tm * timeinfo;
    /// char buffer [20];
    ///
    /// time (&rawtime);
    /// timeinfo = localtime(&rawtime);
    /// bool bRet = KodiAPI::GUI::DialogNumeric::ShowAndGetDate(*timeinfo, "Selected date test call");
    /// strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    /// printf("Selected date it's %s and was on Dialog %s\n", buffer, bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndGetDate(
      tm&                     date,
      const std::string&      strHeading);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to get a IP
    ///
    /// @param[out] strIPAddress Overwritten with user input if return=true and IP address inserted.
    /// @param[in] strHeading Heading to display.
    /// @return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
    ///
    bool ShowAndGetIPAddress(
      std::string&            strIPAddress,
      const std::string&      strHeading);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Use dialog to get normal number.
    ///
    /// @param[out] strInput Overwritten with user input if return=true and time in seconds inserted
    /// @param[in] strHeading Heading to display
    /// @param[in] iAutoCloseTimeoutMs To close the dialog after a specified time, in milliseconds, default is 0 which keeps the dialog open indefinitely.
    /// @return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    ///   #include <stdio.h>      /* printf */
    ///   #include <stdlib.h>     /* strtoull (C++11) */
    ///   #include <kodi/api3/gui/DialogNumeric.hpp>
    ///
    ///   std::string number;
    ///   bool bRet = KodiAPI::GUI::DialogNumeric::ShowAndGetNumber(number, "Number test call");
    ///   printf("Written number input is : %llu and was %s\n",
    ///                  strtoull(number.c_str(), nullptr, 0), bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndGetNumber(
      std::string&            strInput,
      const std::string&      strHeading,
      unsigned int            iAutoCloseTimeoutMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogNumeric
    /// @brief Show numeric keypad to get seconds.
    ///
    /// @param[out] strTime Overwritten with user input if return=true and time in
    ///                seconds inserted.
    /// @param[in] strHeading Heading to display
    /// @return true if successful display and user input. false if unsuccessful
    ///         display, no user input, or canceled editing.
    ///
    bool ShowAndGetSeconds(
      std::string&            strTime,
      const std::string&      strHeading);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
