/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"

#include "Settings.h"
#include "Application.h"
#include "Autorun.h"
#include "LangInfo.h"
#include "Util.h"
#include "addons/Skin.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "cores/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "input/KeyboardLayoutManager.h"
#if defined(TARGET_POSIX)
#include "linux/LinuxTimezone.h"
#endif // defined(TARGET_POSIX)
#include "network/NetworkServices.h"
#include "network/upnp/UPnPSettings.h"
#include "network/WakeOnAccess.h"
#if defined(TARGET_DARWIN_OSX)
#include "osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN)
#include "osx/DarwinUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "SettingAddon.h"
#endif
#if defined(TARGET_RASPBERRY_PI)
#include "linux/RBP.h"
#endif
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif // defined(HAS_LIBAMCODEC)
#include "peripherals/Peripherals.h"
#include "powermanagement/PowerManager.h"
#include "profiles/ProfilesManager.h"
#include "pvr/PVRManager.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingUtils.h"
#include "settings/SkinSettings.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Weather.h"
#include "utils/XBMCTinyXML.h"
#include "utils/SeekHandler.h"
#include "view/ViewStateSettings.h"
#include "input/InputManager.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"
#define SETTINGS_XML_ROOT   "settings"

using namespace XFILE;

CSettings::CSettings()
  : m_initialized(false)
{
  m_settingsManager = new CSettingsManager();
}

CSettings::~CSettings()
{
  Uninitialize();

  delete m_settingsManager;
}

CSettings& CSettings::Get()
{
  static CSettings sSettings;
  return sSettings;
}

bool CSettings::Initialize()
{
  CSingleLock lock(m_critical);
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

  m_settingsManager->SetInitialized();

  InitializeISettingsHandlers();  
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

bool CSettings::Load()
{
  return Load(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !m_settingsManager->Load(xmlDoc.RootElement(), updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from %s, creating new default settings", file.c_str());
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

bool CSettings::Load(const TiXmlElement *root, bool hide /* = false */)
{
  if (root == NULL)
    return false;

  std::map<std::string, CSetting*> *loadedSettings = NULL;
  if (hide)
    loadedSettings = new std::map<std::string, CSetting*>();

  bool updated;
  // only trigger settings events if hiding is disabled
  bool success = m_settingsManager->Load(root, updated, !hide, loadedSettings);
  // if necessary hide all the loaded settings
  if (success && hide && loadedSettings != NULL)
  {
    for(std::map<std::string, CSetting*>::const_iterator setting = loadedSettings->begin(); setting != loadedSettings->end(); ++setting)
      setting->second->SetVisible(false);
  }
  delete loadedSettings;

  return success;
}

void CSettings::SetLoaded()
{
  m_settingsManager->SetLoaded();
}

bool CSettings::Save()
{
  return Save(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  if (!m_settingsManager->Save(root))
    return false;

  return xmlDoc.SaveFile(file);
}

void CSettings::Unload()
{
  CSingleLock lock(m_critical);
  m_settingsManager->Unload();
}

void CSettings::Uninitialize()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  // unregister setting option fillers
  m_settingsManager->UnregisterSettingOptionsFiller("audiocdactions");
  m_settingsManager->UnregisterSettingOptionsFiller("audiocdencoders");
  m_settingsManager->UnregisterSettingOptionsFiller("aequalitylevels");
  m_settingsManager->UnregisterSettingOptionsFiller("audiodevices");
  m_settingsManager->UnregisterSettingOptionsFiller("audiodevicespassthrough");
  m_settingsManager->UnregisterSettingOptionsFiller("audiostreamsilence");
  m_settingsManager->UnregisterSettingOptionsFiller("charsets");
  m_settingsManager->UnregisterSettingOptionsFiller("epgguideviews");
  m_settingsManager->UnregisterSettingOptionsFiller("fontheights");
  m_settingsManager->UnregisterSettingOptionsFiller("fonts");
  m_settingsManager->UnregisterSettingOptionsFiller("languagenames");
  m_settingsManager->UnregisterSettingOptionsFiller("refreshchangedelays");
  m_settingsManager->UnregisterSettingOptionsFiller("refreshrates");
  m_settingsManager->UnregisterSettingOptionsFiller("regions");
  m_settingsManager->UnregisterSettingOptionsFiller("shortdateformats");
  m_settingsManager->UnregisterSettingOptionsFiller("longdateformats");
  m_settingsManager->UnregisterSettingOptionsFiller("timeformats");
  m_settingsManager->UnregisterSettingOptionsFiller("24hourclockformats");
  m_settingsManager->UnregisterSettingOptionsFiller("speedunits");
  m_settingsManager->UnregisterSettingOptionsFiller("temperatureunits");
  m_settingsManager->UnregisterSettingOptionsFiller("rendermethods");
  m_settingsManager->UnregisterSettingOptionsFiller("resolutions");
  m_settingsManager->UnregisterSettingOptionsFiller("screens");
  m_settingsManager->UnregisterSettingOptionsFiller("stereoscopicmodes");
  m_settingsManager->UnregisterSettingOptionsFiller("preferedstereoscopicviewmodes");
  m_settingsManager->UnregisterSettingOptionsFiller("monitors");
  m_settingsManager->UnregisterSettingOptionsFiller("videoseeksteps");
  m_settingsManager->UnregisterSettingOptionsFiller("shutdownstates");
  m_settingsManager->UnregisterSettingOptionsFiller("startupwindows");
  m_settingsManager->UnregisterSettingOptionsFiller("streamlanguages");
  m_settingsManager->UnregisterSettingOptionsFiller("iso6391languages");
  m_settingsManager->UnregisterSettingOptionsFiller("skincolors");
  m_settingsManager->UnregisterSettingOptionsFiller("skinfonts");
  m_settingsManager->UnregisterSettingOptionsFiller("skinsounds");
  m_settingsManager->UnregisterSettingOptionsFiller("skinthemes");
#if defined(TARGET_LINUX)
  m_settingsManager->UnregisterSettingOptionsFiller("timezonecountries");
  m_settingsManager->UnregisterSettingOptionsFiller("timezones");
#endif // defined(TARGET_LINUX)
  m_settingsManager->UnregisterSettingOptionsFiller("verticalsyncs");
  m_settingsManager->UnregisterSettingOptionsFiller("keyboardlayouts");

  // unregister ISettingCallback implementations
  m_settingsManager->UnregisterCallback(&g_advancedSettings);
  m_settingsManager->UnregisterCallback(&CMediaSettings::Get());
  m_settingsManager->UnregisterCallback(&CDisplaySettings::Get());
  m_settingsManager->UnregisterCallback(&CSeekHandler::Get());
  m_settingsManager->UnregisterCallback(&CStereoscopicsManager::Get());
  m_settingsManager->UnregisterCallback(&g_application);
  m_settingsManager->UnregisterCallback(&g_audioManager);
  m_settingsManager->UnregisterCallback(&g_charsetConverter);
  m_settingsManager->UnregisterCallback(&g_graphicsContext);
  m_settingsManager->UnregisterCallback(&g_langInfo);
  m_settingsManager->UnregisterCallback(&CInputManager::Get());
  m_settingsManager->UnregisterCallback(&CNetworkServices::Get());
  m_settingsManager->UnregisterCallback(&g_passwordManager);
  m_settingsManager->UnregisterCallback(&PVR::g_PVRManager);
  m_settingsManager->UnregisterCallback(&CRssManager::Get());
#if defined(TARGET_LINUX)
  m_settingsManager->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
  m_settingsManager->UnregisterCallback(&g_weatherManager);
  m_settingsManager->UnregisterCallback(&PERIPHERALS::CPeripherals::Get());
#if defined(TARGET_DARWIN_OSX)
  m_settingsManager->UnregisterCallback(&XBMCHelper::GetInstance());
#endif

  // cleanup the settings manager
  m_settingsManager->Clear();

  // unregister ISubSettings implementations
  m_settingsManager->UnregisterSubSettings(&g_application);
  m_settingsManager->UnregisterSubSettings(&CDisplaySettings::Get());
  m_settingsManager->UnregisterSubSettings(&CMediaSettings::Get());
  m_settingsManager->UnregisterSubSettings(&CSkinSettings::Get());
  m_settingsManager->UnregisterSubSettings(&g_sysinfo);
  m_settingsManager->UnregisterSubSettings(&CViewStateSettings::Get());

  // unregister ISettingsHandler implementations
  m_settingsManager->UnregisterSettingsHandler(&g_advancedSettings);
  m_settingsManager->UnregisterSettingsHandler(&CMediaSourceSettings::Get());
  m_settingsManager->UnregisterSettingsHandler(&CPlayerCoreFactory::Get());
  m_settingsManager->UnregisterSettingsHandler(&CProfilesManager::Get());
#ifdef HAS_UPNP
  m_settingsManager->UnregisterSettingsHandler(&CUPnPSettings::Get());
#endif
  m_settingsManager->UnregisterSettingsHandler(&CWakeOnAccess::Get());
  m_settingsManager->UnregisterSettingsHandler(&CRssManager::Get());
  m_settingsManager->UnregisterSettingsHandler(&g_langInfo);
  m_settingsManager->UnregisterSettingsHandler(&g_application);
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
  m_settingsManager->UnregisterSettingsHandler(&g_timezone);
#endif

  m_initialized = false;
}

void CSettings::RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList)
{
  m_settingsManager->RegisterCallback(callback, settingList);
}

void CSettings::UnregisterCallback(ISettingCallback *callback)
{
  m_settingsManager->UnregisterCallback(callback);
}

CSetting* CSettings::GetSetting(const std::string &id) const
{
  CSingleLock lock(m_critical);
  if (id.empty())
    return NULL;

  return m_settingsManager->GetSetting(id);
}

std::vector<CSettingSection*> CSettings::GetSections() const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetSections();
}

CSettingSection* CSettings::GetSection(const std::string &section) const
{
  CSingleLock lock(m_critical);
  if (section.empty())
    return NULL;

  return m_settingsManager->GetSection(section);
}

bool CSettings::GetBool(const std::string &id) const
{
  // Backward compatibility (skins use this setting)
  if (StringUtils::EqualsNoCase(id, "lookandfeel.enablemouse"))
    return GetBool("input.enablemouse");

  return m_settingsManager->GetBool(id);
}

bool CSettings::SetBool(const std::string &id, bool value)
{
  return m_settingsManager->SetBool(id, value);
}

bool CSettings::ToggleBool(const std::string &id)
{
  return m_settingsManager->ToggleBool(id);
}

int CSettings::GetInt(const std::string &id) const
{
  return m_settingsManager->GetInt(id);
}

bool CSettings::SetInt(const std::string &id, int value)
{
  return m_settingsManager->SetInt(id, value);
}

double CSettings::GetNumber(const std::string &id) const
{
  return m_settingsManager->GetNumber(id);
}

bool CSettings::SetNumber(const std::string &id, double value)
{
  return m_settingsManager->SetNumber(id, value);
}

std::string CSettings::GetString(const std::string &id) const
{
  return m_settingsManager->GetString(id);
}

bool CSettings::SetString(const std::string &id, const std::string &value)
{
  return m_settingsManager->SetString(id, value);
}

std::vector<CVariant> CSettings::GetList(const std::string &id) const
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeList)
    return std::vector<CVariant>();

  return CSettingUtils::GetList(static_cast<CSettingList*>(setting));
}

bool CSettings::SetList(const std::string &id, const std::vector<CVariant> &value)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeList)
    return false;

  return CSettingUtils::SetList(static_cast<CSettingList*>(setting), value);
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return m_settingsManager->LoadSetting(node, settingId);
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %d\n%s", file.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from %s", file.c_str());
  
  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  return m_settingsManager->Initialize(root);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(TARGET_WINDOWS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-android.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-android.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-android-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_RASPBERRY_PI)
  if (CFile::Exists(SETTINGS_XML_FOLDER "rbp.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp-specific settings definitions");
  if (g_RBP.RasberryPiVersion() > 1 && CFile::Exists(SETTINGS_XML_FOLDER "rbp2.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp2.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp2-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(HAS_IMXVPU)
  if (CFile::Exists(SETTINGS_XML_FOLDER "imx6.xml") && !Initialize(SETTINGS_XML_FOLDER "imx6.xml"))
    CLog::Log(LOGFATAL, "Unable to load imx6-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-linux.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-linux-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#if defined(TARGET_DARWIN_IOS_ATV2)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios_atv2.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios_atv2.xml"))
    CLog::Log(LOGFATAL, "Unable to load atv2-specific settings definitions");
#endif
#endif
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
  // register "addon" and "path" setting types implemented by CSettingAddon
  m_settingsManager->RegisterSettingType("addon", this);
  m_settingsManager->RegisterSettingType("path", this);
}

void CSettings::InitializeControls()
{
  m_settingsManager->RegisterSettingControl("toggle", this);
  m_settingsManager->RegisterSettingControl("spinner", this);
  m_settingsManager->RegisterSettingControl("edit", this);
  m_settingsManager->RegisterSettingControl("button", this);
  m_settingsManager->RegisterSettingControl("list", this);
  m_settingsManager->RegisterSettingControl("slider", this);
  m_settingsManager->RegisterSettingControl("range", this);
  m_settingsManager->RegisterSettingControl("title", this);
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
#if defined(TARGET_DARWIN)
  CSettingString* timezonecountry = (CSettingString*)m_settingsManager->GetSetting("locale.timezonecountry");
  CSettingString* timezone = (CSettingString*)m_settingsManager->GetSetting("locale.timezone");

  if (!g_sysinfo.IsAppleTV2() || CDarwinUtils::GetIOSVersion() >= 4.3)
  {
    timezonecountry->SetRequirementsMet(false);
    timezone->SetRequirementsMet(false);
  }
#endif
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(HAS_TOUCH_SKIN) && defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
  ((CSettingAddon*)m_settingsManager->GetSetting("lookandfeel.skin"))->SetDefault("skin.re-touched");
#endif

#if defined(TARGET_POSIX)
  CSettingString* timezonecountry = (CSettingString*)m_settingsManager->GetSetting("locale.timezonecountry");
  CSettingString* timezone = (CSettingString*)m_settingsManager->GetSetting("locale.timezone");

  if (timezonecountry->IsVisible())
    timezonecountry->SetDefault(g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()));
  if (timezone->IsVisible())
    timezone->SetDefault(g_timezone.GetOSConfiguredTimezone());
#endif // defined(TARGET_POSIX)

#if defined(TARGET_WINDOWS)
  #if defined(HAS_DX)
  ((CSettingString*)m_settingsManager->GetSetting("musicplayer.visualisation"))->SetDefault("visualization.milkdrop");
  #endif

  #if !defined(HAS_GL)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
    ((CSettingBool*)m_settingsManager->GetSetting("videoscreen.fakefullscreen"))->SetDefault(false);
  #endif
#endif

#if !defined(TARGET_WINDOWS)
  ((CSettingString*)m_settingsManager->GetSetting("audiooutput.audiodevice"))->SetDefault(CAEFactory::GetDefaultDevice(false));
  ((CSettingString*)m_settingsManager->GetSetting("audiooutput.passthroughdevice"))->SetDefault(CAEFactory::GetDefaultDevice(true));
#endif

  if (g_application.IsStandAlone())
    ((CSettingInt*)m_settingsManager->GetSetting("powermanagement.shutdownstate"))->SetDefault(POWERSTATE_SHUTDOWN);

#if defined(HAS_WEB_SERVER)
  if (CUtil::CanBindPrivileged())
    ((CSettingInt*)m_settingsManager->GetSetting("services.webserverport"))->SetDefault(80);
#endif
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_DVD_DRIVE
  m_settingsManager->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("aequalitylevels", CAEFactory::SettingOptionsAudioQualityLevelsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiodevices", CAEFactory::SettingOptionsAudioDevicesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiodevicespassthrough", CAEFactory::SettingOptionsAudioDevicesPassthroughFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiostreamsilence", CAEFactory::SettingOptionsAudioStreamsilenceFiller);
  m_settingsManager->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("refreshchangedelays", CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller);
  m_settingsManager->RegisterSettingOptionsFiller("refreshrates", CDisplaySettings::SettingOptionsRefreshRatesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsScreensFiller);
  m_settingsManager->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("monitors", CDisplaySettings::SettingOptionsMonitorsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("shutdownstates", CPowerManager::SettingOptionsShutdownStatesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("streamlanguages", CLangInfo::SettingOptionsStreamLanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinsounds", ADDON::CSkinInfo::SettingOptionsSkinSoundFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
#ifdef TARGET_LINUX
  m_settingsManager->RegisterSettingOptionsFiller("timezonecountries", CLinuxTimezone::SettingOptionsTimezoneCountriesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("timezones", CLinuxTimezone::SettingOptionsTimezonesFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("verticalsyncs", CDisplaySettings::SettingOptionsVerticalSyncsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("keyboardlayouts", CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("loggingcomponents", CAdvancedSettings::SettingOptionsLoggingComponentsFiller);
}

void CSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string> &simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator itCondition = simpleConditions.begin(); itCondition != simpleConditions.end(); ++itCondition)
    m_settingsManager->AddCondition(*itCondition);

  // add more complex conditions
  const std::map<std::string, SettingConditionCheck> &complexConditions = CSettingConditions::GetComplexConditions();
  for (std::map<std::string, SettingConditionCheck>::const_iterator itCondition = complexConditions.begin(); itCondition != complexConditions.end(); ++itCondition)
    m_settingsManager->AddCondition(itCondition->first, itCondition->second);
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  m_settingsManager->RegisterSettingsHandler(&g_advancedSettings);
  m_settingsManager->RegisterSettingsHandler(&CMediaSourceSettings::Get());
  m_settingsManager->RegisterSettingsHandler(&CPlayerCoreFactory::Get());
  m_settingsManager->RegisterSettingsHandler(&CProfilesManager::Get());
#ifdef HAS_UPNP
  m_settingsManager->RegisterSettingsHandler(&CUPnPSettings::Get());
#endif
  m_settingsManager->RegisterSettingsHandler(&CWakeOnAccess::Get());
  m_settingsManager->RegisterSettingsHandler(&CRssManager::Get());
  m_settingsManager->RegisterSettingsHandler(&g_langInfo);
  m_settingsManager->RegisterSettingsHandler(&g_application);
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
  m_settingsManager->RegisterSettingsHandler(&g_timezone);
#endif
  m_settingsManager->RegisterSettingsHandler(&CMediaSettings::Get());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  m_settingsManager->RegisterSubSettings(&g_application);
  m_settingsManager->RegisterSubSettings(&CDisplaySettings::Get());
  m_settingsManager->RegisterSubSettings(&CMediaSettings::Get());
  m_settingsManager->RegisterSubSettings(&CSkinSettings::Get());
  m_settingsManager->RegisterSubSettings(&g_sysinfo);
  m_settingsManager->RegisterSubSettings(&CViewStateSettings::Get());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert("debug.showloginfo");
  settingSet.insert("debug.extralogging");
  settingSet.insert("debug.setextraloglevel");
  m_settingsManager->RegisterCallback(&g_advancedSettings, settingSet);

  settingSet.clear();
  settingSet.insert("karaoke.export");
  settingSet.insert("karaoke.importcsv");
  settingSet.insert("musiclibrary.cleanup");
  settingSet.insert("musiclibrary.export");
  settingSet.insert("musiclibrary.import");
  settingSet.insert("musicfiles.trackformat");
  settingSet.insert("musicfiles.trackformatright");
  settingSet.insert("videolibrary.flattentvshows");
  settingSet.insert("videolibrary.removeduplicates");
  settingSet.insert("videolibrary.groupmoviesets");
  settingSet.insert("videolibrary.cleanup");
  settingSet.insert("videolibrary.import");
  settingSet.insert("videolibrary.export");
  m_settingsManager->RegisterCallback(&CMediaSettings::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("videoscreen.screen");
  settingSet.insert("videoscreen.resolution");
  settingSet.insert("videoscreen.screenmode");
  settingSet.insert("videoscreen.vsync");
  settingSet.insert("videoscreen.monitor");
  settingSet.insert("videoscreen.preferedstereoscopicmode");
  m_settingsManager->RegisterCallback(&CDisplaySettings::Get(), settingSet);
  
  settingSet.clear();
  settingSet.insert("videoplayer.seekdelay");
  settingSet.insert("videoplayer.seeksteps");
  settingSet.insert("musicplayer.seekdelay");
  settingSet.insert("musicplayer.seeksteps");
  m_settingsManager->RegisterCallback(&CSeekHandler::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("videoscreen.stereoscopicmode");
  m_settingsManager->RegisterCallback(&CStereoscopicsManager::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("audiooutput.config");
  settingSet.insert("audiooutput.samplerate");
  settingSet.insert("audiooutput.passthrough");
  settingSet.insert("audiooutput.channels");
  settingSet.insert("audiooutput.processquality");
  settingSet.insert("audiooutput.guisoundmode");
  settingSet.insert("audiooutput.stereoupmix");
  settingSet.insert("audiooutput.ac3passthrough");
  settingSet.insert("audiooutput.ac3transcode");
  settingSet.insert("audiooutput.eac3passthrough");
  settingSet.insert("audiooutput.dtspassthrough");
  settingSet.insert("audiooutput.truehdpassthrough");
  settingSet.insert("audiooutput.dtshdpassthrough");
  settingSet.insert("audiooutput.audiodevice");
  settingSet.insert("audiooutput.passthroughdevice");
  settingSet.insert("audiooutput.streamsilence");
  settingSet.insert("audiooutput.maintainoriginalvolume");
  settingSet.insert("lookandfeel.skin");
  settingSet.insert("lookandfeel.skinsettings");
  settingSet.insert("lookandfeel.font");
  settingSet.insert("lookandfeel.skintheme");
  settingSet.insert("lookandfeel.skincolors");
  settingSet.insert("lookandfeel.skinzoom");
  settingSet.insert("musicplayer.replaygainpreamp");
  settingSet.insert("musicplayer.replaygainnogainpreamp");
  settingSet.insert("musicplayer.replaygaintype");
  settingSet.insert("musicplayer.replaygainavoidclipping");
  settingSet.insert("scrapers.musicvideosdefault");
  settingSet.insert("screensaver.mode");
  settingSet.insert("screensaver.preview");
  settingSet.insert("screensaver.settings");
  settingSet.insert("audiocds.settings");
  settingSet.insert("videoscreen.guicalibration");
  settingSet.insert("videoscreen.testpattern");
  settingSet.insert("videoplayer.useamcodec");
  settingSet.insert("videoplayer.usemediacodec");
  m_settingsManager->RegisterCallback(&g_application, settingSet);

  settingSet.clear();
  settingSet.insert("lookandfeel.soundskin");
  m_settingsManager->RegisterCallback(&g_audioManager, settingSet);

  settingSet.clear();
  settingSet.insert("subtitles.charset");
  settingSet.insert("karaoke.charset");
  settingSet.insert("locale.charset");
  m_settingsManager->RegisterCallback(&g_charsetConverter, settingSet);

  settingSet.clear();
  settingSet.insert("videoscreen.fakefullscreen");
  m_settingsManager->RegisterCallback(&g_graphicsContext, settingSet);

  settingSet.clear();
  settingSet.insert("locale.audiolanguage");
  settingSet.insert("locale.subtitlelanguage");
  settingSet.insert("locale.language");
  settingSet.insert("locale.country");
  settingSet.insert("locale.shortdateformat");
  settingSet.insert("locale.longdateformat");
  settingSet.insert("locale.timeformat");
  settingSet.insert("locale.use24hourclock");
  settingSet.insert("locale.temperatureunit");
  settingSet.insert("locale.speedunit");
  m_settingsManager->RegisterCallback(&g_langInfo, settingSet);

  settingSet.clear();
  settingSet.insert("input.enablejoystick");
  settingSet.insert("input.enablemouse");
  m_settingsManager->RegisterCallback(&CInputManager::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("services.webserver");
  settingSet.insert("services.webserverport");
  settingSet.insert("services.webserverusername");
  settingSet.insert("services.webserverpassword");
  settingSet.insert("services.zeroconf");
  settingSet.insert("services.airplay");
  settingSet.insert("services.airplayvolumecontrol");
  settingSet.insert("services.useairplaypassword");
  settingSet.insert("services.airplaypassword");
  settingSet.insert("services.upnpserver");
  settingSet.insert("services.upnprenderer");
  settingSet.insert("services.upnpcontroller");
  settingSet.insert("services.esenabled");
  settingSet.insert("services.esport");
  settingSet.insert("services.esallinterfaces");
  settingSet.insert("services.esinitialdelay");
  settingSet.insert("services.escontinuousdelay");
  settingSet.insert("smb.winsserver");
  settingSet.insert("smb.workgroup");
  m_settingsManager->RegisterCallback(&CNetworkServices::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("masterlock.lockcode");
  m_settingsManager->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert("pvrmanager.enabled");
  settingSet.insert("pvrmanager.channelmanager");
  settingSet.insert("pvrmanager.groupmanager");
  settingSet.insert("pvrmanager.channelscan");
  settingSet.insert("pvrmanager.resetdb");
  settingSet.insert("pvrclient.menuhook");
  settingSet.insert("pvrmenu.searchicons");
  settingSet.insert("epg.resetepg");
  settingSet.insert("pvrparental.enabled");
  m_settingsManager->RegisterCallback(&PVR::g_PVRManager, settingSet);

  settingSet.clear();
  settingSet.insert("lookandfeel.rssedit");
  m_settingsManager->RegisterCallback(&CRssManager::Get(), settingSet);

#if defined(TARGET_LINUX)
  settingSet.clear();
  settingSet.insert("locale.timezone");
  settingSet.insert("locale.timezonecountry");
  m_settingsManager->RegisterCallback(&g_timezone, settingSet);
#endif

  settingSet.clear();
  settingSet.insert("weather.addon");
  settingSet.insert("weather.addonsettings");
  m_settingsManager->RegisterCallback(&g_weatherManager, settingSet);

  settingSet.clear();
  settingSet.insert("input.peripherals");
  settingSet.insert("locale.language");
  m_settingsManager->RegisterCallback(&PERIPHERALS::CPeripherals::Get(), settingSet);

#if defined(TARGET_DARWIN_OSX)
  settingSet.clear();
  settingSet.insert("input.appleremotemode");
  settingSet.insert("input.appleremotealwayson");
  m_settingsManager->RegisterCallback(&XBMCHelper::GetInstance(), settingSet);
#endif
}

bool CSettings::Reset()
{
  std::string settingsFile = CProfilesManager::Get().GetSettingsFile();
  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at %s", settingsFile.c_str());
  
  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to %s", settingsFile.c_str());
    return false;
  }

  return true;
}
