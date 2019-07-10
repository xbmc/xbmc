/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/dx/DeviceResources.h"
#include "utils/CharsetConverter.h" // Required to initialize converters before usage

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

