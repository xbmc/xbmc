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

#include "WindowDialog.h"

#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "WindowInterceptor.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    WindowDialog::WindowDialog() :
      Window(true), WindowDialogMixin(this)
    {
      CSingleLock lock(g_graphicsContext);
      setWindow(new Interceptor<CGUIWindow>("CGUIWindow",this,getNextAvailalbeWindowId()));
    }

    WindowDialog::~WindowDialog() { deallocating(); }

    bool WindowDialog::OnMessage(CGUIMessage& message)
    {
#ifdef ENABLE_XBMC_TRACE_API
      XBMC_TRACE;
      CLog::Log(LOGDEBUG,"%sNEWADDON WindowDialog::OnMessage Message %d", _tg.getSpaces(),message.GetMessage());
#endif

      switch(message.GetMessage())
      {
      case GUI_MSG_WINDOW_INIT:
        {
          ref(window)->OnMessage(message);
          return true;
        }
        break;

      case GUI_MSG_CLICKED:
        {
          return Window::OnMessage(message);
        }
        break;
      }

      // we do not message CGUIPythonWindow here..
      return ref(window)->OnMessage(message);
    }

    bool WindowDialog::OnAction(const CAction &action)
    {
      XBMC_TRACE;
      return WindowDialogMixin::OnAction(action) ? true : Window::OnAction(action);
    }

    void WindowDialog::OnDeinitWindow(int nextWindowID)
    {
      g_windowManager.RemoveDialog(iWindowId);
      Window::OnDeinitWindow(nextWindowID);
    }

  }
}
