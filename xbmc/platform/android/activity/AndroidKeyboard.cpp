/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidKeyboard.h"

bool CAndroidKeyboard::ShowAndGetInput(char_callback_t pCallback,
                                       const std::string& initialString,
                                       std::string& typedString,
                                       const std::string& heading,
                                       bool bHiddenInput)
{
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_pCallback = pCallback;
    m_text = initialString;
    m_confirmed = false;
    m_confirmOnCancel = false;
    m_inputFinishedEvent.Reset();
    m_keyboard = std::make_unique<jni::CJNIXBMCNativeKeyboard>(this);
  }

  m_keyboard->show(heading, initialString, bHiddenInput);

  // block until either OnInputFinished() or OnInputCanceled() fires (always exactly
  // one of the two, guaranteed by the Java bridge), both coming from the Android UI thread
  m_inputFinishedEvent.Wait();

  std::unique_lock<CCriticalSection> lock(m_critSection);
  bool confirmed = m_confirmed;
  if (confirmed)
    typedString = m_text;

  m_keyboard.reset();
  m_pCallback = nullptr;

  return confirmed;
}

void CAndroidKeyboard::Cancel()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_keyboard)
    m_keyboard->hide();
}

bool CAndroidKeyboard::SetTextToKeyboard(const std::string& text, bool closeKeyboard /* = false */)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_keyboard)
    return false;

  m_keyboard->setText(text);
  m_text = text;

  if (closeKeyboard)
  {
    // hide() always results in OnInputCanceled() (see comment on m_confirmOnCancel),
    // so remember that this particular cancellation should be treated as a confirmation
    m_confirmOnCancel = true;
    m_keyboard->hide();
  }

  return true;
}

void CAndroidKeyboard::OnTextChanged(const std::string& text)
{
  char_callback_t callback{nullptr};
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_text = text;
    callback = m_pCallback;
  }

  if (callback)
    callback(this, text);
}

void CAndroidKeyboard::OnInputFinished(const std::string& text)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_text = text;
  m_confirmed = true;
  m_inputFinishedEvent.Set();
}

void CAndroidKeyboard::OnInputCanceled()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_confirmed = m_confirmOnCancel;
  m_confirmOnCancel = false;
  m_inputFinishedEvent.Set();
}
