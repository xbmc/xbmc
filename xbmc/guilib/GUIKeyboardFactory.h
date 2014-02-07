#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialog.h"
#include "utils/Variant.h"
#include "GUIKeyboard.h"

class CGUIKeyboardFactory
{

  public:
    CGUIKeyboardFactory(void);
    virtual ~CGUIKeyboardFactory(void);

    static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult, unsigned int autoCloseMs = 0);
    static bool ShowAndGetInput(CStdString& aTextString, const CVariant &heading, bool allowEmptyResult, bool hiddenInput = false, unsigned int autoCloseMs = 0);
    static bool ShowAndGetNewPassword(CStdString& strNewPassword, unsigned int autoCloseMs = 0);
    static bool ShowAndGetNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty, unsigned int autoCloseMs = 0);
    static bool ShowAndVerifyNewPassword(CStdString& strNewPassword, unsigned int autoCloseMs = 0);
    static bool ShowAndVerifyNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty, unsigned int autoCloseMs = 0);
    static int  ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries, unsigned int autoCloseMs = 0);
    static bool ShowAndGetFilter(CStdString& aTextString, bool searching, unsigned int autoCloseMs = 0);

    static bool SendTextToActiveKeyboard(const std::string &aTextString, bool closeKeyboard = false);

    static bool isKeyboardActivated() { return g_activedKeyboard != NULL; }
  private:
    static CGUIKeyboard *g_activedKeyboard;
    static FILTERING m_filtering;
    static void keyTypedCB(CGUIKeyboard *ref, const std::string &typedString);
};
