/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Settings.h"

#include "Autorun.h"
#include "GUIPassword.h"
#include "LangInfo.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Skin.h"
#include "application/AppParams.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "input/keyboard/KeyboardLayoutManager.h"

#include <mutex>
#include "network/upnp/UPnPSettings.h"
#include "network/WakeOnAccess.h"
#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
#include "platform/darwin/osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/TVOSSettingsHandler.h"
#endif // defined(TARGET_DARWIN_TVOS)
#if defined(TARGET_DARWIN_EMBEDDED)
#include "SettingAddon.h"
#endif
#include "DiscSettings.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "powermanagement/PowerTypes.h"
#include "profiles/ProfileManager.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/ServicesSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/SubtitlesSettings.h"
#include "settings/lib/SettingsManager.h"
#include "utils/CharsetConverter.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"

using namespace KODI;
using namespace XFILE;

bool CSettings::Initialize()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  GetSettingsManager()->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

void CSettings::RegisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == nullptr)
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == nullptr)
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);
  m_subSettings.erase(subSettings);
}

bool CSettings::Load()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Load(profileManager->GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !Load(xmlDoc.RootElement(), updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from {}, creating new default settings",
              file);
    if (!Reset())
      return false;

    if (!Load(file))
      return false;
  }
  // if the settings had to be updated, we need to save the changes
  else if (updated)
    return Save(file);

  return true;
}

bool CSettings::Load(const TiXmlElement* root)
{
  bool updated = false;
  return Load(root, updated);
}

bool CSettings::Save()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Save(profileManager->GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!SaveValuesToXml(xmlDoc))
    return false;

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == nullptr)
    return false;

  if (!Save(root))
    return false;

  return xmlDoc.SaveFile(file);
}

bool CSettings::Save(TiXmlNode* root) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  // save any ISubSettings implementations
  for (const auto& subSetting : m_subSettings)
  {
    if (!subSetting->Save(root))
      return false;
  }

  return true;
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return GetSettingsManager()->LoadSetting(node, settingId);
}

bool CSettings::GetBool(const std::string& id) const
{
  // Backward compatibility (skins use this setting)
  if (StringUtils::EqualsNoCase(id, "lookandfeel.enablemouse"))
    return CSettingsBase::GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE);

  return CSettingsBase::GetBool(id);
}

void CSettings::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (!m_initialized)
    return;

  GetSettingsManager()->Clear();

  for (auto& subSetting : m_subSettings)
    subSetting->Clear();

  m_initialized = false;
}

bool CSettings::Load(const TiXmlElement* root, bool& updated)
{
  if (root == nullptr)
    return false;

  if (!CSettingsBase::LoadValuesFromXml(root, updated))
    return false;

  return Load(static_cast<const TiXmlNode*>(root));
}

bool CSettings::Load(const TiXmlNode* settings)
{
  bool ok = true;
  std::unique_lock<CCriticalSection> lock(m_critical);
  for (const auto& subSetting : m_subSettings)
    ok &= subSetting->Load(settings);

  return ok;
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from {}, Line {}\n{}", file,
              xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from {}", file);

  return InitializeDefinitionsFromXml(xmlDoc);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(TARGET_WINDOWS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "windows.xml") && !Initialize(SETTINGS_XML_FOLDER "windows.xml"))
    CLog::Log(LOGFATAL, "Unable to load windows-specific settings definitions");
#if defined(TARGET_WINDOWS_DESKTOP)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_WINDOWS_STORE)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win10.xml") && !Initialize(SETTINGS_XML_FOLDER "win10.xml"))
    CLog::Log(LOGFATAL, "Unable to load win10-specific settings definitions");
#endif
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#elif defined(TARGET_DARWIN_TVOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_tvos.xml") &&
      !Initialize(SETTINGS_XML_FOLDER "darwin_tvos.xml"))
    CLog::Log(LOGFATAL, "Unable to load tvos-specific settings definitions");
#endif
#endif

#if defined(PLATFORM_SETTINGS_FILE)
  if (CFile::Exists(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)) && !Initialize(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)))
    CLog::Log(LOGFATAL, "Unable to load platform-specific settings definitions ({})",
              DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE));
#endif

  // load any custom visibility and default values before loading the special
  // appliance.xml so that appliances are able to overwrite even those values
  InitializeVisibility();
  InitializeDefaults();

  if (CFile::Exists(SETTINGS_XML_FOLDER "appliance.xml") && !Initialize(SETTINGS_XML_FOLDER "appliance.xml"))
    CLog::Log(LOGFATAL, "Unable to load appliance-specific settings definitions");

  return true;
}

void CSettings::InitializeSettingTypes()
{
  GetSettingsManager()->RegisterSettingType("addon", this);
  GetSettingsManager()->RegisterSettingType("date", this);
  GetSettingsManager()->RegisterSettingType("path", this);
  GetSettingsManager()->RegisterSettingType("time", this);
}

void CSettings::InitializeControls()
{
  GetSettingsManager()->RegisterSettingControl("toggle", this);
  GetSettingsManager()->RegisterSettingControl("spinner", this);
  GetSettingsManager()->RegisterSettingControl("edit", this);
  GetSettingsManager()->RegisterSettingControl("button", this);
  GetSettingsManager()->RegisterSettingControl("list", this);
  GetSettingsManager()->RegisterSettingControl("slider", this);
  GetSettingsManager()->RegisterSettingControl("range", this);
  GetSettingsManager()->RegisterSettingControl("title", this);
  GetSettingsManager()->RegisterSettingControl("colorbutton", this);
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(TARGET_WINDOWS)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
  {
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    else
      std::static_pointer_cast<CSettingBool>(setting)->SetDefault(false);
  }
#endif

  if (CServiceBroker::GetAppParams()->IsStandAlone())
  {
    auto setting =
        GetSettingsManager()->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    else
      std::static_pointer_cast<CSettingInt>(setting)->SetDefault(POWERSTATE_SHUTDOWN);
  }

  // Initialize deviceUUID if not already set, used in zeroconf advertisements.
  std::shared_ptr<CSettingString> deviceUUID = std::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID));
  if (deviceUUID->GetValue().empty())
  {
    const std::string& uuid = StringUtils::CreateUUID();
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}", CSettings::SETTING_SERVICES_DEVICEUUID);
    else
      std::static_pointer_cast<CSettingString>(setting)->SetValue(uuid);
  }
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_OPTICAL_DRIVE
  GetSettingsManager()->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "subtitlesfonts", SUBTITLES::CSubtitlesSettings::SettingOptionsSubtitleFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshchangedelays", CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshrates", CDisplaySettings::SettingOptionsRefreshRatesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("modes", CDisplaySettings::SettingOptionsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsDispModeFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("monitors", CDisplaySettings::SettingOptionsMonitorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsmodes", CDisplaySettings::SettingOptionsCmsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmswhitepoints", CDisplaySettings::SettingOptionsCmsWhitepointsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsprimaries", CDisplaySettings::SettingOptionsCmsPrimariesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsgammamodes", CDisplaySettings::SettingOptionsCmsGammaModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "keyboardlayouts", KEYBOARD::CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filechunksizes", CServicesSettings::SettingOptionsChunkSizesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachebuffermodes", CServicesSettings::SettingOptionsBufferModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachememorysizes", CServicesSettings::SettingOptionsMemorySizesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachereadfactors", CServicesSettings::SettingOptionsReadFactorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachechunksizes", CServicesSettings::SettingOptionsCacheChunkSizesFiller);
}

void CSettings::UninitializeOptionFillers()
{
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdactions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdencoders");
  GetSettingsManager()->UnregisterSettingOptionsFiller("charsets");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fontheights");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlesfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("languagenames");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshchangedelays");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshrates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("regions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shortdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("longdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timeformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("24hourclockformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("speedunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("temperatureunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("rendermethods");
  GetSettingsManager()->UnregisterSettingOptionsFiller("resolutions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("screens");
  GetSettingsManager()->UnregisterSettingOptionsFiller("stereoscopicmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("preferedstereoscopicviewmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("monitors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmswhitepoints");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsprimaries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsgammamodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("videoseeksteps");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shutdownstates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("startupwindows");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlestreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitledownloadlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("iso6391languages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skincolors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinthemes");
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezonecountries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezones");
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("verticalsyncs");
  GetSettingsManager()->UnregisterSettingOptionsFiller("keyboardlayouts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filechunksizes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachebuffermodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachememorysizes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachereadfactors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachechunksizes");
}

void CSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string> &simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator itCondition = simpleConditions.begin(); itCondition != simpleConditions.end(); ++itCondition)
    GetSettingsManager()->AddCondition(*itCondition);

  // add more complex conditions
  const std::map<std::string, SettingConditionCheck> &complexConditions = CSettingConditions::GetComplexConditions();
  for (std::map<std::string, SettingConditionCheck>::const_iterator itCondition = complexConditions.begin(); itCondition != complexConditions.end(); ++itCondition)
    GetSettingsManager()->AddDynamicCondition(itCondition->first, itCondition->second);
}

void CSettings::UninitializeConditions()
{
  CSettingConditions::Deinitialize();
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSourceSettings::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->RegisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CWakeOnAccess::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSettings::GetInstance());
}

void CSettings::UninitializeISettingsHandlers()
{
  // unregister ISettingsHandler implementations
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->UnregisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterSettingsHandler(&CWakeOnAccess::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->UnregisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSourceSettings::GetInstance());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  RegisterSubSettings(&CDisplaySettings::GetInstance());
  RegisterSubSettings(&CMediaSettings::GetInstance());
  RegisterSubSettings(&CSkinSettings::GetInstance());
  RegisterSubSettings(&g_sysinfo);
  RegisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::UninitializeISubSettings()
{
  // unregister ISubSettings implementations
  UnregisterSubSettings(&CDisplaySettings::GetInstance());
  UnregisterSubSettings(&CMediaSettings::GetInstance());
  UnregisterSubSettings(&CSkinSettings::GetInstance());
  UnregisterSubSettings(&g_sysinfo);
  UnregisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS);
  GetSettingsManager()->RegisterCallback(&CMediaSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREEN);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_RESOLUTION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREENMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_3DLUT);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_10BITSURFACES);
  GetSettingsManager()->RegisterCallback(&CDisplaySettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SUBTITLES_CHARSET);
  settingSet.insert(CSettings::SETTING_LOCALE_CHARSET);
  GetSettingsManager()->RegisterCallback(&g_charsetConverter, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_AUDIOLANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_LANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_COUNTRY);
  settingSet.insert(CSettings::SETTING_LOCALE_SHORTDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_LONGDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_USE24HOURCLOCK);
  settingSet.insert(CSettings::SETTING_LOCALE_TEMPERATUREUNIT);
  settingSet.insert(CSettings::SETTING_LOCALE_SPEEDUNIT);
  GetSettingsManager()->RegisterCallback(&g_langInfo, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_MASTERLOCK_LOCKCODE);
  GetSettingsManager()->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_RSSEDIT);
  GetSettingsManager()->RegisterCallback(&CRssManager::GetInstance(), settingSet);

#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEMODE);
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON);
  GetSettingsManager()->RegisterCallback(&XBMCHelper::GetInstance(), settingSet);
#endif

#if defined(TARGET_DARWIN_TVOS)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY);
  GetSettingsManager()->RegisterCallback(&CTVOSInputSettings::GetInstance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_SHOW_RUNNING);
  settingSet.insert(CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES);
  GetSettingsManager()->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS);
  GetSettingsManager()->RegisterCallback(&CWakeOnAccess::GetInstance(), settingSet);

#ifdef HAVE_LIBBLURAY
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_DISC_PLAYBACK);
  GetSettingsManager()->RegisterCallback(&CDiscSettings::GetInstance(), settingSet);
#endif
}

void CSettings::UninitializeISettingCallbacks()
{
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
  GetSettingsManager()->UnregisterCallback(&XBMCHelper::GetInstance());
#endif
  GetSettingsManager()->UnregisterCallback(&CWakeOnAccess::GetInstance());
#ifdef HAVE_LIBBLURAY
  GetSettingsManager()->UnregisterCallback(&CDiscSettings::GetInstance());
#endif
}

bool CSettings::Reset()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  const std::string settingsFile = profileManager->GetSettingsFile();

  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at {}", settingsFile);

  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to {}", settingsFile);
    return false;
  }

  return true;
}
