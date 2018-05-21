/*
 *      Copyright (C) 2005-2018 Team Kodi
 *      http://kodi.tv
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
#include "powermanagement/WinIdleTimer.h"
#include "Application.h"
#include <winrt/Windows.System.Display.h>

using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::System::Display;
using namespace winrt::Windows::UI::Core;

void CWinIdleTimer::StartZero()
{
  static DisplayRequest displayRequest = nullptr;
  if (!g_application.IsDPMSActive())
  {
    if (!displayRequest)
      displayRequest = DisplayRequest();

    auto workItem = DispatchedHandler([&]()
    {
      try
      {
        // this couple of calls activate and deactivate a display-required
        // request in other words they reset display idle timer
        displayRequest.RequestActive();
        displayRequest.RequestRelease();
      }
      catch (const winrt::hresult_error&) { }
    });
    CoreWindow window = CoreApplication::MainView().CoreWindow();
    window.Dispatcher().RunAsync(CoreDispatcherPriority::High, workItem);
  }
  CStopWatch::StartZero();
}
