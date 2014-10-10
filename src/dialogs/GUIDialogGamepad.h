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

#include "GUIDialogBoxBase.h"

class CGUIDialogGamepad :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogGamepad(void);
  virtual ~CGUIDialogGamepad(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool IsCanceled() const;
  std::string m_strUserInput;
  std::string m_strPassword;
  int m_iRetries;
  bool m_bUserInputCleanup;
  bool m_bHideInputChars;
  static bool ShowAndGetInput(std::string& aTextString, const std::string& dlgHeading, bool bHideUserInput);
  static bool ShowAndVerifyNewPassword(std::string& strNewPassword);
  static int ShowAndVerifyPassword(std::string& strPassword, const std::string& dlgHeading, int iRetries);
  static bool ShowAndVerifyInput(std::string& strPassword, const std::string& dlgHeading, const std::string& dlgLine0, const std::string& dlgLine1, const std::string& dlgLine2, bool bGetUserInput, bool bHideInputChars);
protected:
  virtual bool OnAction(const CAction &action);
  bool m_bCanceled;
  char m_cHideInputChar;
};
