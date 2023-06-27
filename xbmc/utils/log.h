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
  CLog();
  ~CLog();

  // implementation of ISettingsHandler
  void OnSettingsLoaded() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  void Initialize(const std::string& path);
  void UnregisterFromSettings();
  void Deinitialize();

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
  static inline void Log(int level, const std::string_view& format, Args&&... args)
  {
    Log(MapLogLevel(level), format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(int level,
                         uint32_t component,
                         const std::string_view& format,
                         Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(spdlog::level::level_enum level,
                         const std::string_view& format,
                         Args&&... args)
  {
    GetInstance().FormatAndLogInternal(level, format, std::forward<Args>(args)...);
  }

  template<typename... Args>
  static inline void Log(spdlog::level::level_enum level,
                         uint32_t component,
                         const std::string_view& format,
                         Args&&... args)
  {
    if (!GetInstance().CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

#define LogF(level, format, ...) Log((level), ("{}: " format), __FUNCTION__, ##__VA_ARGS__)
#define LogFC(level, component, format, ...) \
  Log((level), (component), ("{}: " format), __FUNCTION__, ##__VA_ARGS__)

private:
  static CLog& GetInstance();

  static spdlog::level::level_enum MapLogLevel(int level);

  template<typename... Args>
  inline void FormatAndLogInternal(spdlog::level::level_enum level,
                                   const std::string_view& format,
                                   Args&&... args)
  {
    auto message = fmt::format(format, std::forward<Args>(args)...);

    // fixup newline alignment, number of spaces should equal prefix length
    FormatLineBreaks(message);

    m_defaultLogger->log(level, message);
  }

  Logger CreateLogger(const std::string& loggerName);

  void SetComponentLogLevel(const std::vector<CVariant>& components);

  void FormatLineBreaks(std::string& message);

  std::unique_ptr<IPlatformLog> m_platform;
  std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> m_sinks;
  Logger m_defaultLogger;

  std::shared_ptr<spdlog::sinks::sink> m_fileSink;

  int m_logLevel;

  bool m_componentLogEnabled = false;
  uint32_t m_componentLogLevels = 0;
};
