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
template<typename Mutex>
class basic_file_sink;

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
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  void Initialize(const std::string& path);
  void Uninitialize();

  void SetLogLevel(int level);
  int GetLogLevel() { return m_logLevel; }
  bool IsLogLevelLogged(int loglevel);

  bool CanLogComponent(uint32_t component) const;
  static void SettingOptionsLoggingComponentsFiller(std::shared_ptr<const CSetting> setting,
                                                    std::vector<IntegerSettingOption>& list,
                                                    int& current,
                                                    void* data);

  Logger GetLogger(const std::string& loggerName);

  template<typename Char, typename... Args>
  static inline void Log(int level, const Char* format, Args&&... args)
  {
    Log(MapLogLevel(level), format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void Log(int level, uint32_t component, const Char* format, Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void Log(spdlog::level::level_enum level, const Char* format, Args&&... args)
  {
    GetInstance().FormatAndLogInternal(level, format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void Log(spdlog::level::level_enum level,
                         uint32_t component,
                         const Char* format,
                         Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void LogFunction(int level,
                                 const char* functionName,
                                 const Char* format,
                                 Args&&... args)
  {
    LogFunction(MapLogLevel(level), functionName, format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void LogFunction(
      int level, const char* functionName, uint32_t component, const Char* format, Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    LogFunction(level, functionName, format, std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level,
                                 const char* functionName,
                                 const Char* format,
                                 Args&&... args)
  {
    if (functionName == nullptr || strlen(functionName) == 0)
      GetInstance().FormatAndLogInternal(level, format, std::forward<Args>(args)...);
    else
      GetInstance().FormatAndLogFunctionInternal(level, functionName, format,
                                                 std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level,
                                 const char* functionName,
                                 uint32_t component,
                                 const Char* format,
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
    GetInstance().FormatAndLogInternal(
        level, StringUtils::Format("{0:s}: {1:s}", functionName, format).c_str(),
        std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void FormatAndLogFunctionInternal(spdlog::level::level_enum level,
                                                  const char* functionName,
                                                  const wchar_t* format,
                                                  Args&&... args)
  {
    GetInstance().FormatAndLogInternal(
        level, StringUtils::Format(L"{0:s}: {1:s}", functionName, format).c_str(),
        std::forward<Args>(args)...);
  }

  template<typename Char, typename... Args>
  inline void FormatAndLogInternal(spdlog::level::level_enum level,
                                   const Char* format,
                                   Args&&... args)
  {
    // TODO: for now we manually format the messages to support both python- and printf-style formatting.
    //       this can be removed once all log messages have been adjusted to python-style formatting
    auto logString = StringUtils::Format(format, std::forward<Args>(args)...);

    // fixup newline alignment, number of spaces should equal prefix length
    StringUtils::Replace(logString, "\n", "\n                                                   ");

    m_defaultLogger->log(level, std::move(logString));
  }

  Logger CreateLogger(const std::string& loggerName);

  void SetComponentLogLevel(const std::vector<CVariant>& components);

  std::unique_ptr<IPlatformLog> m_platform;
  std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> m_sinks;
  Logger m_defaultLogger;

  std::shared_ptr<spdlog::sinks::basic_file_sink<std::mutex>> m_fileSink;

  int m_logLevel;

  bool m_componentLogEnabled;
  uint32_t m_componentLogLevels;
};

namespace XbmcUtils
{
class LogImplementation : public XbmcCommons::ILogger
{
public:
  ~LogImplementation() override = default;
  inline void log(int logLevel, IN_STRING const char* message) override
  {
    CLog::Log(logLevel, "{0:s}", message);
  }
};
} // namespace XbmcUtils
