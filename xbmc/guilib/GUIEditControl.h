/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIEditControl.h
\brief
*/

#include "GUIButtonControl.h"
#include "utils/Stopwatch.h"
#include "utils/StringValidation.h"
#include "utils/Variant.h"

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
  explicit CGUIEditControl(const CGUIButtonControl& button);
  explicit CGUIEditControl(const CGUIEditControl& button);
  ~CGUIEditControl(void) override;
  CGUIEditControl* Clone() const override { return new CGUIEditControl(*this); }

  bool OnMessage(CGUIMessage &message) override;
  bool OnAction(const CAction &action) override;
  void OnClick() override;

  void SetLabel(const std::string &text) override;
  void SetLabel2(const std::string &text) override;
  void SetHint(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& hint);

  std::string GetLabel2() const override;

  unsigned int GetCursorPosition() const;
  void SetCursorPosition(unsigned int iPosition);

  void SetInputType(INPUT_TYPE type, const CVariant& heading);

  void SetTextChangeActions(const CGUIAction& textChangeActions)
  {
    m_textChangeActions = textChangeActions;
  }

  bool HasTextChangeActions() const { return m_textChangeActions.HasActionsMeetingCondition(); }

  virtual bool HasInvalidInput() const { return m_invalidInput; }
  virtual void SetInputValidation(StringValidation::Validator inputValidator, void *data = NULL);

protected:
  void SetFocus(bool focus) override;
  void ProcessText(unsigned int currentTime) override;
  void RenderText() override;
  CGUILabel::COLOR GetTextColor() const override;
  std::wstring GetDisplayedText() const;
  std::string GetDescriptionByIndex(int index) const override;
  bool SetStyledText(const std::wstring &text);

  /*!
   * \brief Recalculate the text offset position for the right label
   *        by updating m_textOffset and validate the cursor position.
   */
  void RecalcRightLabelPosition();

  void ValidateCursor();
  void UpdateText(bool sendUpdate = true);
  void OnPasteClipboard();
  void OnSMSCharacter(unsigned int key);
  void DefaultConstructor();

  virtual bool ValidateInput(const std::wstring &data) const;
  void ValidateInput();

  /*! \brief Clear out the current text input if it's an MD5 password.
   \return true if the password is cleared, false otherwise.
   */
  bool ClearMD5();

  /*! \brief Append a given char to the composing cursor
   * \param deadUnicodeKey - the unicode key that started the composing sequence
   */
  void ComposingCursorAppendChar(std::uint32_t deadUnicodeKey);
  /*! \brief Reset the cursor aspect to normal input (i.e. not composing a key)
   */
  void ResetCursor();
  /*! \brief Cancel the key composition
   * \param deadUnicodeKey - the unicode key that ended/cancelled the composing sequence
   */
  void CancelKeyComposition(std::uint32_t deadUnicodeKey);
  /*! \brief Check if the control is composing a key
   * \return true if a key is being composed, false otherwise
   */
  bool IsComposingKey() const;

  std::wstring m_text2;
  std::string  m_text;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_hintInfo;
  float m_textOffset;
  CRect m_clipRect; ///< clipping rect for the second label

  unsigned int m_cursorPos;
  bool m_cursorBlinkEnabled{true};
  unsigned int m_cursorBlink;
  // visible cursor chars
  std::vector<std::uint32_t> m_cursorChars{'|'};
  // cursor char buffer
  std::vector<std::uint32_t> m_cursorCharsBuffer{};

  std::string m_inputHeading;
  INPUT_TYPE m_inputType;
  bool m_isMD5;

  CGUIAction m_textChangeActions;

  bool m_invalidInput;
  StringValidation::Validator m_inputValidator;
  void *m_inputValidatorData;

  unsigned int m_smsKeyIndex;
  unsigned int m_smsLastKey;
  CStopWatch   m_smsTimer;

  std::wstring m_edit;
  int          m_editOffset;
  int          m_editLength;
};
