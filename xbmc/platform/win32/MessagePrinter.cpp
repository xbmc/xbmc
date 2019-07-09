/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/MessagePrinter.h"

#include "CompileInfo.h"

#include "platform/win32/CharsetConverter.h"

#include <windows.h>

void CMessagePrinter::DisplayMessage(const std::string& message)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  MessageBox(nullptr, ToW(message).c_str(), ToW(CCompileInfo::GetAppName()).c_str(), MB_OK | MB_ICONINFORMATION);
}

void CMessagePrinter::DisplayWarning(const std::string& warning)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  MessageBox(nullptr, ToW(warning).c_str(), ToW(CCompileInfo::GetAppName()).c_str(), MB_OK | MB_ICONWARNING);
}

void CMessagePrinter::DisplayError(const std::string& error)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  MessageBox(nullptr, ToW(error).c_str(), ToW(CCompileInfo::GetAppName()).c_str(), MB_OK | MB_ICONERROR);
}

void CMessagePrinter::DisplayHelpMessage(const std::vector<std::pair<std::string, std::string>>& help)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  //very crude implementation, pretty it up when possible
  std::string message;
  for (const auto& line : help)
  {
    message.append(line.first + "\t" + line.second + "\r\n");
  }

  MessageBox(nullptr, ToW(message).c_str(), ToW(CCompileInfo::GetAppName()).c_str(), MB_OK | MB_ICONINFORMATION);
}
