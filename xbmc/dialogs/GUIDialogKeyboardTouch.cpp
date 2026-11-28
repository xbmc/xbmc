/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogKeyboardTouch.h"

#include "ServiceBroker.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#if defined(TARGET_DARWIN_EMBEDDED)
#include "platform/darwin/ios-common/DarwinEmbedKeyboard.h"
#elif defined(TARGET_ANDROID)
#include "platform/android/activity/AndroidKeyboard.h"
#elif defined(TARGET_WASM)
#include "windowing/wasm/WasmKeyboard.h"
#elif defined(TARGET_WEBOS)
#include "windowing/wayland/WebOSKeyboard.h"
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
#elif defined(TARGET_ANDROID)
  m_keyboard.reset(new CAndroidKeyboard(*this));
#elif defined(TARGET_WASM)
  m_keyboard = std::make_unique<KODI::WINDOWING::WASM::CWasmKeyboard>();
#elif defined(TARGET_WEBOS)
  m_keyboard = std::make_unique<KODI::WINDOWING::WAYLAND::CWebOSKeyboard>(*this);
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

#if defined(TARGET_WEBOS)
  // the dialog can also be closed without the keyboard, e.g. on back or shutdown
  m_keyboard->Cancel();
  StopThread(true);
#endif
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
  if (IsDialogRunning())
    Close(false, 0, true, false);
}

void CGUIDialogKeyboardTouch::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  if (m_keyboard && m_keyboard->ConsumeRenderDirty())
  {
    const auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
    dirtyregions.emplace_back(CRect(0, 0, context.GetWidth(), context.GetHeight()));
  }
  CGUIDialog::Process(currentTime, dirtyregions);
}

void CGUIDialogKeyboardTouch::Render()
{
  CGUIDialog::Render();
  if (m_keyboard)
    m_keyboard->Render();
}
