/*
 *      Copyright (C) 2012 Team XBMC
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

#pragma once

#include "guilib/GUIKeyboard.h"
#include "guilib/GUIDialog.h"
#include "utils/Variant.h"

enum KEYBOARD {CAPS, LOWER, SYMBOLS };

class CGUIDialogKeyboardGeneric : public CGUIDialog, public CGUIKeyboard
{
  public:
    CGUIDialogKeyboardGeneric();

    //CGUIKeyboard Interface
    virtual bool ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput);
    virtual int GetWindowId() const;

    //CGUIDialog Interface
    virtual void FrameMove();
    void SetHeading(const std::string& heading);
    void SetText(const CStdString& aTextString);
    CStdString GetText() const;
    bool IsConfirmed() { return m_bIsConfirmed; };
    void SetHiddenInput(bool hiddenInput) { m_hiddenInput = hiddenInput; };
    void Character(WCHAR wch);

  protected:
    virtual void OnInitWindow();
    virtual bool OnAction(const CAction &action);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnDeinitWindow(int nextWindowID);
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
    void Backspace();
    void SendSearchMessage();

    CStdStringW m_strEdit;
    bool m_bIsConfirmed;
    KEYBOARD m_keyType;
    int m_iMode;
    bool m_bShift;
    bool m_hiddenInput;

    unsigned int m_lastRemoteClickTime;
    WORD m_lastRemoteKeyClicked;
    int m_indexInSeries;
    std::string m_strHeading;
    static const char* s_charsSeries[10];


    char_callback_t m_pCharCallback;
};
