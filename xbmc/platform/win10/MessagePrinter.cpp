/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/MessagePrinter.h"

#include "CompileInfo.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "windowing/win10/WinSystemWin10DX.h"

#include "platform/win32/CharsetConverter.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Popups.h>

int WINAPI MessageBox(void* hWnd, const char* lpText, const char* lpCaption, UINT uType)
{
  using namespace winrt::Windows::ApplicationModel::Core;

  auto coreWindow = CoreApplication::MainView().CoreWindow();
  if (!coreWindow)
    return IDOK;

  auto wText = KODI::PLATFORM::WINDOWS::ToW(lpText);
  auto wCaption = KODI::PLATFORM::WINDOWS::ToW(lpCaption);

  auto handler = winrt::Windows::UI::Core::DispatchedHandler([wText, wCaption]()
  {
    // Show the message dialog
    auto msg = winrt::Windows::UI::Popups::MessageDialog(wText, wCaption);
    // Set the command to be invoked when a user presses 'ESC'
    msg.CancelCommandIndex(1);
    msg.ShowAsync();
  });

  if (coreWindow.Dispatcher().HasThreadAccess())
    handler();
  else
    coreWindow.Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, handler);

  return IDOK;
}

void CMessagePrinter::DisplayMessage(const std::string& message)
{
  MessageBox(NULL, message.c_str(), CCompileInfo::GetAppName(), MB_OK | MB_ICONINFORMATION);
}

void CMessagePrinter::DisplayWarning(const std::string& warning)
{
  MessageBox(NULL, warning.c_str(), CCompileInfo::GetAppName(), MB_OK | MB_ICONWARNING);
}

void CMessagePrinter::DisplayError(const std::string& error)
{
  MessageBox(NULL, error.c_str(), CCompileInfo::GetAppName(), MB_OK | MB_ICONERROR);
}

void CMessagePrinter::DisplayHelpMessage(const std::vector<std::pair<std::string, std::string>>& help)
{
  //very crude implementation, pretty it up when possible
  std::string message;
  for (const auto& line : help)
  {
    message.append(line.first + "\t" + line.second + "\r\n");
  }

  MessageBox(NULL, message.c_str(), CCompileInfo::GetAppName(), MB_OK | MB_ICONINFORMATION);
}