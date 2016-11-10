#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

// spdlog specific defines
#ifdef TARGET_WINDOWS
#define SPDLOG_WCHAR_FILENAMES
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#endif

#define SPDLOG_FMT_EXTERNAL
#define SPDLOG_FINAL final
#define SPDLOG_LEVEL_NAMES  { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "OFF" };
#define SPDLOG_ENABLE_PATTERN_PADDING

#include <spdlog/spdlog.h>

#include "commons/ilog.h"
#include "settings/AdvancedSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"

#if !defined(TARGET_WINDOWS)
#if defined(_UNICODE)
#define _T(x) L##x
#else  // defined(_UNICODE)
#define _T(x) x
#endif  // defined(_UNICODE)
#endif  // !defined(TARGET_WINDOWS)

#if defined(TARGET_ANDROID)
#include "platform/android/utils/AndroidInterfaceForCLog.h"
using PlatformInterfaceForCLog = CAndroidInterfaceForCLog;
#elif defined(TARGET_DARWIN)
#include "platform/darwin/DarwinInterfaceForCLog.h"
using PlatformInterfaceForCLog = CDarwinInterfaceForCLog;
#elif defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
#include "platform/win32/Win32InterfaceForCLog.h"
using PlatformInterfaceForCLog = CWin32InterfaceForCLog;
#else
#include "platform/posix/PosixInterfaceForCLog.h"
using PlatformInterfaceForCLog = CPosixInterfaceForCLog;
#endif

class CLog
{
public:
  using Logger = std::shared_ptr<spdlog::logger>;

  static void Initialize(const std::string& path);
  static void Uninitialize();

  static void SetLogLevel(int level);
  static int GetLogLevel() { return m_logLevel; }
  static void SetExtraLogLevels(int level) { m_extraLogLevels = level; }
  static bool IsLogLevelLogged(int loglevel);

  static inline Logger Get(const std::string& loggerName)
  {
    try
    {
      // try to create the logger with our file sink in case it doesn't exist
      auto logger = spdlog::create(loggerName, m_fileSink);
      logger->flush_on(spdlog::level::debug);

      return logger;
    }
    catch (const spdlog::spdlog_ex&)
    {
      // otherwise retrieve the existing logger
      return spdlog::get(loggerName);
    }
  }

  template <typename Char, typename... Args>
  static inline void Log(int level, const Char* format, Args&&... args)
  {
    Log(MapLogLevel(level), format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void Log(int level, int component, const Char* format, Args&&... args)
  {
    if (!g_advancedSettings.CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void Log(spdlog::level::level_enum level, const Char* format, Args&&... args)
  {
    if (m_defaultLogger == nullptr)
      return;

    FormatAndLogInternal(level, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void Log(spdlog::level::level_enum level, int component, const Char* format, Args&&... args)
  {
    if (!g_advancedSettings.CanLogComponent(component))
      return;

    Log(level, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void LogFunction(int level, const char* functionName, const Char* format, Args&&... args)
  {
    LogFunction(MapLogLevel(level), functionName, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void LogFunction(int level, const char* functionName, int component, const Char* format, Args&&... args)
  {
    if (!g_advancedSettings.CanLogComponent(component))
      return;

    LogFunction(level, functionName, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level, const char* functionName, const Char* format, Args&&... args)
  {
    if (m_defaultLogger == nullptr)
      return;

    if (functionName == nullptr || strlen(functionName) == 0)
      FormatAndLogInternal(level, format, std::forward<Args>(args)...);
    else
      FormatAndLogFunctionInternal(level, functionName, format, std::forward<Args>(args)...);
  }

  template <typename Char, typename... Args>
  static inline void LogFunction(spdlog::level::level_enum level, const char* functionName, int component, const Char* format, Args&&... args)
  {
    if (!g_advancedSettings.CanLogComponent(component))
      return;

    LogFunction(level, functionName, format, std::forward<Args>(args)...);
  }

#define LogF(level, format, ...) LogFunction((level), __FUNCTION__, (format), ##__VA_ARGS__)

private:
  static spdlog::level::level_enum MapLogLevel(int level)
  {
    switch (level)
    {
      case LOGDEBUG:
        return spdlog::level::debug;
      case LOGINFO:
      case LOGNOTICE:
        return spdlog::level::info;
      case LOGWARNING:
        return spdlog::level::warn;
      case LOGERROR:
        return spdlog::level::err;
      case LOGSEVERE:
      case LOGFATAL:
        return spdlog::level::critical;
      case LOGNONE:
        return spdlog::level::off;

      default:
        break;
    }

    return spdlog::level::info;
  }

  template <typename... Args>
  static inline void FormatAndLogInternal(spdlog::level::level_enum level, const char* format, Args&&... args)
  {
    // TODO: for now we manually format the messages to support both python- and printf-style formatting.
    //       this can be removed once all log messages have been adjusted to python-style formatting
    m_defaultLogger->log(level, StringUtils::Format(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static inline void FormatAndLogInternal(spdlog::level::level_enum level, const wchar_t* format, Args&&... args)
  {
    m_defaultLogger->log(level, StringUtils::Format(format, std::forward<Args>(args)...).c_str());
  }

  template <typename... Args>
  static inline void FormatAndLogFunctionInternal(spdlog::level::level_enum level, const char* functionName, const char* format, Args&&... args)
  {
    FormatAndLogInternal(level, StringUtils::Format("{0:s}: {1:s}", functionName, format).c_str(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  static inline void FormatAndLogFunctionInternal(spdlog::level::level_enum level, const char* functionName, const wchar_t* format, Args&&... args)
  {
    FormatAndLogInternal(level, StringUtils::Format(L"{0:s}: {1:s}", functionName, format).c_str(), std::forward<Args>(args)...);
  }

  static spdlog::sink_ptr m_fileSink;
  static Logger m_defaultLogger;

  static PlatformInterfaceForCLog m_platform;

  static int m_logLevel;
  static int m_extraLogLevels;
};

namespace XbmcUtils
{
  class LogImplementation : public XbmcCommons::ILogger
  {
  public:
    ~LogImplementation() override = default;
    inline void log(int logLevel, IN_STRING const char* message) override { CLog::Log(logLevel, "{0:s}", message); }
  };
}
