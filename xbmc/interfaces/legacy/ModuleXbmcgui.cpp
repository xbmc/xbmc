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

#include "ModuleXbmcgui.h"
#include "LanguageHook.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

#define NOTIFICATION_INFO     "info"
#define NOTIFICATION_WARNING  "warning"
#define NOTIFICATION_ERROR    "error"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void lock()
    {
      CLog::Log(LOGWARNING,"'xbmcgui.lock()' is deprecated and serves no purpose anymore, it will be removed in future releases");
    }

    void unlock()
    {
      CLog::Log(LOGWARNING,"'xbmcgui.unlock()' is deprecated and serves no purpose anymore, it will be removed in future releases");
    }

    long getCurrentWindowId()
    {
      DelayedCallGuard dg;
      CSingleLock gl(g_graphicsContext);
      return g_windowManager.GetActiveWindow();
    }

    long getCurrentWindowDialogId()
    {
      DelayedCallGuard dg;
      CSingleLock gl(g_graphicsContext);
      return g_windowManager.GetTopMostModalDialogID();
    }
    
    const char* getNOTIFICATION_INFO()    { return NOTIFICATION_INFO; }
    const char* getNOTIFICATION_WARNING() { return NOTIFICATION_WARNING; }
    const char* getNOTIFICATION_ERROR()   { return NOTIFICATION_ERROR; }

  }
}
