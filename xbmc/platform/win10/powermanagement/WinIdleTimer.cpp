/*
 *      Copyright (C) 2005-2018 Team Kodi
 *      http://kodi.tv
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
#include "powermanagement/WinIdleTimer.h"
#include "Application.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::System::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;

void CWinIdleTimer::StartZero()
{
  if (!g_application.IsDPMSActive())
  {
    auto workItem = ref new DispatchedHandler([]()
    {
      try
      {
        auto displayRequest = ref new DisplayRequest();
        // this couple of calls activate and deactivate a display-required
        // request in other words they reset display idle timer
        displayRequest->RequestActive();
        displayRequest->RequestRelease();
      }
      catch (Platform::Exception^ ex) { }
    });
    CoreWindow^ window = CoreApplication::MainView->CoreWindow;
    window->Dispatcher->RunAsync(CoreDispatcherPriority::High, workItem);
  }
  CStopWatch::StartZero();
}
