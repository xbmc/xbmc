/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LanguageHook.h"
#include "utils/GlobalsHandling.h"

namespace XBMCAddon
{
  // just need a place for the vtab
  LanguageHook::~LanguageHook() = default;

  static thread_local LanguageHook* addonLanguageHookTls;
  static bool threadLocalInitialized = false;
  static xbmcutil::InitFlag initer(threadLocalInitialized);

  void LanguageHook::SetLanguageHook(LanguageHook* languageHook)
  {
    XBMC_TRACE;
    languageHook->Acquire();
    addonLanguageHookTls = languageHook;
  }

  LanguageHook* LanguageHook::GetLanguageHook()
  {
    return threadLocalInitialized ? addonLanguageHookTls : NULL;
  }

  void LanguageHook::ClearLanguageHook()
  {
    LanguageHook* lh = addonLanguageHookTls;
    addonLanguageHookTls = NULL;
    if (lh)
      lh->Release();
  }
}
