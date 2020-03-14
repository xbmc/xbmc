/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>

#ifdef TARGET_WINDOWS
using spdlog_filename_t = std::wstring;
#else
using spdlog_filename_t = std::string;
#endif

namespace spdlog
{
namespace sinks
{
template<typename Mutex>
class dist_sink;
}
} // namespace spdlog

class IPlatformLog
{
public:
  virtual ~IPlatformLog() = default;

  static std::unique_ptr<IPlatformLog> CreatePlatformLog();

  virtual spdlog_filename_t GetLogFilename(const std::string& filename) const = 0;
  virtual void AddSinks(
      std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const = 0;
};
