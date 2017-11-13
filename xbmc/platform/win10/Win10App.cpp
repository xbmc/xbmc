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

#include "pch.h"

#include "Application.h"
#include "AppParamParser.h"
#include "platform/xbmc.h"
#include "platform/XbmcContext.h"
#include "platform/win32/CharsetConverter.h"
#include "settings/AdvancedSettings.h"
#include "utils/Environment.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "windowing/win10/WinEventsWin10.h"
#include "Win10App.h"

#include <collection.h>
#include <ppltasks.h>

using namespace KODI::PLATFORM::WINDOWS10;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Storage::AccessCache;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

IFrameworkView^ ViewProvider::CreateView()
{
  return ref new App();
}

App::App()
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
  // Register event handlers for app lifecycle. This example includes Activated, so that we
  // can make the CoreWindow active and start rendering on the window.
  applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
  
  CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);
  CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &App::OnResuming);
  // TODO 
  // CoreApplication::UnhandledErrorDetected += ref new EventHandler<UnhandledErrorDetectedEventArgs^>(this, &App::OnUnhandledErrorDetected);

  //Initialize COM
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  // At this point we have access to the device. 
  // We can create the device-dependent resources.
  CWinEventsWin10::InitOSKeymap();

  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2, 2), &wd);

  // set up some xbmc specific relationships
  setlocale(LC_NUMERIC, "C");
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
  g_Windowing.SetCoreWindow(window);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
}

// This method is called after the window becomes active.
void App::Run()
{
  {
    XBMC::Context context;
    // Initialize before CAppParamParser so it can set the log level
    g_advancedSettings.Initialize();
    // fix the case then window opened in FS, but current setting is RES_WINDOW
    // the proper way is make window params related to setting, but in this setting isn't loaded yet
    // perhaps we should observe setting changes and change window's Preffered props 
    bool fullscreen = ApplicationView::GetForCurrentView()->IsFullScreenMode;
    g_advancedSettings.m_startFullScreen = fullscreen;

    CAppParamParser appParamParser;
    appParamParser.Parse(m_argv.data(), m_argv.size());
    // Create and run the app
    XBMC_Run(true, appParamParser);
  }

  WSACleanup();
  CoUninitialize();
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

void push_back(std::vector<char*> &vec, std::string &str)
{
  if (!str.empty())
  {
    char *val = new char[str.length() + 1];
    std::strcpy(val, str.c_str());
    vec.push_back(val);
  }
}

// Application lifecycle event handlers.
void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
  m_argv.clear();
  push_back(m_argv, std::string("dummy"));

  // Check for protocol activation
  if (args->Kind == ActivationKind::Protocol)
  {
    auto protocolArgs = static_cast< ProtocolActivatedEventArgs^>(args);
    Platform::String^ argval = protocolArgs->Uri->ToString();
    // Manipulate arguments …
  }
  // Check for file activation
  else if (args->Kind == ActivationKind::File)
  {
    auto fileArgs = static_cast<FileActivatedEventArgs^>(args);
    if (fileArgs && fileArgs->Files && fileArgs->Files->Size > 0)
    {
      using KODI::PLATFORM::WINDOWS::FromW;
      for (auto file : fileArgs->Files)
      {
        if (!StorageApplicationPermissions::FutureAccessList->CheckAccess(file))
        {
          // add file to FAL to get access to it later
          StorageApplicationPermissions::FutureAccessList->Add(file, file->Path);
        }
        std::string filePath = FromW(file->Path->Data(), file->Path->Length());
        push_back(m_argv, filePath);
      }
    }
  }
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
  // Save app state asynchronously after requesting a deferral. Holding a deferral
  // indicates that the application is busy performing suspending operations. Be
  // aware that a deferral may not be held indefinitely. After about five seconds,
  // the app will be forced to exit.
  SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

  Concurrency::create_task([this, deferral]()
  {
    g_Windowing.TrimDevice();
    // Insert your code here.
    deferral->Complete();
  });
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
  // Restore any data or state that was unloaded on suspend. By default, data
  // and state are persisted when resuming from suspend. Note that this event
  // does not occur if the app was previously terminated.

  // Insert your code here.
}
