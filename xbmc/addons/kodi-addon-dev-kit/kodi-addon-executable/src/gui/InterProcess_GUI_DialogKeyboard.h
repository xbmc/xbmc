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

  struct CKODIAddon_InterProcess_GUI_DialogKeyboard
  {
    bool Dialogs_Keyboard_ShowAndGetInputWithHead(
        std::string&            strText,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        bool                    hiddenInput,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndGetInput(
        std::string&            strText,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndGetNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndVerifyNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs);
    int Dialogs_Keyboard_ShowAndVerifyPassword(
        std::string&            strPassword,
        const std::string&      strHeading,
        int                     iRetries,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_ShowAndGetFilter(
        std::string&            strText,
        bool                    searching,
        unsigned int            autoCloseMs);
    bool Dialogs_Keyboard_SendTextToActiveKeyboard(
        const std::string&      aTextString,
        bool                    closeKeyboard);
    bool Dialogs_Keyboard_isKeyboardActivated();
  };

}; /* extern "C" */
