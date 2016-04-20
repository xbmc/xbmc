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

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace GUI
{
extern "C"
{

  struct CAddOnDialog_Keyboard
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static bool ShowAndGetInput(char         &aTextString,
                                unsigned int &iMaxStringSize,
                                bool          allowEmptyResult,
                                unsigned int  autoCloseMs);

    static bool ShowAndGetInputWithHead(char         &aTextString,
                                        unsigned int &iMaxStringSize,
                                        const char   *heading,
                                        bool          allowEmptyResult,
                                        bool          hiddenInput,
                                        unsigned int  autoCloseMs);

    static bool ShowAndGetNewPassword(char         &strNewPassword,
                                      unsigned int &iMaxStringSize,
                                      unsigned int  autoCloseMs);

    static bool ShowAndGetNewPasswordWithHead(char         &newPassword,
                                              unsigned int &iMaxStringSize,
                                              const char   *strHeading,
                                              bool          allowEmptyResult,
                                              unsigned int  autoCloseMs);

    static bool ShowAndVerifyNewPassword(char         &strNewPassword,
                                         unsigned int &iMaxStringSize,
                                         unsigned int  autoCloseMs);

    static bool ShowAndVerifyNewPasswordWithHead(char         &strNewPassword,
                                                 unsigned int &iMaxStringSize,
                                                 const char   *strHeading,
                                                 bool          allowEmpty,
                                                 unsigned int  autoCloseMs);

    static int  ShowAndVerifyPassword(char         &strPassword,
                                      unsigned int &iMaxStringSize,
                                      const char   *strHeading,
                                      int           iRetries,
                                      unsigned int  autoCloseMs);

    static bool ShowAndGetFilter(char         &aTextString,
                                 unsigned int &iMaxStringSize,
                                 bool          searching,
                                 unsigned int  autoCloseMs);

    static bool SendTextToActiveKeyboard(const char *aTextString,
                                         bool        closeKeyboard);

    static bool isKeyboardActivated();
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
