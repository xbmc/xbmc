/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "log.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <cstring>
#include <set>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/dup_filter_sink.h>

namespace
{
static constexpr unsigned char Utf8Bom[3] = {0xEF, 0xBB, 0xBF};
static const std::string LogFileExtension = ".log";
static const std::string LogPattern = "%Y-%m-%d %T.%e T:%-5t %7l <%n>: %v";
} // namespace

CLog::CLog()
  : m_platform(IPlatformLog::CreatePlatformLog()),
    m_sinks(std::make_shared<spdlog::sinks::dist_sink_mt>()),
    m_defaultLogger(CreateLogger("general")),
    m_logLevel(LOG_LEVEL_DEBUG)
{
  // add platform-specific debug sinks
  m_platform->AddSinks(m_sinks);

  // register the default logger with spdlog
  spdlog::set_default_logger(m_defaultLogger);

  // set the formatting pattern globally
  spdlog::set_pattern(LogPattern);

  // flush on debug logs
  spdlog::flush_on(spdlog::level::debug);

  // set the log level
  SetLogLevel(m_logLevel);
}

CLog::~CLog()
{
  spdlog::drop("general");
}

void CLog::OnSettingsLoaded()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_componentLogEnabled = settings->GetBool(CSettings::SETTING_DEBUG_EXTRALOGGING);
  SetComponentLogLevel(settings->GetList(CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL));
}

void CLog::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_DEBUG_EXTRALOGGING)
    m_componentLogEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL)
    SetComponentLogLevel(
        CSettingUtils::GetList(std::static_pointer_cast<const CSettingList>(setting)));
}

void CLog::Initialize(const std::string& path) 
{
  if (m_fileSink != nullptr)
    return;

  // register setting callbacks
  auto settingsManager =
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager();
  settingsManager->RegisterSettingOptionsFiller("loggingcomponents",
                                                SettingOptionsLoggingComponentsFiller);
  settingsManager->RegisterSettingsHandler(this);
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_DEBUG_EXTRALOGGING);
  settingSet.insert(CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL);
  settingsManager->RegisterCallback(this, settingSet);

  if (path.empty())
    return;

  // put together the path to the log file(s)
  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  const std::string filePathBase = URIUtils::AddFileToFolder(path, appName);
  const std::string filePath = filePathBase + LogFileExtension;
  const std::string oldFilePath = filePathBase + ".old" + LogFileExtension;

  // handle old.log by deleting an existing old.log and renaming the last log to old.log
  XFILE::CFile::Delete(oldFilePath);
  XFILE::CFile::Rename(filePath, oldFilePath);

  // write UTF-8 BOM
  {
    XFILE::CFile file;
    if (file.OpenForWrite(filePath, true))
      file.Write(Utf8Bom, sizeof(Utf8Bom));
  }

  // create the file sink within a duplicate filter sink
  auto duplicateFilterSink =
      std::make_shared<spdlog::sinks::dup_filter_sink_st>(std::chrono::seconds(10));
  auto basicFileSink = std::make_shared<spdlog::sinks::basic_file_sink_st>(
      m_platform->GetLogFilename(filePath), false);
  basicFileSink->set_pattern(LogPattern);
  duplicateFilterSink->add_sink(basicFileSink);
  m_fileSink = duplicateFilterSink;

  // add it to the existing sinks
  m_sinks->add_sink(m_fileSink);
}

void CLog::UnregisterFromSettings()
{
  // unregister setting callbacks
  auto settingsManager =
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager();
  settingsManager->UnregisterSettingOptionsFiller("loggingcomponents");
  settingsManager->UnregisterSettingsHandler(this);
  settingsManager->UnregisterCallback(this);
}

void CLog::Deinitialize()
{
  if (m_fileSink == nullptr)
    return;

  // flush all loggers
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) { logger->flush(); });

  // flush the file sink
  m_fileSink->flush();

  // remove and destroy the file sink
  m_sinks->remove_sink(m_fileSink);
  m_fileSink.reset();
}

void CLog::SetLogLevel(int level)
{
  if (level < LOG_LEVEL_NONE || level > LOG_LEVEL_MAX)
    return;

  m_logLevel = level;

  auto spdLevel = spdlog::level::info;
  if (level <= LOG_LEVEL_NONE)
    spdLevel = spdlog::level::off;
  else if (level >= LOG_LEVEL_DEBUG)
    spdLevel = spdlog::level::trace;

  if (m_defaultLogger != nullptr && m_defaultLogger->level() == spdLevel)
    return;

  spdlog::set_level(spdLevel);
  FormatAndLogInternal(spdlog::level::info, "Log level changed to \"{}\"",
                       spdlog::level::to_string_view(spdLevel));
}

bool CLog::IsLogLevelLogged(int loglevel)
{
  if (m_logLevel >= LOG_LEVEL_DEBUG)
    return true;
  if (m_logLevel <= LOG_LEVEL_NONE)
    return false;

  return (loglevel & LOGMASK) >= LOGINFO;
}

bool CLog::CanLogComponent(uint32_t component) const
{
  if (!m_componentLogEnabled || component == 0)
    return false;

  return ((m_componentLogLevels & component) == component);
}

void CLog::SettingOptionsLoggingComponentsFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data)
{
  list.emplace_back(g_localizeStrings.Get(669), LOGSAMBA);
  list.emplace_back(g_localizeStrings.Get(670), LOGCURL);
  list.emplace_back(g_localizeStrings.Get(672), LOGFFMPEG);
  list.emplace_back(g_localizeStrings.Get(675), LOGJSONRPC);
  list.emplace_back(g_localizeStrings.Get(676), LOGAUDIO);
  list.emplace_back(g_localizeStrings.Get(680), LOGVIDEO);
  list.emplace_back(g_localizeStrings.Get(683), LOGAVTIMING);
  list.emplace_back(g_localizeStrings.Get(684), LOGWINDOWING);
  list.emplace_back(g_localizeStrings.Get(685), LOGPVR);
  list.emplace_back(g_localizeStrings.Get(686), LOGEPG);
  list.emplace_back(g_localizeStrings.Get(39117), LOGANNOUNCE);
  list.emplace_back(g_localizeStrings.Get(39124), LOGADDONS);
#ifdef HAS_DBUS
  list.emplace_back(g_localizeStrings.Get(674), LOGDBUS);
#endif
#ifdef HAS_WEB_SERVER
  list.emplace_back(g_localizeStrings.Get(681), LOGWEBSERVER);
#endif
#ifdef HAS_AIRTUNES
  list.emplace_back(g_localizeStrings.Get(677), LOGAIRTUNES);
#endif
#ifdef HAS_UPNP
  list.emplace_back(g_localizeStrings.Get(678), LOGUPNP);
#endif
#ifdef HAVE_LIBCEC
  list.emplace_back(g_localizeStrings.Get(679), LOGCEC);
#endif
  list.emplace_back(g_localizeStrings.Get(682), LOGDATABASE);
#if defined(HAS_FILESYSTEM_SMB)
  list.emplace_back(g_localizeStrings.Get(37050), LOGWSDISCOVERY);
#endif
}

Logger CLog::GetLogger(const std::string& loggerName)
{
  auto logger = spdlog::get(loggerName);
  if (logger == nullptr)
    logger = CreateLogger(loggerName);

  return logger;
}

CLog& CLog::GetInstance()
{
  return CServiceBroker::GetLogging();
}

spdlog::level::level_enum CLog::MapLogLevel(int level)
{
  switch (level)
  {
    case LOGDEBUG:
      return spdlog::level::debug;
    case LOGINFO:
      return spdlog::level::info;
    case LOGWARNING:
      return spdlog::level::warn;
    case LOGERROR:
      return spdlog::level::err;
    case LOGFATAL:
      return spdlog::level::critical;
    case LOGNONE:
      return spdlog::level::off;

    default:
      break;
  }

  return spdlog::level::info;
}

Logger CLog::CreateLogger(const std::string& loggerName)
{
  // create the logger
  auto logger = std::make_shared<spdlog::logger>(loggerName, m_sinks);

  // initialize the logger
  spdlog::initialize_logger(logger);

  return logger;
}

void CLog::SetComponentLogLevel(const std::vector<CVariant>& components)
{
  m_componentLogLevels = 0;
  for (const auto& component : components)
  {
    if (!component.isInteger())
      continue;

    m_componentLogLevels |= static_cast<uint32_t>(component.asInteger());
  }
}

void CLog::FormatLineBreaks(std::string& message)
{
  StringUtils::Replace(message, "\n", "\n                                                   ");
}
