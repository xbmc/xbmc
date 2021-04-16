/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/android/utils/AndroidInterfaceForCLog.h"

#include "CompileInfo.h"

#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/dist_sink.h>

std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  return std::make_unique<CAndroidInterfaceForCLog>();
}

void CAndroidInterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  distributionSink->add_sink(
      std::make_shared<spdlog::sinks::android_sink_st>(CCompileInfo::GetAppName()));
}
