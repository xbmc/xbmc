/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://kodi.tv
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

#include "GUIDialogKeyboardTouch.h"
#if defined(TARGET_DARWIN_IOS)
#include "platform/darwin/ios/IOSKeyboard.h"
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
#if defined(TARGET_DARWIN_IOS)
  m_keyboard.reset(new CIOSKeyboard());
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
  return;
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