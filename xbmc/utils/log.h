/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// spdlog specific defines
// clang-format off
#include <string_view>
#define SPDLOG_LEVEL_NAMES \
{ \
  std::string_view{"TRACE"}, \
  std::string_view{"DEBUG"}, \
  std::string_view{"INFO"}, \
  std::string_view{"WARNING"}, \
  std::string_view{"ERROR"}, \
  std::string_view{"FATAL"}, \
  std::string_view{"OFF"} \
};
// clang-format on

#include "commons/ilog.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/IPlatformLog.h"
#include "utils/logtypes.h"

#include <source_location>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace spdlog::sinks
{
class sink;

template<typename Mutex>
class dist_sink;
} // namespace spdlog::sinks

#if FMT_VERSION >= 100000
using fmt::enums::format_as;

namespace fmt
{
template<typename T, typename Char>
struct formatter<std::atomic<T>, Char> : formatter<T, Char>
{
};
} // namespace fmt
#endif

class CLog : public ISettingsHandler, public ISettingCallback
{
public:
  // id of the "general" log component
  static constexpr uint32_t LOG_COMPONENT_GENERAL = 0;

  CLog();
  ~CLog() override;

  // implementation of ISettingsHandler
  void OnSettingsLoaded() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  void Initialize(const std::string& path);
  void UnregisterFromSettings();
  void Deinitialize();

  void SetLogLevel(int level);
  int GetLogLevel() const { return m_logLevel; }
  bool IsLogLevelLogged(int loglevel) const;

  bool CanLogComponent(uint32_t component) const;
  static void SettingOptionsLoggingComponentsFiller(const std::shared_ptr<const CSetting>& setting,
                                                    std::vector<IntegerSettingOption>& list,
                                                    int& current);

  Logger GetLogger(const std::string& loggerName);

  template<typename... Args>
  static void Log(int level, fmt::format_string<Args...> format, Args&&... args)
  {
    Log(MapLogLevel(level), format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Log(int level, uint32_t component, fmt::format_string<Args...> format, Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(MapLogLevel(level), component, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Log(spdlog::level::level_enum level,
                  fmt::format_string<Args...> format,
                  Args&&... args)
  {
    Log(level, LOG_COMPONENT_GENERAL, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static void Log(spdlog::level::level_enum level,
                  uint32_t component,
                  fmt::format_string<Args...> format,
                  Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    GetInstance().FormatAndLogInternal(level, component, format, fmt::make_format_args(args...));
  }

  template<typename... Args>
  static void Log(const std::string& loggerName,
                  int level,
                  fmt::format_string<Args...> format,
                  Args&&... args)
  {
    GetInstance().FormatAndLogInternal(loggerName, MapLogLevel(level), format,
                                       fmt::make_format_args(args...));
  }

#ifdef TARGET_WINDOWS
#define LogF(level, format, ...) Log((level), ("{}: " format), __FUNCTION__, ##__VA_ARGS__)
#define LogFC(level, component, format, ...) \
  Log((level), (component), ("{}: " format), __FUNCTION__, ##__VA_ARGS__)
#else
#define LogF(level, format, ...) \
  Log((level), ("{}: " format), std::source_location::current().function_name(), ##__VA_ARGS__)
#define LogFC(level, component, format, ...) \
  Log((level), (component), ("{}: " format), std::source_location::current().function_name(), \
      ##__VA_ARGS__)
#endif

private:
  static CLog& GetInstance();

  static spdlog::level::level_enum MapLogLevel(int level);

  void FormatAndLogInternal(spdlog::level::level_enum level,
                            uint32_t component,
                            fmt::string_view format,
                            fmt::format_args args);

  void FormatAndLogInternal(const std::string& loggerName,
                            spdlog::level::level_enum level,
                            fmt::string_view format,
                            fmt::format_args args);

  Logger CreateLogger(const std::string& loggerName);

  Logger GetLoggerById(uint32_t component);

  void SetComponentLogLevel(const std::vector<CVariant>& components);

  void FormatLineBreaks(std::string& message) const;

  std::unique_ptr<IPlatformLog> m_platform;
  std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> m_sinks;
  Logger m_defaultLogger;

  std::shared_ptr<spdlog::sinks::sink> m_fileSink;

  int m_logLevel{LOG_LEVEL_DEBUG};

  bool m_componentLogEnabled{false};
  uint32_t m_componentLogLevels{0};
};
