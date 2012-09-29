/*!
\file GUIEditControl.h
\brief
*/

#ifndef GUILIB_GUIEditControl_H
#define GUILIB_GUIEditControl_H

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

#include "GUIButtonControl.h"
#include "utils/Stopwatch.h"

/*!
 \ingroup controls
 \brief
 */

class CGUIEditControl : public CGUIButtonControl
{
public:
  enum INPUT_TYPE {
                    INPUT_TYPE_READONLY = -1,
                    INPUT_TYPE_TEXT = 0,
                    INPUT_TYPE_NUMBER,
                    INPUT_TYPE_SECONDS,
                    INPUT_TYPE_TIME,
                    INPUT_TYPE_DATE,
                    INPUT_TYPE_IPADDRESS,
                    INPUT_TYPE_PASSWORD,
                    INPUT_TYPE_PASSWORD_MD5,
                    INPUT_TYPE_SEARCH,
                    INPUT_TYPE_FILTER,
                    INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW
                  };

  CGUIEditControl(int parentID, int controlID, float posX, float posY,
                  float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                  const CLabelInfo& labelInfo, const std::string &text);
  CGUIEditControl(const CGUIButtonControl &button);
  virtual ~CGUIEditControl(void);
  virtual CGUIEditControl *Clone() const { return new CGUIEditControl(*this); };

  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnClick();

  virtual void SetLabel(const std::string &text);
  virtual void SetLabel2(const std::string &text);
  void SetHint(const CGUIInfoLabel& hint);

  virtual CStdString GetLabel2() const;

  unsigned int GetCursorPosition() const;
  void SetCursorPosition(unsigned int iPosition);

  void SetInputType(INPUT_TYPE type, int heading);

  void SetTextChangeActions(const CGUIAction& textChangeActions) { m_textChangeActions = textChangeActions; };

  bool HasTextChangeActions() { return m_textChangeActions.HasActionsMeetingCondition(); };

protected:
  virtual void ProcessText(unsigned int currentTime);
  virtual void RenderText();
  CStdStringW GetDisplayedText() const;
  void RecalcLabelPosition();
  void ValidateCursor();
  void UpdateText(bool sendUpdate = true);
  void OnPasteClipboard();
  void OnSMSCharacter(unsigned int key);
  void DefaultConstructor();  

  /*! \brief Clear out the current text input if it's an MD5 password.
   \return true if the password is cleared, false otherwise.
   */
  bool ClearMD5();
  
  CStdStringW m_text2;
  CStdString  m_text;
  CGUIInfoLabel m_hintInfo;
  float m_textOffset;
  float m_textWidth;
  CRect m_clipRect; ///< clipping rect for the second label

  static const int spaceWidth = 5;

  unsigned int m_cursorPos;
  unsigned int m_cursorBlink;

  int m_inputHeading;
  INPUT_TYPE m_inputType;
  bool m_isMD5;

  CGUIAction m_textChangeActions;

  unsigned int m_smsKeyIndex;
  unsigned int m_smsLastKey;
  CStopWatch   m_smsTimer;

  static const char*        smsLetters[10];
  static const unsigned int smsDelay;
};
#endif
