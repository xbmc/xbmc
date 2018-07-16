/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"

class CGUIDialogGamepad :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogGamepad(void);
  ~CGUIDialogGamepad(void) override;
  bool OnMessage(CGUIMessage& message) override;
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
  bool OnAction(const CAction &action) override;
  void OnInitWindow() override;
  bool m_bCanceled;
  char m_cHideInputChar;
};
