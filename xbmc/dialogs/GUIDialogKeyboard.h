#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "guilib/GUIDialog.h"
#include "utils/Variant.h"

enum KEYBOARD {CAPS, LOWER, SYMBOLS };

class CGUIDialogKeyboard: public CGUIDialog
{

public:
  CGUIDialogKeyboard(void);
  virtual ~CGUIDialogKeyboard(void);

  virtual void FrameMove();
  void SetHeading(const CVariant& heading);
  void SetText(const CStdString& aTextString);
  CStdString GetText() const;
  bool IsConfirmed() { return m_bIsConfirmed; };
  void SetHiddenInput(bool hiddenInput) { m_hiddenInput = hiddenInput; };

  static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult);
  static bool ShowAndGetInput(CStdString& aTextString, const CVariant &heading, bool allowEmptyResult, bool hiddenInput = false);
  static bool ShowAndGetNewPassword(CStdString& strNewPassword);
  static bool ShowAndGetNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty);
  static bool ShowAndVerifyNewPassword(CStdString& strNewPassword);
  static bool ShowAndVerifyNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries);
  static bool ShowAndGetFilter(CStdString& aTextString, bool searching);

  virtual void Close(bool forceClose = false);

protected:
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  void SetControlLabel(int id, const CStdString &label);
  void OnShift();
  void MoveCursor(int iAmount);
  int GetCursorPos() const;
  void OnSymbols();
  void OnIPAddress();
  void OnOK();

private:

  void OnClickButton(int iButtonControl);
  void OnRemoteNumberClick(int key);
  void UpdateButtons();
  char GetCharacter(int iButton);
  void UpdateLabel();
  void ResetShiftAndSymbols();

  void Character(WCHAR wch);
  void Backspace();
  void SendSearchMessage();

  CStdStringW m_strEdit;
  bool m_bIsConfirmed;
  KEYBOARD m_keyType;
  int m_iMode;
  bool m_bShift;
  bool m_hiddenInput;
  // filtering type
  enum FILTERING { FILTERING_NONE = 0, FILTERING_CURRENT, FILTERING_SEARCH };
  FILTERING m_filtering;

  unsigned int m_lastRemoteClickTime;
  WORD m_lastRemoteKeyClicked;
  int m_indexInSeries;
  CStdString m_strHeading;
  static const char* s_charsSeries[10];
};
