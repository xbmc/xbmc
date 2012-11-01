#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

    static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult);
    static bool ShowAndGetInput(CStdString& aTextString, const CVariant &heading, bool allowEmptyResult, bool hiddenInput = false);
    static bool ShowAndGetNewPassword(CStdString& strNewPassword);
    static bool ShowAndGetNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty);
    static bool ShowAndVerifyNewPassword(CStdString& strNewPassword);
    static bool ShowAndVerifyNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty);
    static int  ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries);
    static bool ShowAndGetFilter(CStdString& aTextString, bool searching);

  private:
    static FILTERING m_filtering;
    static void keyTypedCB(CGUIKeyboard *ref, const std::string &typedString);
};
