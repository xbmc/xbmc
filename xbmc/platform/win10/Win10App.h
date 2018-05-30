/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#pragma once

#include "utils/CharsetConverter.h" // Required to initialize converters before usage
#include "rendering/dx/DeviceResources.h"

struct XBMC_keysym;

namespace KODI
{
  namespace PLATFORM
  {
    namespace WINDOWS10
    {
// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
struct App : winrt::implements<App, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource, winrt::Windows::ApplicationModel::Core::IFrameworkView>
{
  App();
  winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView()
  {
    return *this;
  }
  // IFrameworkView implementation.
  void Initialize(const winrt::Windows::ApplicationModel::Core::CoreApplicationView&);
  void SetWindow(const winrt::Windows::UI::Core::CoreWindow&);
  void Load(const winrt::hstring& entryPoint);
  void Run();
  void Uninitialize();

protected:
  // Application lifecycle event handlers.
  void OnActivated(const winrt::Windows::ApplicationModel::Core::CoreApplicationView&, const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs&);
  void OnSuspending(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::ApplicationModel::SuspendingEventArgs&);
  void OnResuming(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::Foundation::IInspectable&);

private:
  std::vector<char*> m_argv;
};
    } // namespace WINDOWS10
  } // namespace PLATFORM
} // namespace KODI

