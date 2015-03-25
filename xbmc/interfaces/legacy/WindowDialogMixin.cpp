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

#include "WindowDialogMixin.h"
#include "WindowInterceptor.h"

#include "ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void WindowDialogMixin::show()
    {
      XBMC_TRACE;
      ThreadMessage tMsg = {TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_OPENING, 0};
      tMsg.lpVoid = w->window->get();
      CApplicationMessenger::Get().SendMessage(tMsg, true);
    }

    void WindowDialogMixin::close()
    {
      XBMC_TRACE;
      w->bModal = false;
      w->PulseActionEvent();

      ThreadMessage tMsg = {TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_CLOSING, 0};
      tMsg.lpVoid = w->window->get();
      CApplicationMessenger::Get().SendMessage(tMsg, true);

      w->iOldWindowId = 0;
    }

    bool WindowDialogMixin::IsDialogRunning() const { XBMC_TRACE; return w->window->isActive(); }

    bool WindowDialogMixin::OnAction(const CAction &action)
    {
      XBMC_TRACE;
      switch (action.GetID())
      {
      case HACK_CUSTOM_ACTION_OPENING:
        {
          // This is from the CGUIPythonWindowXMLDialog::Show_Internal
          g_windowManager.RegisterDialog(w->window->get());
          // active this dialog...
          CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
          w->OnMessage(msg);
          w->window->setActive(true);
          // TODO: Figure out how to clean up the CAction
          return true;
        }
        break;
        
      case HACK_CUSTOM_ACTION_CLOSING:
        {
          // This is from the CGUIPythonWindowXMLDialog::Show_Internal
          w->window->get()->Close();
          return true;
        }
        break;
      }

      return false;
    }
  }
}



