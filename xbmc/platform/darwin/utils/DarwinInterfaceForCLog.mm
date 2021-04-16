/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DarwinInterfaceForCLog.h"

#include <string>

#import <Foundation/Foundation.h>
#include <spdlog/common.h>
#include <spdlog/details/pattern_formatter.h>
#include <spdlog/sinks/dist_sink.h>

std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CDarwinInterfaceForCLog>();
}

CDarwinInterfaceForCLog::CDarwinInterfaceForCLog()
  : m_formatter(std::make_unique<spdlog::pattern_formatter>())
{
}

void CDarwinInterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  distributionSink->add_sink(std::make_shared<CDarwinInterfaceForCLog>());
}

void CDarwinInterfaceForCLog::log(const spdlog::details::log_msg& msg)
{
  spdlog::memory_buf_t formatted;
  m_formatter->format(msg, formatted);
  formatted.push_back('\0');
  NSLog(@"%s", formatted.data());
}

void CDarwinInterfaceForCLog::flush()
{
  fflush(stderr);
}

void CDarwinInterfaceForCLog::set_pattern(const std::string& pattern)
{
  set_formatter(std::make_unique<spdlog::pattern_formatter>(pattern));
}

void CDarwinInterfaceForCLog::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
{
  m_formatter = std::move(sink_formatter);
}
