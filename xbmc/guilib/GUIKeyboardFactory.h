/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIKeyboard.h"

#include <string>

class CVariant;

class CGUIKeyboardFactory
{

  public:
    CGUIKeyboardFactory(void);
    virtual ~CGUIKeyboardFactory(void);

    static bool ShowAndGetInput(std::string& aTextString, bool allowEmptyResult, unsigned int autoCloseMs = 0);
    static bool ShowAndGetInput(std::string& aTextString, CVariant heading, bool allowEmptyResult, bool hiddenInput = false, unsigned int autoCloseMs = 0);
    static bool ShowAndGetNewPassword(std::string& strNewPassword, unsigned int autoCloseMs = 0);
    static bool ShowAndGetNewPassword(std::string& newPassword, CVariant heading, bool allowEmpty, unsigned int autoCloseMs = 0);
    static bool ShowAndVerifyNewPassword(std::string& strNewPassword, unsigned int autoCloseMs = 0);
    static bool ShowAndVerifyNewPassword(std::string& newPassword, CVariant heading, bool allowEmpty, unsigned int autoCloseMs = 0);
    static int  ShowAndVerifyPassword(std::string& strPassword, const std::string& strHeading, int iRetries, unsigned int autoCloseMs = 0);
    static bool ShowAndGetFilter(std::string& aTextString, bool searching, unsigned int autoCloseMs = 0);

    static bool SendTextToActiveKeyboard(const std::string &aTextString, bool closeKeyboard = false);

    static bool isKeyboardActivated() { return g_activeKeyboard != NULL; }
  private:
    static CGUIKeyboard *g_activeKeyboard;
    static FILTERING m_filtering;
    static void keyTypedCB(CGUIKeyboard *ref, const std::string &typedString);
};
