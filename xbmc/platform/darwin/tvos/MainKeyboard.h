/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIKeyboard.h"

class CMainKeyboard : public CGUIKeyboard
{
  public:
    CMainKeyboard():m_pCharCallback(nullptr),m_bCanceled(false){}
    virtual bool ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput);
    virtual void Cancel();
    void fireCallback(const std::string &str);
    void invalidateCallback(){m_pCharCallback = nullptr;}
    virtual bool SetTextToKeyboard(const std::string &text, bool closeKeyboard = false);

  private:
    char_callback_t m_pCharCallback;
    bool m_bCanceled;
};
