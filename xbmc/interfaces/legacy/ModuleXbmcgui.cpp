/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ModuleXbmcgui.h"

#include "LanguageHook.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/GraphicContext.h"

#include <mutex>

#define NOTIFICATION_INFO     "info"
#define NOTIFICATION_WARNING  "warning"
#define NOTIFICATION_ERROR    "error"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    long getCurrentWindowId()
    {
      DelayedCallGuard dg;
      std::unique_lock<CCriticalSection> gl(CServiceBroker::GetWinSystem()->GetGfxContext());
      return CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    }

    long getCurrentWindowDialogId()
    {
      DelayedCallGuard dg;
      std::unique_lock<CCriticalSection> gl(CServiceBroker::GetWinSystem()->GetGfxContext());
      return CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog();
    }

    long getScreenHeight()
    {
      XBMC_TRACE;
      return CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
    }

    long getScreenWidth()
    {
      XBMC_TRACE;
      return CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
    }

    const char* getNOTIFICATION_INFO()    { return NOTIFICATION_INFO; }
    const char* getNOTIFICATION_WARNING() { return NOTIFICATION_WARNING; }
    const char* getNOTIFICATION_ERROR()   { return NOTIFICATION_ERROR; }

  }
}
