/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/wasm/utils/WasmInterfaceForCLog.h"

#include "ServiceBroker.h"
#include "application/AppParams.h"

#include <mutex>

#include <emscripten.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/dist_sink.h>

namespace
{
// Routes log lines to the browser console, mapping spdlog levels onto console methods so
// DevTools filtering works.
class CBrowserConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    spdlog::memory_buf_t formatted;
    formatter_->format(msg, formatted);

    // The formatter appends a newline, which the console would render as an empty line.
    size_t length = formatted.size();
    while (length > 0 && (formatted[length - 1] == '\n' || formatted[length - 1] == '\r'))
      --length;

    int method = 3;
    switch (msg.level)
    {
      case spdlog::level::trace:
      case spdlog::level::debug:
        method = 0;
        break;
      case spdlog::level::info:
        method = 1;
        break;
      case spdlog::level::warn:
        method = 2;
        break;
      default:
        break;
    }

    // clang-format off
    EM_ASM({
      const text = UTF8ToString($0, $1);
      switch ($2)
      {
        case 0: console.debug(text); break;
        case 1: console.info(text); break;
        case 2: console.warn(text); break;
        default: console.error(text); break;
      }
    }, formatted.data(), length, method);
    // clang-format on
  }

  void flush_() override {}
};
} // namespace

std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CWasmInterfaceForCLog>();
}

void CWasmInterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  if (CServiceBroker::GetAppParams()->GetLogTarget() == "console")
    distributionSink->add_sink(std::make_shared<CBrowserConsoleSink>());
}
