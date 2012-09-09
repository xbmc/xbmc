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

#include "WindowDialogMixin.h"
#include "WindowInterceptor.h"

#include "ApplicationMessenger.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    void WindowDialogMixin::show()
    {
      TRACE;
      DelayedCallGuard dcguard(w->languageHook);

      ThreadMessage tMsg = {TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_OPENING, 0};
      tMsg.lpVoid = w->window->get();
      CApplicationMessenger::Get().SendMessage(tMsg, true);
    }

    void WindowDialogMixin::close()
    {
      TRACE;
      DelayedCallGuard dcguard(w->languageHook);
      w->bModal = false;
      w->PulseActionEvent();

      ThreadMessage tMsg = {TMSG_GUI_PYTHON_DIALOG, HACK_CUSTOM_ACTION_CLOSING, 0};
      tMsg.lpVoid = w->window->get();
      CApplicationMessenger::Get().SendMessage(tMsg, true);

    }
  }
}



