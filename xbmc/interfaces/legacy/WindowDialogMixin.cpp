/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowDialogMixin.h"

#include "ServiceBroker.h"
#include "WindowInterceptor.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void WindowDialogMixin::show()
    {
      XBMC_TRACE;
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_OPENING,
                                                 0, static_cast<void*>(w->window->get()));
    }

    void WindowDialogMixin::close()
    {
      XBMC_TRACE;
      w->bModal = false;
      w->PulseActionEvent();

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_CLOSING,
                                                 0, static_cast<void*>(w->window->get()));

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



