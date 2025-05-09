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
#include "settings/SettingsContainer.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <cstring>
#include <set>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/dup_filter_sink.h>

namespace
{
constexpr unsigned char Utf8Bom[3] = {0xEF, 0xBB, 0xBF};
const std::string LogFileExtension = ".log";
const std::string LogPattern = "%Y-%m-%d %T.%e T:%-5t %7l <%n>: %v";

struct ComponentInfo
{
  const char* name{nullptr};
  uint32_t stringId{0};
};

// clang-format off
constexpr auto componentMap = make_map<int, ComponentInfo>({
  // component id,  component name, component setting value string id
  {LOGSAMBA,        {"samba",       669}},
  {LOGCURL,         {"curl",        670}},
  {LOGFFMPEG,       {"ffmpeg",      672}},
#ifdef HAS_DBUS
  {LOGDBUS,         {"dbus",        674}},
#endif
  {LOGJSONRPC,      {"jsonrpc",     675}},
  {LOGAUDIO,        {"audio",       676}},
#ifdef HAS_AIRTUNES
  {LOGAIRTUNES,     {"airtunes",    677}},
#endif
#ifdef HAS_UPNP
  {LOGUPNP,         {"upnp",        678}},
#endif
#ifdef HAVE_LIBCEC
  {LOGCEC,          {"cec",         679}},
#endif
  {LOGVIDEO,        {"video",       680}},
#ifdef HAS_WEB_SERVER
  {LOGWEBSERVER,    {"webserver",   681}},
#endif
  {LOGDATABASE,     {"database",    682}},
  {LOGAVTIMING,     {"avtiming",    683}},
  {LOGWINDOWING,    {"windowing",   684}},
  {LOGPVR,          {"pvr",         685}},
  {LOGEPG,          {"epg",         686}},
  {LOGANNOUNCE,     {"announce",    39117}},
#if defined(HAS_FILESYSTEM_SMB)
  {LOGWSDISCOVERY,  {"wsdiscovery", 37050}},
#endif
  {LOGADDONS,       {"addons",      39124}},
});
// clang-format on

} // unnamed namespace

CLog::CLog()
  : m_platform(IPlatformLog::CreatePlatformLog()),
    m_sinks(std::make_shared<spdlog::sinks::dist_sink_mt>()),
    m_defaultLogger(CreateLogger("general"))
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
  if (setting == nullptr)
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
  settingsManager->RegisterCallback(
      this, {CSettings::SETTING_DEBUG_EXTRALOGGING, CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL});

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
  FormatAndLogInternal(spdlog::level::info, LOG_COMPONENT_GENERAL, "Log level changed to \"{}\"",
                       fmt::make_format_args(spdlog::level::to_string_view(spdLevel)));
}

bool CLog::IsLogLevelLogged(int loglevel) const
{
  if (m_logLevel >= LOG_LEVEL_DEBUG)
    return true;
  if (m_logLevel <= LOG_LEVEL_NONE)
    return false;

  return (loglevel & LOGMASK) >= LOGINFO;
}

bool CLog::CanLogComponent(uint32_t component) const
{
  if (component == LOG_COMPONENT_GENERAL)
    return true;

  if (!m_componentLogEnabled)
    return false;

  return ((m_componentLogLevels & component) == component);
}

void CLog::SettingOptionsLoggingComponentsFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current)
{
  for (const auto& [id, names] : componentMap)
  {
    // localized component setting name, component id
    list.emplace_back(g_localizeStrings.Get(names.stringId), id);
  }
}

Logger CLog::GetLoggerById(uint32_t component)
{
  if (component != LOG_COMPONENT_GENERAL)
  {
    const auto it{componentMap.find(component)};
    if (it != componentMap.cend())
      return GetLogger((*it).second.name);
  }
  // default and fallback
  return m_defaultLogger;
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

void CLog::FormatAndLogInternal(spdlog::level::level_enum level,
                                uint32_t component,
                                fmt::string_view format,
                                fmt::format_args args)
{
  if (level < m_defaultLogger->level())
    return;

  auto message = fmt::vformat(format, args);
  FormatLineBreaks(message);
  GetLoggerById(component)->log(level, message);
}

void CLog::FormatAndLogInternal(const std::string& loggerName,
                                spdlog::level::level_enum level,
                                fmt::string_view format,
                                fmt::format_args args)
{
  if (level < m_defaultLogger->level())
    return;

  auto message = fmt::vformat(format, args);
  FormatLineBreaks(message);
  GetLogger(loggerName)->log(level, message);
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

void CLog::FormatLineBreaks(std::string& message) const
{
  // fixup newline alignment, number of spaces should equal prefix length
  StringUtils::Replace(message, "\n", "\n                                                   ");
}
