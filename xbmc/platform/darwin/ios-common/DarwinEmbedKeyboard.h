/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIKeyboard.h"

struct CDarwinEmbedKeyboardImpl;

class CDarwinEmbedKeyboard : public CGUIKeyboard
{
public:
  CDarwinEmbedKeyboard();
  bool ShowAndGetInput(char_callback_t pCallback,
                       const std::string& initialString,
                       std::string& typedString,
                       const std::string& heading,
                       bool bHiddenInput) override;
  void Cancel() override;
  void fireCallback(const std::string& str);
  void invalidateCallback();
  bool SetTextToKeyboard(const std::string& text, bool closeKeyboard = false) override;

private:
  char_callback_t m_pCharCallback = nullptr;
  bool m_canceled = false;
  std::unique_ptr<CDarwinEmbedKeyboardImpl> m_impl;
};
