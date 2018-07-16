/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "guilib/GUIKeyboard.h"
#include <atomic>
#include <memory>

class CGUIDialogKeyboardTouch : public CGUIDialog, public CGUIKeyboard, public CThread
{
public:
  CGUIDialogKeyboardTouch();
  bool ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput) override;
  bool SetTextToKeyboard(const std::string &text, bool closeKeyboard = false) override;
  void Cancel() override;
  int GetWindowId() const override;

protected:
  void OnInitWindow() override;
  using CGUIControlGroup::Process;
  void Process() override;

  char_callback_t m_pCharCallback;
  std::string m_initialString;
  std::string m_typedString;
  std::string m_heading;
  bool m_bHiddenInput;

  std::unique_ptr<CGUIKeyboard> m_keyboard;
  std::atomic_bool m_active;
  bool m_confirmed;
};
