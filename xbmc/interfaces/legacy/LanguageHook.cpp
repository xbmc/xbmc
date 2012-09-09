/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "LanguageHook.h"
#include "threads/Thread.h"
#include "threads/ThreadLocal.h"
#include "utils/GlobalsHandling.h"

namespace XBMCAddon
{
  // just need a place for the vtab
  LanguageHook::~LanguageHook() {}

  static XbmcThreads::ThreadLocal<LanguageHook> addonLanguageHookTls;
  static bool threadLocalInitilialized = false;
  static xbmcutil::InitFlag initer(threadLocalInitilialized);

  void LanguageHook::setLanguageHook(LanguageHook* languageHook)
  {
    TRACE;
    languageHook->Acquire();
    addonLanguageHookTls.set(languageHook);
  }

  LanguageHook* LanguageHook::getLanguageHook()
  {
    return threadLocalInitilialized ? addonLanguageHookTls.get() : NULL;
  }

  void LanguageHook::clearLanguageHook()
  {
    LanguageHook* lh = addonLanguageHookTls.get();
    addonLanguageHookTls.set(NULL);
    if (lh)
      lh->Release();
  }
}
