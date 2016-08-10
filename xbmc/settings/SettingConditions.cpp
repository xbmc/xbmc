/*
 *      Copyright (C) 2013 Team XBMC
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
#include "SettingConditions.h"
#include "Application.h"
#include "GUIPassword.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#if defined(TARGET_ANDROID)
#include "platform/android/activity/AndroidFeatures.h"
#endif // defined(TARGET_ANDROID)
#include "cores/AudioEngine/AEFactory.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "guilib/LocalizeStrings.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "profiles/ProfilesManager.h"
#include "pvr/PVRManager.h"
#include "settings/SettingAddon.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif // defined(HAS_LIBAMCODEC)
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "windowing/WindowingFactory.h"
#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/DarwinUtils.h"
#endif// defined(TARGET_DARWIN_OSX)

bool AddonHasSettings(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  const CSettingAddon *settingAddon = dynamic_cast<const CSettingAddon*>(setting);
  if (settingAddon == NULL)
    return false;

  ADDON::AddonPtr addon;
  if (!ADDON::CAddonMgr::GetInstance().GetAddon(settingAddon->GetValue(), addon, settingAddon->GetAddonType()) || addon == NULL)
    return false;

  if (addon->Type() == ADDON::ADDON_SKIN)
    return ((ADDON::CSkinInfo*)addon.get())->HasSkinFile("SkinSettings.xml");

  return addon->HasSettings();
}

bool CheckMasterLock(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return g_passwordManager.IsMasterLockUnlocked(StringUtils::EqualsNoCase(value, "true"));
}

bool CheckPVRParentalPin(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return PVR::g_PVRManager.CheckParentalPIN(g_localizeStrings.Get(19262).c_str());
}

bool HasPeripherals(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return PERIPHERALS::g_peripherals.GetNumberOfPeripherals() > 0;
}

bool SupportsPeripheralControllers(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  using namespace PERIPHERALS;

  PeripheralBusAddonPtr bus = std::static_pointer_cast<CPeripheralBusAddon>(g_peripherals.GetBusByType(PERIPHERAL_BUS_ADDON));
  return bus != nullptr && bus->HasFeature(FEATURE_JOYSTICK);
}

bool HasRumbleFeature(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  using namespace PERIPHERALS;

  std::vector<CPeripheral*> results;
  g_peripherals.GetPeripheralsWithFeature(results, FEATURE_RUMBLE);
  return !results.empty();
}

bool IsFullscreen(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return g_Windowing.IsFullScreen();
}

bool IsMasterUser(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return g_passwordManager.bMasterUser;
}

bool IsUsingTTFSubtitles(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CUtil::IsUsingTTFSubtitles();
}

bool ProfileCanWriteDatabase(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().canWriteDatabases();
}

bool ProfileCanWriteSources(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources();
}

bool ProfileHasAddons(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().hasAddons();
}

bool ProfileHasDatabase(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().hasDatabases();
}

bool ProfileHasSources(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().hasSources();
}

bool ProfileHasAddonManagerLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().addonmanagerLocked();
}

bool ProfileHasFilesLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().filesLocked();
}

bool ProfileHasMusicLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().musicLocked();
}

bool ProfileHasPicturesLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().picturesLocked();
}

bool ProfileHasProgramsLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().programsLocked();
}

bool ProfileHasSettingsLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
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
  return slValue <= CProfilesManager::GetInstance().GetCurrentProfile().settingsLockLevel();
}

bool ProfileHasVideosLocked(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  return CProfilesManager::GetInstance().GetCurrentProfile().videoLocked();
}

bool ProfileLockMode(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  char *tmp = NULL;
  LockType lock = (LockType)strtol(value.c_str(), &tmp, 0);
  if (tmp != NULL && *tmp != '\0')
    return false;

  return CProfilesManager::GetInstance().GetCurrentProfile().getLockMode() == lock;
}

bool GreaterThan(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  const CSettingInt *settingInt = dynamic_cast<const CSettingInt*>(setting);
  if (settingInt == NULL)
    return false;

  char *tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs > rhs;
}

bool GreaterThanOrEqual(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  const CSettingInt *settingInt = dynamic_cast<const CSettingInt*>(setting);
  if (settingInt == NULL)
    return false;

  char *tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs >= rhs;
}

bool LessThan(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  const CSettingInt *settingInt = dynamic_cast<const CSettingInt*>(setting);
  if (settingInt == NULL)
    return false;

  char *tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs < rhs;
}

bool LessThanOrEqual(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  const CSettingInt *settingInt = dynamic_cast<const CSettingInt*>(setting);
  if (settingInt == NULL)
    return false;

  char *tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs <= rhs;
}

std::set<std::string> CSettingConditions::m_simpleConditions;
std::map<std::string, SettingConditionCheck> CSettingConditions::m_complexConditions;

void CSettingConditions::Initialize()
{
  if (!m_simpleConditions.empty())
    return;

  // add simple conditions
  m_simpleConditions.insert("true");
#ifdef HAS_UPNP
  m_simpleConditions.insert("has_upnp");
#endif
#ifdef HAS_AIRPLAY
  m_simpleConditions.insert("has_airplay");
#endif
#ifdef HAS_EVENT_SERVER
  m_simpleConditions.insert("has_event_server");
#endif
#ifdef HAVE_X11
  m_simpleConditions.insert("have_x11");
#endif
#ifdef HAS_GL
  m_simpleConditions.insert("has_gl");
#endif
#ifdef HAS_GLX
  m_simpleConditions.insert("has_glx");
#endif
#ifdef HAS_GLES
  m_simpleConditions.insert("has_gles");
#endif
#if HAS_GLES == 2
  m_simpleConditions.insert("has_glesv2");
#endif
#ifdef HAS_TIME_SERVER
  m_simpleConditions.insert("has_time_server");
#endif
#ifdef HAS_WEB_SERVER
  m_simpleConditions.insert("has_web_server");
#endif
#ifdef HAS_FILESYSTEM_SMB
  m_simpleConditions.insert("has_filesystem_smb");
#endif
#ifdef HAS_ZEROCONF
  m_simpleConditions.insert("has_zeroconf");
#endif
#ifdef HAVE_LIBOPENMAX
  m_simpleConditions.insert("have_libopenmax");
#endif
#ifdef HAS_OMXPLAYER
  m_simpleConditions.insert("has_omxplayer");
#endif
#ifdef HAVE_LIBVA
  m_simpleConditions.insert("have_libva");
#endif
#ifdef HAVE_LIBVDPAU
  m_simpleConditions.insert("have_libvdpau");
#endif
#ifdef TARGET_ANDROID
  m_simpleConditions.insert("has_mediacodec");
#endif
#ifdef TARGET_DARWIN
  m_simpleConditions.insert("HasVTB");
#endif
#ifdef HAS_LIBAMCODEC
  if (aml_present())
    m_simpleConditions.insert("have_amcodec");
#endif
#ifdef TARGET_DARWIN_OSX
  if (CDarwinUtils::IsSnowLeopard())
    m_simpleConditions.insert("osxissnowleopard");
#endif
#if defined(TARGET_WINDOWS) && defined(HAS_DX)
  m_simpleConditions.insert("has_dx");
  m_simpleConditions.insert("hasdxva2");
#endif
#ifdef HAVE_LCMS2
  m_simpleConditions.insert("have_lcms2");
#endif

  if (g_application.IsStandAlone())
    m_simpleConditions.insert("isstandalone");

  if(CAEFactory::SupportsQualitySetting())
    m_simpleConditions.insert("has_ae_quality_levels");

  // add complex conditions
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("addonhassettings",              AddonHasSettings));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("checkmasterlock",               CheckMasterLock));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("checkpvrparentalpin",           CheckPVRParentalPin));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("hasperipherals",                HasPeripherals));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("supportsperipheralcontrollers", SupportsPeripheralControllers));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("hasrumblefeature",              HasRumbleFeature));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("isfullscreen",                  IsFullscreen));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("ismasteruser",                  IsMasterUser));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("isusingttfsubtitles",           IsUsingTTFSubtitles));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilecanwritedatabase",       ProfileCanWriteDatabase));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilecanwritesources",        ProfileCanWriteSources));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasaddons",              ProfileHasAddons));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasdatabase",            ProfileHasDatabase));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehassources",             ProfileHasSources));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasaddonmanagerlocked",  ProfileHasAddonManagerLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasfileslocked",         ProfileHasFilesLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasmusiclocked",         ProfileHasMusicLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehaspictureslocked",      ProfileHasPicturesLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasprogramslocked",      ProfileHasProgramsLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehassettingslocked",      ProfileHasSettingsLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasvideoslocked",        ProfileHasVideosLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilelockmode",               ProfileLockMode));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("aesettingvisible",              CAEFactory::IsSettingVisible));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("codecoptionvisible",            CDVDVideoCodec::IsSettingVisible));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("gt",                            GreaterThan));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("gte",                           GreaterThanOrEqual));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("lt",                            LessThan));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("lte",                           LessThanOrEqual));
}

bool CSettingConditions::Check(const std::string &condition, const std::string &value /* = "" */, const CSetting *setting /* = NULL */)
{
  if (m_simpleConditions.find(condition) != m_simpleConditions.end())
    return true;

  std::map<std::string, SettingConditionCheck>::const_iterator itCondition = m_complexConditions.find(condition);
  if (itCondition != m_complexConditions.end())
    return itCondition->second(condition, value, setting, NULL);

  return Check(condition);
}
