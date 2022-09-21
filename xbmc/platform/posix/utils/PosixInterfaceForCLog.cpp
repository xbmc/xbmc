/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PosixInterfaceForCLog.h"

#include "ServiceBroker.h"
#include "application/AppParams.h"

#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if !defined(TARGET_ANDROID) && !defined(TARGET_DARWIN)
std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CPosixInterfaceForCLog>();
}
#endif

void CPosixInterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  if (CServiceBroker::GetAppParams()->GetLogTarget() == "console")
    distributionSink->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
}
