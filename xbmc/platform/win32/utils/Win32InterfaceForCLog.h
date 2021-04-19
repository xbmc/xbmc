/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)
#error This file is for win32 platforms only
#endif // !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)

#include "utils/IPlatformLog.h"

class CWin32InterfaceForCLog : public IPlatformLog
{
public:
  CWin32InterfaceForCLog() = default;
  ~CWin32InterfaceForCLog() override = default;

  spdlog_filename_t GetLogFilename(const std::string& filename) const override;
  void AddSinks(
      std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const override;
};
