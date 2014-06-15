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
#include "GUIPassword.h"
#include "LangInfo.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#if defined(HAVE_LIBCRYSTALHD)
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#endif // defined(HAVE_LIBCRYSTALHD)
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "cores/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIFontManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "input/MouseStat.h"
#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK)
#include "input/SDLJoystick.h"
#endif // defined(HAS_SDL_JOYSTICK)
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
#include "peripherals/Peripherals.h"
#include "powermanagement/PowerManager.h"
#include "profiles/ProfilesManager.h"
#include "pvr/PVRManager.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingPath.h"
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
#include "view/ViewStateSettings.h"
#include "windowing/WindowingFactory.h"
#if defined(TARGET_ANDROID)
#include "android/activity/AndroidFeatures.h"
#endif

#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

#if defined(TARGET_DARWIN_OSX)
#include "osx/DarwinUtils.h"
#endif// defined(TARGET_DARWIN_OSX)

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"
#define SETTINGS_XML_ROOT   "settings"

using namespace XFILE;

bool AddonHasSettings(const std::string &condition, const std::string &value, const std::string &settingId)
{
  if (settingId.empty())
    return false;

  CSettingAddon *setting = (CSettingAddon*)CSettings::Get().GetSetting(settingId);
  if (setting == NULL)
    return false;

  ADDON::AddonPtr addon;
  if (!ADDON::CAddonMgr::Get().GetAddon(setting->GetValue(), addon, setting->GetAddonType()) || addon == NULL)
    return false;

  if (addon->Type() == ADDON::ADDON_SKIN)
    return ((ADDON::CSkinInfo*)addon.get())->HasSkinFile("SkinSettings.xml");

  return addon->HasSettings();
}

bool CheckMasterLock(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return g_passwordManager.IsMasterLockUnlocked(StringUtils::EqualsNoCase(value, "true"));
}

bool CheckPVRParentalPin(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return PVR::g_PVRManager.CheckParentalPIN(g_localizeStrings.Get(19262).c_str());
}

bool HasPeripherals(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return PERIPHERALS::g_peripherals.GetNumberOfPeripherals() > 0;
}

bool IsFullscreen(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return g_Windowing.IsFullScreen();
}

bool IsMasterUser(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return g_passwordManager.bMasterUser;
}

bool IsUsingTTFSubtitles(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CUtil::IsUsingTTFSubtitles();
}

bool ProfileCanWriteDatabase(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().canWriteDatabases();
}

bool ProfileCanWriteSources(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().canWriteSources();
}

bool ProfileHasAddons(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().hasAddons();
}

bool ProfileHasDatabase(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().hasDatabases();
}

bool ProfileHasSources(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().hasSources();
}

bool ProfileHasAddonManagerLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().addonmanagerLocked();
}

bool ProfileHasFilesLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().filesLocked();
}

bool ProfileHasMusicLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().musicLocked();
}

bool ProfileHasPicturesLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().picturesLocked();
}

bool ProfileHasProgramsLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().programsLocked();
}

bool ProfileHasSettingsLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  LOCK_LEVEL::SETTINGS_LOCK slValue=LOCK_LEVEL::ALL;
  if (StringUtils::EqualsNoCase(value, "none"))
    slValue = LOCK_LEVEL::NONE;
  else if (StringUtils::EqualsNoCase(value, "standard"))
    slValue = LOCK_LEVEL::STANDARD;
  else if (StringUtils::EqualsNoCase(value, "advanced"))
    slValue = LOCK_LEVEL::ADVANCED;
  else if (StringUtils::EqualsNoCase(value, "expert"))
    slValue = LOCK_LEVEL::EXPERT;
  return slValue <= CProfilesManager::Get().GetCurrentProfile().settingsLockLevel();
}

bool ProfileHasVideosLocked(const std::string &condition, const std::string &value, const std::string &settingId)
{
  return CProfilesManager::Get().GetCurrentProfile().videoLocked();
}

bool ProfileLockMode(const std::string &condition, const std::string &value, const std::string &settingId)
{
  char *tmp = NULL;
  LockType lock = (LockType)strtol(value.c_str(), &tmp, 0);
  if (tmp != NULL && *tmp != '\0')
    return false;

  return CProfilesManager::Get().GetCurrentProfile().getLockMode() == lock;
}

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

CSetting* CSettings::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = NULL */) const
{
  if (StringUtils::EqualsNoCase(settingType, "addon"))
    return new CSettingAddon(settingId, settingsManager);
  else if (StringUtils::EqualsNoCase(settingType, "path"))
    return new CSettingPath(settingId, settingsManager);

  return NULL;
}

ISettingControl* CSettings::CreateControl(const std::string &controlType) const
{
  if (StringUtils::EqualsNoCase(controlType, "toggle"))
    return new CSettingControlCheckmark();
  else if (StringUtils::EqualsNoCase(controlType, "spinner"))
    return new CSettingControlSpinner();
  else if (StringUtils::EqualsNoCase(controlType, "edit"))
    return new CSettingControlEdit();
  else if (StringUtils::EqualsNoCase(controlType, "button"))
    return new CSettingControlButton();
  else if (StringUtils::EqualsNoCase(controlType, "list"))
    return new CSettingControlList();

  return NULL;
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
    CLog::Log(LOGERROR, "CSettingsManager: unable to load settings from %s, creating new default settings", file.c_str());
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
  m_settingsManager->UnregisterSettingOptionsFiller("languages");
  m_settingsManager->UnregisterSettingOptionsFiller("pvrstartlastchannel");
  m_settingsManager->UnregisterSettingOptionsFiller("refreshchangedelays");
  m_settingsManager->UnregisterSettingOptionsFiller("refreshrates");
  m_settingsManager->UnregisterSettingOptionsFiller("regions");
  m_settingsManager->UnregisterSettingOptionsFiller("rendermethods");
  m_settingsManager->UnregisterSettingOptionsFiller("resolutions");
  m_settingsManager->UnregisterSettingOptionsFiller("screens");
  m_settingsManager->UnregisterSettingOptionsFiller("stereoscopicmodes");
  m_settingsManager->UnregisterSettingOptionsFiller("preferedstereoscopicviewmodes");
  m_settingsManager->UnregisterSettingOptionsFiller("shutdownstates");
  m_settingsManager->UnregisterSettingOptionsFiller("startupwindows");
  m_settingsManager->UnregisterSettingOptionsFiller("streamlanguages");
  m_settingsManager->UnregisterSettingOptionsFiller("skincolors");
  m_settingsManager->UnregisterSettingOptionsFiller("skinfonts");
  m_settingsManager->UnregisterSettingOptionsFiller("skinsounds");
  m_settingsManager->UnregisterSettingOptionsFiller("skinthemes");
#if defined(TARGET_LINUX)
  m_settingsManager->UnregisterSettingOptionsFiller("timezonecountries");
  m_settingsManager->UnregisterSettingOptionsFiller("timezones");
#endif // defined(TARGET_LINUX)
  m_settingsManager->UnregisterSettingOptionsFiller("verticalsyncs");

  // unregister ISettingCallback implementations
  m_settingsManager->UnregisterCallback(&g_advancedSettings);
  m_settingsManager->UnregisterCallback(&CMediaSettings::Get());
  m_settingsManager->UnregisterCallback(&CDisplaySettings::Get());
  m_settingsManager->UnregisterCallback(&CStereoscopicsManager::Get());
  m_settingsManager->UnregisterCallback(&g_application);
  m_settingsManager->UnregisterCallback(&g_audioManager);
  m_settingsManager->UnregisterCallback(&g_charsetConverter);
  m_settingsManager->UnregisterCallback(&g_graphicsContext);
  m_settingsManager->UnregisterCallback(&g_langInfo);
#if defined(TARGET_WINDOWS) || defined(HAS_SDL_JOYSTICK)
  m_settingsManager->UnregisterCallback(&g_Joystick);
#endif
  m_settingsManager->UnregisterCallback(&g_Mouse);
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

  CSettingList *listSetting = static_cast<CSettingList*>(setting);
  return ListToValues(listSetting, listSetting->GetValue());
}

bool CSettings::SetList(const std::string &id, const std::vector<CVariant> &value)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeList)
    return false;

  CSettingList *listSetting = static_cast<CSettingList*>(setting);
  SettingPtrList newValues;
  bool ret = true;
  int index = 0;
  for (std::vector<CVariant>::const_iterator itValue = value.begin(); itValue != value.end(); ++itValue)
  {
    CSetting *settingValue = listSetting->GetDefinition()->Clone(StringUtils::Format("%s.%d", listSetting->GetId().c_str(), index++));
    if (settingValue == NULL)
      return false;

    switch (listSetting->GetElementType())
    {
      case SettingTypeBool:
        if (!itValue->isBoolean())
          return false;
        ret = static_cast<CSettingBool*>(settingValue)->SetValue(itValue->asBoolean());
        break;

      case SettingTypeInteger:
        if (!itValue->isInteger())
          return false;
        ret = static_cast<CSettingInt*>(settingValue)->SetValue((int)itValue->asInteger());
        break;

      case SettingTypeNumber:
        if (!itValue->isDouble())
          return false;
        ret = static_cast<CSettingNumber*>(settingValue)->SetValue(itValue->asDouble());
        break;

      case SettingTypeString:
        if (!itValue->isString())
          return false;
        ret = static_cast<CSettingString*>(settingValue)->SetValue(itValue->asString());
        break;

      default:
        ret = false;
        break;
    }

    if (!ret)
    {
      delete settingValue;
      return false;
    }

    newValues.push_back(SettingPtr(settingValue));
  }

  return listSetting->SetValue(newValues);
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return m_settingsManager->LoadSetting(node, settingId);
}

std::vector<CVariant> CSettings::ListToValues(const CSettingList *setting, const std::vector< boost::shared_ptr<CSetting> > &values)
{
  std::vector<CVariant> realValues;

  if (setting == NULL)
    return realValues;

  for (SettingPtrList::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    switch (setting->GetElementType())
    {
      case SettingTypeBool:
        realValues.push_back(static_cast<const CSettingBool*>(it->get())->GetValue());
        break;

      case SettingTypeInteger:
        realValues.push_back(static_cast<const CSettingInt*>(it->get())->GetValue());
        break;

      case SettingTypeNumber:
        realValues.push_back(static_cast<const CSettingNumber*>(it->get())->GetValue());
        break;

      case SettingTypeString:
        realValues.push_back(static_cast<const CSettingString*>(it->get())->GetValue());
        break;

      default:
        break;
    }
  }

  return realValues;
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
#elif defined(TARGET_RASPBERRY_PI)
  if (CFile::Exists(SETTINGS_XML_FOLDER "rbp.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp-specific settings definitions");
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
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
#if defined(TARGET_DARWIN)
  CSettingString* timezonecountry = (CSettingString*)m_settingsManager->GetSetting("locale.timezonecountry");
  CSettingString* timezone = (CSettingString*)m_settingsManager->GetSetting("locale.timezone");

  if (!g_sysinfo.IsAppleTV2() || GetIOSVersion() >= 4.3)
  {
    timezonecountry->SetRequirementsMet(false);
    timezone->SetRequirementsMet(false);
  }
#endif
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(HAS_SKIN_TOUCHED) && defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
  ((CSettingAddon*)m_settingsManager->GetSetting("lookandfeel.skin"))->SetDefault("skin.touched");
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
  m_settingsManager->RegisterSettingOptionsFiller("audiocdencoders", MEDIA_DETECT::CAutorun::SettingOptionAudioCdEncodersFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("aequalitylevels", CAEFactory::SettingOptionsAudioQualityLevelsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiodevices", CAEFactory::SettingOptionsAudioDevicesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiodevicespassthrough", CAEFactory::SettingOptionsAudioDevicesPassthroughFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiostreamsilence", CAEFactory::SettingOptionsAudioStreamsilenceFiller);
  m_settingsManager->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("epgguideviews", PVR::CGUIWindowPVRGuide::SettingOptionsEpgGuideViewFiller);
  m_settingsManager->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("languages", CLangInfo::SettingOptionsLanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("pvrstartlastchannel", PVR::CPVRManager::SettingOptionsPvrStartLastChannelFiller);
  m_settingsManager->RegisterSettingOptionsFiller("refreshchangedelays", CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller);
  m_settingsManager->RegisterSettingOptionsFiller("refreshrates", CDisplaySettings::SettingOptionsRefreshRatesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsScreensFiller);
  m_settingsManager->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("shutdownstates", CPowerManager::SettingOptionsShutdownStatesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("streamlanguages", CLangInfo::SettingOptionsStreamLanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinsounds", ADDON::CSkinInfo::SettingOptionsSkinSoundFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
#ifdef TARGET_LINUX
  m_settingsManager->RegisterSettingOptionsFiller("timezonecountries", CLinuxTimezone::SettingOptionsTimezoneCountriesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("timezones", CLinuxTimezone::SettingOptionsTimezonesFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("verticalsyncs", CDisplaySettings::SettingOptionsVerticalSyncsFiller);
}

void CSettings::InitializeConditions()
{
  // add basic conditions
  m_settingsManager->AddCondition("true");
#ifdef HAS_UPNP
  m_settingsManager->AddCondition("has_upnp");
#endif
#ifdef HAS_AIRPLAY
  m_settingsManager->AddCondition("has_airplay");
#endif
#ifdef HAS_EVENT_SERVER
  m_settingsManager->AddCondition("has_event_server");
#endif
#ifdef HAVE_X11
  m_settingsManager->AddCondition("have_x11");
#endif
#ifdef HAS_GL
  m_settingsManager->AddCondition("has_gl");
#endif
#ifdef HAS_GLES
  m_settingsManager->AddCondition("has_gles");
#endif
#if HAS_GLES == 2
  m_settingsManager->AddCondition("has_glesv2");
#endif
#ifdef HAS_KARAOKE
  m_settingsManager->AddCondition("has_karaoke");
#endif
#ifdef HAS_SDL_JOYSTICK
  m_settingsManager->AddCondition("has_sdl_joystick");
#endif
#ifdef HAS_SKIN_TOUCHED
  m_settingsManager->AddCondition("has_skin_touched");
#endif
#ifdef HAS_TIME_SERVER
  m_settingsManager->AddCondition("has_time_server");
#endif
#ifdef HAS_WEB_SERVER
  m_settingsManager->AddCondition("has_web_server");
#endif
#ifdef HAS_ZEROCONF
  m_settingsManager->AddCondition("has_zeroconf");
#endif
#ifdef HAVE_LIBCRYSTALHD
  m_settingsManager->AddCondition("have_libcrystalhd");
  if (CCrystalHD::GetInstance()->DevicePresent())
    m_settingsManager->AddCondition("hascrystalhddevice");
#endif
#ifdef HAVE_LIBOPENMAX
  m_settingsManager->AddCondition("have_libopenmax");
#endif
#ifdef HAVE_LIBVA
  m_settingsManager->AddCondition("have_libva");
#endif
#ifdef HAVE_LIBVDPAU
  m_settingsManager->AddCondition("have_libvdpau");
#endif
#ifdef TARGET_ANDROID
  if (CAndroidFeatures::GetVersion() > 15)
    m_settingsManager->AddCondition("has_mediacodec");
#endif
#ifdef HAS_LIBSTAGEFRIGHT
  m_settingsManager->AddCondition("have_libstagefrightdecoder");
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
  m_settingsManager->AddCondition("have_videotoolboxdecoder");
  if (g_sysinfo.HasVideoToolBoxDecoder())
    m_settingsManager->AddCondition("hasvideotoolboxdecoder");
#endif
#ifdef HAS_LIBAMCODEC
  if (aml_present())
    m_settingsManager->AddCondition("have_amcodec");
#endif
#ifdef TARGET_DARWIN_IOS_ATV2
  if (g_sysinfo.IsAppleTV2())
    m_settingsManager->AddCondition("isappletv2");
#endif
#ifdef TARGET_DARWIN_OSX
  if (DarwinIsSnowLeopard())
    m_settingsManager->AddCondition("osxissnowleopard");
#endif
#if defined(TARGET_WINDOWS) && defined(HAS_DX)
  m_settingsManager->AddCondition("has_dx");
  m_settingsManager->AddCondition("hasdxva2");
#endif

  if (g_application.IsStandAlone())
    m_settingsManager->AddCondition("isstandalone");

  if(CAEFactory::SupportsQualitySetting())
    m_settingsManager->AddCondition("has_ae_quality_levels");

  // add more complex conditions
  m_settingsManager->AddCondition("addonhassettings", AddonHasSettings);
  m_settingsManager->AddCondition("checkmasterlock", CheckMasterLock);
  m_settingsManager->AddCondition("checkpvrparentalpin", CheckPVRParentalPin);
  m_settingsManager->AddCondition("hasperipherals", HasPeripherals);
  m_settingsManager->AddCondition("isfullscreen", IsFullscreen);
  m_settingsManager->AddCondition("ismasteruser", IsMasterUser);
  m_settingsManager->AddCondition("isusingttfsubtitles", IsUsingTTFSubtitles);
  m_settingsManager->AddCondition("profilecanwritedatabase", ProfileCanWriteDatabase);
  m_settingsManager->AddCondition("profilecanwritesources", ProfileCanWriteSources);
  m_settingsManager->AddCondition("profilehasaddons", ProfileHasAddons);
  m_settingsManager->AddCondition("profilehasdatabase", ProfileHasDatabase);
  m_settingsManager->AddCondition("profilehassources", ProfileHasSources);
  m_settingsManager->AddCondition("profilehasaddonmanagerlocked", ProfileHasAddonManagerLocked);
  m_settingsManager->AddCondition("profilehasfileslocked", ProfileHasFilesLocked);
  m_settingsManager->AddCondition("profilehasmusiclocked", ProfileHasMusicLocked);
  m_settingsManager->AddCondition("profilehaspictureslocked", ProfileHasPicturesLocked);
  m_settingsManager->AddCondition("profilehasprogramslocked", ProfileHasProgramsLocked);
  m_settingsManager->AddCondition("profilehassettingslocked", ProfileHasSettingsLocked);
  m_settingsManager->AddCondition("profilehasvideoslocked", ProfileHasVideosLocked);
  m_settingsManager->AddCondition("profilelockmode", ProfileLockMode);
  m_settingsManager->AddCondition("aesettingvisible", CAEFactory::IsSettingVisible);
  m_settingsManager->AddCondition("codecoptionvisible", CDVDVideoCodec::IsSettingVisible);
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
  m_settingsManager->RegisterCallback(&CDisplaySettings::Get(), settingSet);

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
  settingSet.insert("audiooutput.normalizelevels");
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
  m_settingsManager->RegisterCallback(&g_langInfo, settingSet);

#if defined(HAS_SDL_JOYSTICK)
  settingSet.clear();
  settingSet.insert("input.enablejoystick");
  m_settingsManager->RegisterCallback(&g_Joystick, settingSet);
#endif

  settingSet.clear();
  settingSet.insert("input.enablemouse");
  m_settingsManager->RegisterCallback(&g_Mouse, settingSet);

#if defined(HAS_GL) && defined(HAVE_X11)
  settingSet.clear();
  settingSet.insert("input.enablesystemkeys");
  m_settingsManager->RegisterCallback(&g_Windowing, settingSet);
#endif

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
