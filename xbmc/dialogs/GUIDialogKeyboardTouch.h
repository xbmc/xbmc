/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
