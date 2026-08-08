/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIKeyboard.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#include "platform/android/activity/JNIXBMCNativeKeyboard.h"

#include <memory>
#include <string>

/*!
 * \brief Native Android on-screen keyboard implementation.
 *
 * Wraps the Java soft keyboard dialog (jni::CJNIXBMCNativeKeyboard) and blocks
 * the calling thread until the user has either confirmed or canceled the
 * input, as required by the CGUIKeyboard interface.
 */
class CAndroidKeyboard : public CGUIKeyboard, public jni::IKeyboardInputHandler
{
public:
  CAndroidKeyboard() = default;
  ~CAndroidKeyboard() override = default;

  // implementation of CGUIKeyboard
  bool ShowAndGetInput(char_callback_t pCallback,
                       const std::string& initialString,
                       std::string& typedString,
                       const std::string& heading,
                       bool bHiddenInput) override;
  void Cancel() override;
  bool SetTextToKeyboard(const std::string& text, bool closeKeyboard = false) override;

  // implementation of jni::IKeyboardInputHandler
  void OnTextChanged(const std::string& text) override;
  void OnInputFinished(const std::string& text) override;
  void OnInputCanceled() override;

private:
  std::unique_ptr<jni::CJNIXBMCNativeKeyboard> m_keyboard;
  char_callback_t m_pCallback{nullptr};

  CCriticalSection m_critSection;
  CEvent m_inputFinishedEvent;
  std::string m_text;
  bool m_confirmed{false};

  //! \brief Set by SetTextToKeyboard() when closeKeyboard is true.
  //!
  //! The Java bridge only exposes hide(), which always dismisses the dialog as if
  //! canceled (see XBMCNativeKeyboard.java.in cancelInput()). To still honor the
  //! CGUIKeyboard::SetTextToKeyboard(text, closeKeyboard=true) contract of confirming
  //! and closing (as done by CDarwinEmbedKeyboard on iOS/tvOS), the resulting
  //! OnInputCanceled() callback is reinterpreted as a confirmation whenever this flag
  //! is set.
  bool m_confirmOnCancel{false};
};
