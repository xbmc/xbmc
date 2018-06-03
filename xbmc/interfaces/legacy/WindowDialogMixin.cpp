 /*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "WindowDialogMixin.h"
#include "WindowInterceptor.h"

#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"

using namespace KODI::MESSAGING;

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void WindowDialogMixin::show()
    {
      XBMC_TRACE;
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_OPENING, 0, static_cast<void*>(w->window->get()));
    }

    void WindowDialogMixin::close()
    {
      XBMC_TRACE;
      w->bModal = false;
      w->PulseActionEvent();

      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_CLOSING, 0, static_cast<void*>(w->window->get()));

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
          CServiceBroker::GetGUI()->GetWindowManager().RegisterDialog(w->window->get());
          // active this dialog...
          CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
          w->OnMessage(msg);
          w->window->setActive(true);
          //! @todo Figure out how to clean up the CAction
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



