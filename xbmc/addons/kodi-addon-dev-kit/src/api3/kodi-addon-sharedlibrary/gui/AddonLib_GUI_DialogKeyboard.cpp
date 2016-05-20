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
#include KITINCLUDE(ADDON_API_LEVEL, gui/DialogKeyboard.hpp)

API_NAMESPACE

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
    strText.resize(1024);
    unsigned int size = (unsigned int)strText.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetInputWithHead(strText[0], size, strHeading.c_str(), allowEmptyResult, hiddenInput, autoCloseMs);
    strText.resize(size);
    strText.shrink_to_fit();
    return ret;
  }

  bool ShowAndGetInput(
        std::string&            strText,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    strText.resize(1024);
    unsigned int size = (unsigned int)strText.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetInput(strText[0], size, allowEmptyResult, autoCloseMs);
    strText.resize(size);
    strText.shrink_to_fit();
    return ret;
  }

  bool ShowAndGetNewPassword(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    strNewPassword.resize(1024);
    unsigned int size = (unsigned int)strNewPassword.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetNewPasswordWithHead(strNewPassword[0], size, strHeading.c_str(), allowEmptyResult, autoCloseMs);
    strNewPassword.resize(size);
    strNewPassword.shrink_to_fit();
    return ret;
  }

  bool ShowAndGetNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs)
  {
    strNewPassword.resize(1024);
    unsigned int size = (unsigned int)strNewPassword.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetNewPassword(strNewPassword[0], size, autoCloseMs);
    strNewPassword.resize(size);
    strNewPassword.shrink_to_fit();
    return ret;
  }

  bool ShowAndVerifyNewPassword(
        std::string&            strNewPassword,
        const std::string&      strHeading,
        bool                    allowEmptyResult,
        unsigned int            autoCloseMs)
  {
    strNewPassword.resize(1024);
    unsigned int size = (unsigned int)strNewPassword.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetNewPasswordWithHead(strNewPassword[0], size, strHeading.c_str(), allowEmptyResult, autoCloseMs);
    strNewPassword.resize(size);
    strNewPassword.shrink_to_fit();
    return ret;
  }

  bool ShowAndVerifyNewPassword(
        std::string&            strNewPassword,
        unsigned int            autoCloseMs)
  {
    strNewPassword.resize(1024);
    unsigned int size = (unsigned int)strNewPassword.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndVerifyNewPassword(strNewPassword[0], size, autoCloseMs);
    strNewPassword.resize(size);
    strNewPassword.shrink_to_fit();
    return ret;
  }

  int ShowAndVerifyPassword(
        std::string&            strPassword,
        const std::string&      strHeading,
        int                     iRetries,
        unsigned int            autoCloseMs)
  {
    strPassword.resize(1024);
    unsigned int size = (unsigned int)strPassword.capacity();
    int ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndVerifyPassword(strPassword[0], size, strHeading.c_str(), iRetries, autoCloseMs);
    strPassword.resize(size);
    strPassword.shrink_to_fit();
    return ret;
  }

  bool ShowAndGetFilter(
        std::string&            strText,
        bool                    searching,
        unsigned int            autoCloseMs)
  {
    strText.resize(1024);
    unsigned int size = (unsigned int)strText.capacity();
    bool ret = g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.ShowAndGetFilter(strText[0], size, searching, autoCloseMs);
    strText.resize(size);
    strText.shrink_to_fit();
    return ret;
  }

  bool SendTextToActiveKeyboard(
        const std::string&      aTextString,
        bool                    closeKeyboard)
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.SendTextToActiveKeyboard(aTextString.c_str(), closeKeyboard);
  }

  bool IsKeyboardActivated()
  {
    return g_interProcess.m_Callbacks->GUI.Dialogs.Keyboard.isKeyboardActivated();
  }

} /* namespace DialogKeyboard */
} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
