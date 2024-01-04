/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/android/utils/AndroidInterfaceForCLog.h"

#include "CompileInfo.h"

#include <dlfcn.h>
#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/dist_sink.h>

// On some Android platforms debug logging is deactivated.
// We try to activate debug logging for our app via function "__android_log_set_minimum_priority" from "/system/lib/liblog.so".
// The function is defined in API level 30 (Android 11) as:
//   int32_t __android_log_set_minimum_priority(int32_t priority);
void ActivateAndroidDebugLogging()
{
  void* libHandle = dlopen("liblog.so", RTLD_LAZY);
  if (libHandle)
  {
    void* funcPtr = dlsym(libHandle, "__android_log_set_minimum_priority");
    if (funcPtr)
    {
      typedef int32_t (*android_log_set_minimum_priority_func)(int32_t);
      reinterpret_cast<android_log_set_minimum_priority_func>(funcPtr)(ANDROID_LOG_DEBUG);
    }
    dlclose(libHandle);
  }
}

std::unique_ptr<IPlatformLog> IPlatformLog::CreatePlatformLog()
{
  ActivateAndroidDebugLogging();
  return std::make_unique<CAndroidInterfaceForCLog>();
}

void CAndroidInterfaceForCLog::AddSinks(
    std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> distributionSink) const
{
  distributionSink->add_sink(
      std::make_shared<spdlog::sinks::android_sink_st>(CCompileInfo::GetAppName()));
}
