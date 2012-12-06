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

#include "ModuleXbmcgui.h"
#include "LanguageHook.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void lock()
    {
      CLog::Log(LOGWARNING,"'xbmcgui.lock()' is depreciated and serves no purpose anymore, it will be removed in future releases");
    }

    void unlock()
    {
      CLog::Log(LOGWARNING,"'xbmcgui.unlock()' is depreciated and serves no purpose anymore, it will be removed in future releases");
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
  }
}
