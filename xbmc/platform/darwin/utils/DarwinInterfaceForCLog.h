/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/utils/PosixInterfaceForCLog.h"

#include <memory>
#include <mutex>

#include <spdlog/formatter.h>
#include <spdlog/sinks/sink.h>

class CDarwinInterfaceForCLog : public CPosixInterfaceForCLog, public spdlog::sinks::sink
{
public:
  CDarwinInterfaceForCLog();
  ~CDarwinInterfaceForCLog() = default;

  // specialization of CPosixInterfaceForCLog
  void AddSinks(
      std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const override;

  // implementations of spdlog::sink
  void log(const spdlog::details::log_msg& msg) override;
  void flush() override;
  void set_pattern(const std::string& pattern) override;
  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;

private:
  std::unique_ptr<spdlog::formatter> m_formatter;
  std::mutex m_mutex;
};
