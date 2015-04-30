/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "LanguageHook.h"
#include "threads/ThreadLocal.h"
#include "utils/GlobalsHandling.h"

namespace XBMCAddon
{
  // just need a place for the vtab
  LanguageHook::~LanguageHook() {}

  static XbmcThreads::ThreadLocal<LanguageHook> addonLanguageHookTls;
  static bool threadLocalInitilialized = false;
  static xbmcutil::InitFlag initer(threadLocalInitilialized);

  void LanguageHook::SetLanguageHook(LanguageHook* languageHook)
  {
    XBMC_TRACE;
    languageHook->Acquire();
    addonLanguageHookTls.set(languageHook);
  }

  LanguageHook* LanguageHook::GetLanguageHook()
  {
    return threadLocalInitilialized ? addonLanguageHookTls.get() : NULL;
  }

  void LanguageHook::ClearLanguageHook()
  {
    LanguageHook* lh = addonLanguageHookTls.get();
    addonLanguageHookTls.set(NULL);
    if (lh)
      lh->Release();
  }
}
