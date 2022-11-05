/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win10App.h"

#include "application/AppEnvironment.h"
#include "application/AppParamParser.h"
#include "application/AppParams.h"
#include "application/Application.h"
#include "pch.h"
#include "platform/Environment.h"
#include "platform/xbmc.h"
#include "rendering/dx/RenderContext.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/win10/WinEventsWin10.h"

#include "platform/win32/CharsetConverter.h"

#include <ppltasks.h>
#include <winrt/Windows.Storage.AccessCache.h>

using namespace KODI::PLATFORM::WINDOWS10;
namespace winrt
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Storage::AccessCache;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

App::App() = default;

// The first method called when the IFrameworkView is being created.
void App::Initialize(const CoreApplicationView& applicationView)
{
  // Register event handlers for app lifecycle. This example includes Activated, so that we
  // can make the CoreWindow active and start rendering on the window.
  applicationView.Activated({ this, &App::OnActivated });

  CoreApplication::Suspending({ this, &App::OnSuspending });
  CoreApplication::Resuming({ this, &App::OnResuming });
  // TODO
  // CoreApplication::UnhandledErrorDetected += ref new EventHandler<UnhandledErrorDetectedEventArgs^>(this, &App::OnUnhandledErrorDetected);

  //Initialize COM
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  // At this point we have access to the device.
  // We can create the device-dependent resources.
  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2, 2), &wd);

  // set up some xbmc specific relationships
  setlocale(LC_NUMERIC, "C");
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(const CoreWindow&)
{
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(const winrt::hstring&)
{
}

// This method is called after the window becomes active.
void App::Run()
{
  {
    // fix the case then window opened in FS, but current setting is RES_WINDOW
    // the proper way is make window params related to setting, but in this setting isn't loaded yet
    // perhaps we should observe setting changes and change window's Preferred props
    bool fullscreen = ApplicationView::GetForCurrentView().IsFullScreenMode();

    CAppParamParser appParamParser;
    appParamParser.Parse(m_argv.data(), m_argv.size());

    const auto params = appParamParser.GetAppParams();
    params->SetStartFullScreen(fullscreen);

    if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::Xbox)
      params->SetStandAlone(true);

    // Create and run the app
    CAppEnvironment::SetUp(params);
    XBMC_Run(true);
    CAppEnvironment::TearDown();
  }

  WSACleanup();
  //CoUninitialize();
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

void push_back(std::vector<char*> &vec, const std::string &str)
{
  if (!str.empty())
  {
    char *val = new char[str.length() + 1];
    std::strcpy(val, str.c_str());
    vec.push_back(val);
  }
}

// Application lifecycle event handlers.
void App::OnActivated(const CoreApplicationView& applicationView, const IActivatedEventArgs& args)
{
  m_argv.clear();
  push_back(m_argv, "dummy");

  if (args.Kind() == ActivationKind::Launch)
  {
    LaunchActivatedEventArgs launchArgs = args.as<LaunchActivatedEventArgs>();
    if (launchArgs.PrelaunchActivated())
    {
      // opt-out of Prelaunch
      CoreApplication::Exit();
      return;
    }
  }
  // Check for protocol activation
  else if (args.Kind() == ActivationKind::Protocol)
  {
    ProtocolActivatedEventArgs protocolArgs = args.as<ProtocolActivatedEventArgs>();
    winrt::hstring argval = protocolArgs.Uri().ToString();
    // Manipulate arguments …
  }
  // Check for file activation
  else if (args.Kind() == ActivationKind::File)
  {
    FileActivatedEventArgs fileArgs = args.as<FileActivatedEventArgs>();
    if (fileArgs && fileArgs.Files() && fileArgs.Files().Size() > 0)
    {
      using KODI::PLATFORM::WINDOWS::FromW;
      for (auto file : fileArgs.Files())
      {
        if (!StorageApplicationPermissions::FutureAccessList().CheckAccess(file))
        {
          // add file to FAL to get access to it later
          StorageApplicationPermissions::FutureAccessList().Add(file, file.Path());
        }
        std::string filePath = FromW(file.Path().c_str());
        push_back(m_argv, filePath);
      }
    }
  }
}

void App::OnSuspending(const winrt::IInspectable&, const SuspendingEventArgs& args)
{
  // Save app state asynchronously after requesting a deferral. Holding a deferral
  // indicates that the application is busy performing suspending operations. Be
  // aware that a deferral may not be held indefinitely. After about five seconds,
  // the app will be forced to exit.
  SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();

  Concurrency::create_task([this, deferral]()
  {
    auto windowing = DX::Windowing();
    if (windowing)
      windowing->TrimDevice();
    // Insert your code here.
    deferral.Complete();
  });
}

void App::OnResuming(const winrt::IInspectable&, const winrt::IInspectable&)
{
  // Restore any data or state that was unloaded on suspend. By default, data
  // and state are persisted when resuming from suspend. Note that this event
  // does not occur if the app was previously terminated.

  // Insert your code here.
}
