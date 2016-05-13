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
  /// \defgroup CPP_KodiAPI_GUI_DialogKeyboard Dialog Keyboard
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Keyboard dialogs</b>
  ///
  /// The functions listed below have to be permitted by the user for the
  /// representation of a keyboard around an input.
  ///
  /// The class supports several kinds, from an easy text choice up to the
  /// passport Word production and their confirmation for add-on.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogKeyboard.hpp "#include <kodi/api3/gui/DialogKeyboard.hpp>"
  /// be included to enjoy it.
  ///
  namespace DialogKeyboard
  {
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Show keyboard with initial value `text` and replace with  result
    /// string.
    ///
    /// @param[out] text             Overwritten with user input if return=true.
    /// @param[in] heading           String shown on dialog title.
    /// @param[in] allowEmptyResult  Whether a blank password is valid or not.
    /// @param[in] hiddenInput       The inserted input is not shown as text.
    /// @param[in] autoCloseMs       To  close  the  dialog  after  a  specified
    ///                              time, in milliseconds, default is  0  which
    ///                              keeps the dialog open indefinitely.
    /// @return                      true if successful display and user  input.
    ///                              false  if  unsuccessful  display,  no  user
    ///                              input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/gui/DialogKeyboard.hpp>
    ///
    /// /*
    ///  * The example shows the display of keyboard call dialog at Kodi from the add-on.
    ///  * Below all values are set, however, can last two (hidden input = false and autoCloseMs = 0)
    ///  * to be released if not needed.
    ///  */
    /// std::string text = "Please change me to them want you want"; /*< It can be leaved empty or a
    ///                                                                  entry text added */
    /// bool bRet = KodiAPI::GUI::DialogKeyboard::ShowAndGetInput(text,
    ///                                                      "Demonstration text entry",
    ///                                                      true,
    ///                                                      false,
    ///                                                      0);
    /// fprintf(stderr, "Written keyboard input is : %s and was %s\n",
    ///                   text.c_str(), bRet ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndGetInput(
      std::string&            text,
      const std::string&      heading,
      bool                    allowEmptyResult,
      bool                    hiddenInput = false,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief The example shows the display of keyboard  call dialog  at  Kodi
    /// from the add-on.
    ///
    /// @param[out] text            Overwritten with user input if return=true.
    /// @param[in] allowEmptyResult If  set  to true  keyboard can  also  exited
    ///                             without entered text.
    /// @param[in] autoCloseMs      To close the dialog after a specified  time,
    ///                             in milliseconds,  default is  0 which  keeps
    ///                             the dialog open indefinitely.
    /// @return                     true if successful display and user input.
    ///                             false  if  unsuccessful   display,  no  user
    ///                             input, or canceled editing.
    ///
    bool ShowAndGetInput(
      std::string&            text,
      bool                    allowEmptyResult,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Shows  keyboard  and  prompts  for  a  password.   Differs  from
    /// `ShowAndVerifyNewPassword()` in that no second verification
    ///
    /// @param[out] newPassword      Overwritten with user input if return=true.
    /// @param[in] heading           String shown on dialog title.
    /// @param[in] allowEmptyResult  Whether a blank password is valid or not.
    /// @param[in] autoCloseMs       To close the dialog after a specified time,
    ///                              in milliseconds, default is  0  which keeps
    ///                              the dialog open indefinitely.
    /// @return                      true if successful display  and user input.
    ///                              false  if  unsuccessful  display,  no  user
    ///                              input, or canceled editing.
    ///
    bool ShowAndGetNewPassword(
      std::string&            newPassword,
      const std::string&      heading,
      bool                    allowEmptyResult,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Shows keyboard and prompts for a password. Differs from
    /// `ShowAndVerifyNewPassword()` in that no second verification
    ///
    /// @param[out] newPassword    Overwritten with user input if  return=true.
    /// @param[in] autoCloseMs     To close the dialog after a  specified  time,
    ///                            in milliseconds, default  is  0  which  keeps
    ///                            the dialog open indefinitely.
    /// @return                    true if successful display  and  user  input.
    ///                            false  if  unsuccessful  display,   no  user
    ///                            input, or canceled editing.
    ///
    bool ShowAndGetNewPassword(
      std::string&            newPassword,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Show keyboard twice to  get and confirm a  user-entered  password
    /// string.
    ///
    /// @param[out] newPassword    Overwritten with user input if return=true.
    /// @param[in] heading         String shown on dialog title.
    /// @param[in] allowEmptyResult
    /// @param[in] autoCloseMs     To close the dialog after a  specified  time,
    ///                            in milliseconds,  default  is 0  which  keeps
    ///                            the dialog open indefinitely.
    /// @return                    true if successful display  and  user  input.
    ///                            false  if  unsuccessful   display,   no  user
    ///                            input, or canceled editing.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// #include <kodi/api3/gui/DialogKeyboard.hpp>
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
    ///   KodiAPI::AddOn::General::GetMD5("kodi", password);
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
    ///     ret = KodiAPI::GUI::DialogKeyboard::ShowAndVerifyPassword(password, "Demo password call for PW 'kodi'", i, 0);
    ///     if (ret == 0)
    ///     {
    ///       fprintf(stderr, "Password successfull confirmed after '%i' tries\n", i+1);
    ///       break;
    ///     }
    ///     else if (ret < 0)
    ///     {
    ///       fprintf(stderr, "Canceled editing on try '%i'\n", i+1);
    ///       break;
    ///     }
    ///     else /* if (ret > 0) */
    ///     {
    ///       fprintf(stderr, "Wrong password entered on try '%i'\n", i+1);
    ///     }
    ///   }
    /// }
    /// else
    ///   fprintf(stderr, "Requested global setting value 'masterlock.maxretries' not present!");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndVerifyNewPassword(
      std::string&            newPassword,
      const std::string&      heading,
      bool                    allowEmptyResult,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Show keyboard twice to get and confirm  a user-entered  password
    /// string.
    ///
    /// @param[out] newPassword    Overwritten with user input if return=true.
    /// @param[in] autoCloseMs     To close the dialog after a specified   time,
    ///                            in milliseconds, default  is  0  which  keeps
    ///                            the dialog open indefinitely.
    /// @return                    true if successful display  and  user  input.
    ///                            false  if  unsuccessful   display,   no  user
    ///                            input, or canceled editing.
    ///
    bool ShowAndVerifyNewPassword(
      std::string&            newPassword,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Show keyboard and verify user input against `password`.
    ///
    /// @param[out] password       Value to compare against user input.
    /// @param[in] heading         String shown on dialog title.
    /// @param[in] iRetries        If   greater   than   0,   shows   "Incorrect
    ///                            password,  %d retries left" on dialog line 2,
    ///                            else line 2 is blank.
    /// @param[in] autoCloseMs     To close the dialog  after a specified  time,
    ///                            in milliseconds,  default is  0  which  keeps
    ///                            the dialog open indefinitely.
    /// @return                    0 if successful display  and user input. 1 if
    ///                            unsuccessful input. -1 if no user  input  or
    ///                            canceled editing.
    ///
    int ShowAndVerifyPassword(
      std::string&            password,
      const std::string&      heading,
      int                     iRetries,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Shows a filter related keyboard
    ///
    /// @param[out] text           Overwritten with user input if  return=true.
    /// @param[in] searching       Use dialog for  search and  send  our  search
    ///                            message in safe way (only  the active  window
    ///                            needs it)
    ///  - header name if true is "Enter search string"
    ///  - header name if false is "Enter value"
    /// @param autoCloseMs         To close the dialog after  a specified  time,
    ///                            in milliseconds, default  is  0  which  keeps
    ///                            the dialog open indefinitely.
    /// @return                    true if successful display  and  user  input.
    ///                            false   if  unsuccessful  display,   no  user
    ///                            input, or canceled editing.
    ///
    bool ShowAndGetFilter(
      std::string&            text,
      bool                    searching,
      unsigned int            autoCloseMs = 0);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Send a text to a visible keyboard
    ///
    /// @param[in] text            Overwritten with user input if  return=true.
    /// @param[in] closeKeyboard   The open dialog is if also closed on 'true'.
    /// @return                    true   if    successful   done,    false   if
    ///                            unsuccessful or keyboard not present.
    ///
    bool SendTextToActiveKeyboard(
      const std::string&      text,
      bool                    closeKeyboard = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogKeyboard
    /// @brief Check for visible keyboard on GUI
    ///
    /// @return  true if keyboard present, false if not present
    ///
    bool IsKeyboardActivated();
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
