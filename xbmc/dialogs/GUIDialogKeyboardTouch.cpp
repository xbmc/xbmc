/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogKeyboardTouch.h"
#if defined(TARGET_DARWIN_EMBEDDED)
#include "platform/darwin/ios-common/DarwinEmbedKeyboard.h"
#endif

CGUIDialogKeyboardTouch::CGUIDialogKeyboardTouch()
: CGUIDialog(WINDOW_DIALOG_KEYBOARD_TOUCH, "")
, CGUIKeyboard()
, CThread("keyboard")
, m_pCharCallback(NULL)
{
}

bool CGUIDialogKeyboardTouch::ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput)
{
#if defined(TARGET_DARWIN_EMBEDDED)
  m_keyboard.reset(new CDarwinEmbedKeyboard());
#endif

  if (!m_keyboard)
    return false;

  m_pCharCallback = pCallback;
  m_initialString = initialString;
  m_typedString = typedString;
  m_heading = heading;
  m_bHiddenInput = bHiddenInput;

  m_confirmed = false;
  Initialize();
  Open();

  m_keyboard.reset();

  if (m_confirmed)
  {
    typedString = m_typedString;
    return true;
  }

  return false;
}

bool CGUIDialogKeyboardTouch::SetTextToKeyboard(const std::string &text, bool closeKeyboard)
{
  if (m_keyboard)
    return m_keyboard->SetTextToKeyboard(text, closeKeyboard);

  return false;
}

void CGUIDialogKeyboardTouch::Cancel()
{
  if (m_keyboard)
    m_keyboard->Cancel();
}

int CGUIDialogKeyboardTouch::GetWindowId() const
{
  return GetID();
}

void CGUIDialogKeyboardTouch::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  m_windowLoaded = true;
  m_active = true;
  Create();
}

void CGUIDialogKeyboardTouch::Process()
{
  if (m_keyboard)
  {
    m_confirmed = m_keyboard->ShowAndGetInput(m_pCharCallback, m_initialString, m_typedString, m_heading, m_bHiddenInput);
  }
  Close();
}