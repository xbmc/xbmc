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

#include "log.h"

#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/file_sinks.h>

#include "CompileInfo.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"

static const std::string LogFileExtension = ".log";

spdlog::sink_ptr CLog::m_fileSink = nullptr;
CLog::Logger CLog::m_defaultLogger = nullptr;
PlatformInterfaceForCLog CLog::m_platform;

int CLog::m_logLevel = LOG_LEVEL_NORMAL;
int CLog::m_extraLogLevels = 0;

void CLog::Initialize(const std::string& path)
{
  if (m_fileSink != nullptr)
    return;

  // create the default sink(s)
  auto distSink = std::make_shared<spdlog::sinks::dist_sink_mt>();

  if (!path.empty())
  {
    // put together the path to the log file(s)
    std::string appName = CCompileInfo::GetAppName();
    StringUtils::ToLower(appName);
    const std::string filePathBase = URIUtils::AddFileToFolder(path, appName);
    const std::string filePath = filePathBase + LogFileExtension;
    const std::string oldFilePath = filePathBase + ".old" + LogFileExtension;

    // handle old.log by deleting an existing old.log and renaming the last log to old.log
    XFILE::CFile::Delete(oldFilePath);
    XFILE::CFile::Rename(filePath, oldFilePath);

    // add the file sink logger
    distSink->add_sink(std::make_shared<spdlog::sinks::simple_file_sink_mt>(m_platform.GetLogFilename(filePath), true));
  }

  // add platform-specific debug sinks
  m_platform.AddSinks(distSink);
  m_fileSink = distSink;

  // create the default logger
  m_defaultLogger = spdlog::create("root", m_fileSink);
  m_defaultLogger->flush_on(spdlog::level::debug);

  // set the default formatting
  spdlog::set_pattern("%T.%f T:%t %l %n: %v");
  // set special pattern for the default logger (which doesn't have a proper logger name)
  m_defaultLogger->set_pattern("%T.%f T:%t %l: %v");
}

void CLog::Uninitialize()
{
  // flush all loggers
  spdlog::apply_all(
    [](std::shared_ptr<spdlog::logger> logger)
    {
      logger->flush();
    });

  // flush the file sink
  if (m_fileSink != nullptr)
  {
    m_fileSink->flush();
    m_fileSink.reset();
  }

  // drop all other loggers
  spdlog::drop_all();
}

void CLog::SetLogLevel(int level)
{
  if (level < LOG_LEVEL_NONE || level > LOG_LEVEL_MAX)
    return;

  m_logLevel = level;

  auto spdLevel = spdlog::level::info;
#if defined(_DEBUG) || defined(PROFILE)
  spdLevel = spdlog::level::trace;
#else
  if (level <= LOG_LEVEL_NONE)
    spdLevel = spdlog::level::off;
  else if (level >= LOG_LEVEL_DEBUG)
    spdLevel = spdlog::level::trace;
  else
    spdLevel = spdlog::level::info;
#endif

  if (m_defaultLogger != nullptr && m_defaultLogger->level() == spdLevel)
    return;

  spdlog::set_level(spdLevel);
  Log(spdlog::level::info, "Log level changed to \"%s\"", spdlog::level::to_str(spdLevel));
}

bool CLog::IsLogLevelLogged(int loglevel)
{
  const int extras = (loglevel & ~LOGMASK);
  if (extras != 0 && (m_extraLogLevels & extras) == 0)
    return false;

  if (m_logLevel >= LOG_LEVEL_DEBUG)
    return true;
  if (m_logLevel <= LOG_LEVEL_NONE)
    return false;

  return (loglevel & LOGMASK) >= LOGNOTICE;
}
