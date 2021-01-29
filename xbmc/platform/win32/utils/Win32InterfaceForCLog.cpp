/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)
#error This file is for win32 platforms only
#endif // !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)

#include "Win32InterfaceForCLog.h"

#include "platform/win32/WIN32Util.h"

#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/msvc_sink.h>

std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CWin32InterfaceForCLog>();
}

spdlog_filename_t CWin32InterfaceForCLog::GetLogFilename(const std::string& filename) const
{
  return CWIN32Util::ConvertPathToWin32Form(CWIN32Util::SmbToUnc(filename));
}

void CWin32InterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  distributionSink->add_sink(std::make_shared<spdlog::sinks::msvc_sink_st>());
}
