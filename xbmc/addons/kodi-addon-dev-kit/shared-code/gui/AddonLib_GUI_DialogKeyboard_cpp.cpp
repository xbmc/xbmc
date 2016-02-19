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
#include "kodi/api2/gui/DialogKeyboard.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
namespace DialogKeyboard
{

  bool ShowAndGetInput(
        std::string&            strText,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        bool                    hiddenInput,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndGetInputWithHead(strText, strHeading, allowEmptyResult, hiddenInput, autoCloseMs);
  }
        
  bool ShowAndGetInput(
        std::string&            strText,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndGetInput(strText, allowEmptyResult, autoCloseMs);
  }

  bool ShowAndGetNewPassword(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(strNewPassword, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool ShowAndGetNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndGetNewPassword(strNewPassword, autoCloseMs);
  }

  bool ShowAndVerifyNewPassword(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(strNewPassword, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool ShowAndVerifyNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndVerifyNewPassword(strNewPassword, autoCloseMs);
  }
    
  int ShowAndVerifyPassword(
        std::string&            strPassword,
        const std::string&      strHeading,
        int                     iRetries,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndVerifyPassword(strPassword, strHeading, iRetries, autoCloseMs);
  }

  bool ShowAndGetFilter(
        std::string&            strText,
        bool                    searching,
        unsigned int            autoCloseMs)
  {
    return g_interProcess.Dialogs_Keyboard_ShowAndGetFilter(strText, searching, autoCloseMs);
  }

  bool SendTextToActiveKeyboard(
        const std::string&      aTextString,
        bool                    closeKeyboard)
  {
    return g_interProcess.Dialogs_Keyboard_SendTextToActiveKeyboard(aTextString, closeKeyboard);
  }
  
  bool IsKeyboardActivated()
  {
    return g_interProcess.Dialogs_Keyboard_isKeyboardActivated();
  }

}; /* namespace DialogKeyboard */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
