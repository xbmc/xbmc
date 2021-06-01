/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// spdlog specific defines
#define SPDLOG_LEVEL_NAMES {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "OFF"};

#include "commons/ilog.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/IPlatformLog.h"
#include "utils/StringUtils.h"
#include "utils/logtypes.h"

#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace spdlog
{
namespace sinks
{
class sink;

template<typename Mutex>
class dist_sink;
} // namespace sinks
} // namespace spdlog

class CLog : public ISettingsHandler, public ISettingCallback
{
public:
  CLog();
  ~CLog() = default;

  // implementation of ISettingsHandler
  void OnSettingsLoaded() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  void Initialize(const std::string& path);
  void Uninitialize();

  void SetLogLevel(int level);
  int GetLogLevel() { return m_logLevel; }
  bool IsLogLevelLogged(int loglevel);

  bool CanLogComponent(uint32_t component) const;
  static void SettingOptionsLoggingComponentsFiller(const std::shared_ptr<const CSetting>& setting,
                                                    std::vector<IntegerSettingOption>& list,
                                                    int& current,
                                                    void* data);

  Logger GetLogger(const std::string& loggerName);

  template<typename... Args>
  static inline void Log(int level, const char* format, Args&&... args)
  {
    Log(MapLogLevel(level), format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(int level, uint32_t component, const char* format, Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(spdlog::level::level_enum level, const char* format, Args&&... args)
  {
    GetInstance().FormatAndLogInternal(level, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(spdlog::level::level_enum level,
                         uint32_t component,
                         const char* format,
                         Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void LogFunction(int level,
                                 const char* functionName,
                                 const char* format,
                                 Args&&... args)
  {
    LogFunction(MapLogLevel(level), functionName, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void LogFunction(
      int level, const char* functionName, uint32_t component, const char* format, Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    LogFunction(level, functionName, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level,
                                 const char* functionName,
                                 const char* format,
                                 Args&&... args)
  {
    if (functionName == nullptr || strlen(functionName) == 0)
      GetInstance().FormatAndLogInternal(level, format, std::forward<Args>(args)...);
    else
      GetInstance().FormatAndLogFunctionInternal(level, functionName, format,
                                                 std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level,
                                 const char* functionName,
                                 uint32_t component,
                                 const char* format,
                                 Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    LogFunction(level, functionName, format, std::forward<Args>(args)...);
  }

#define LogF(level, format, ...) LogFunction((level), __FUNCTION__, (format), ##__VA_ARGS__)
#define LogFC(level, component, format, ...) \
  LogFunction((level), __FUNCTION__, (component), (format), ##__VA_ARGS__)

private:
  static CLog& GetInstance();

  static spdlog::level::level_enum MapLogLevel(int level);

  template<typename... Args>
  static inline void FormatAndLogFunctionInternal(spdlog::level::level_enum level,
                                                  const char* functionName,
                                                  const char* format,
                                                  Args&&... args)
  {
    GetInstance().FormatAndLogInternal(level,
                                       StringUtils::Format("{0:s}: {1:s}", functionName, format),
                                       std::forward<Args>(args)...);
  }

  template<typename... Args>
  inline void FormatAndLogInternal(spdlog::level::level_enum level,
                                   std::string format,
                                   Args&&... args)
  {
    // fixup newline alignment, number of spaces should equal prefix length
    StringUtils::Replace(format, "\n", "\n                                                   ");

    m_defaultLogger->log(level, format, std::forward<Args>(args)...);
  }

  Logger CreateLogger(const std::string& loggerName);

  void SetComponentLogLevel(const std::vector<CVariant>& components);

  std::unique_ptr<IPlatformLog> m_platform;
  std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> m_sinks;
  Logger m_defaultLogger;

  std::shared_ptr<spdlog::sinks::sink> m_fileSink;

  int m_logLevel;

  bool m_componentLogEnabled;
  uint32_t m_componentLogLevels;
};
