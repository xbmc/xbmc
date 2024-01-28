/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingConditions.h"

#include "LockType.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "application/AppParams.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAESettings.h"
#include "ServiceBroker.h"
#include "GUIPassword.h"
#if defined(HAS_WEB_SERVER)
#include "network/WebServer.h"
#endif
#include "peripherals/Peripherals.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingAddon.h"
#include "settings/SettingsComponent.h"
#include "utils/FontUtils.h"
#include "utils/StringUtils.h"
#include "windowing/WinSystem.h"

namespace
{
bool AddonHasSettings(const std::string& condition,
                      const std::string& value,
                      const SettingConstPtr& setting,
                      void* data)
{
  if (setting == NULL)
    return false;

  std::shared_ptr<const CSettingAddon> settingAddon = std::dynamic_pointer_cast<const CSettingAddon>(setting);
  if (settingAddon == NULL)
    return false;

  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(settingAddon->GetValue(), addon,
                                              settingAddon->GetAddonType(),
                                              ADDON::OnlyEnabled::CHOICE_YES) ||
      addon == NULL)
    return false;

  if (addon->Type() == ADDON::AddonType::SKIN)
    return ((ADDON::CSkinInfo*)addon.get())->HasSkinFile("SkinSettings.xml");

  return addon->CanHaveAddonOrInstanceSettings();
}

bool CheckMasterLock(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  return g_passwordManager.IsMasterLockUnlocked(StringUtils::EqualsNoCase(value, "true"));
}

bool HasPeripherals(const std::string& condition,
                    const std::string& value,
                    const SettingConstPtr& setting,
                    void* data)
{
  return CServiceBroker::GetPeripherals().GetNumberOfPeripherals() > 0;
}

bool HasPeripheralLibraries(const std::string& condition,
                            const std::string& value,
                            const SettingConstPtr& setting,
                            void* data)
{
  return CServiceBroker::GetAddonMgr().HasInstalledAddons(ADDON::AddonType::PERIPHERALDLL);
}

bool HasRumbleFeature(const std::string& condition,
                      const std::string& value,
                      const SettingConstPtr& setting,
                      void* data)
{
  return CServiceBroker::GetPeripherals().SupportsFeature(PERIPHERALS::FEATURE_RUMBLE);
}

bool HasRumbleController(const std::string& condition,
                         const std::string& value,
                         const SettingConstPtr& setting,
                         void* data)
{
  return CServiceBroker::GetPeripherals().HasPeripheralWithFeature(PERIPHERALS::FEATURE_RUMBLE);
}

bool HasPowerOffFeature(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  return CServiceBroker::GetPeripherals().SupportsFeature(PERIPHERALS::FEATURE_POWER_OFF);
}

bool HasSystemSdrPeakLuminance(const std::string& condition,
                               const std::string& value,
                               const SettingConstPtr& setting,
                               void* data)
{
  return CServiceBroker::GetWinSystem()->HasSystemSdrPeakLuminance();
}

bool SupportsVideoSuperResolution(const std::string& condition,
                                  const std::string& value,
                                  const SettingConstPtr& setting,
                                  void* data)
{
  return CServiceBroker::GetWinSystem()->SupportsVideoSuperResolution();
}

bool SupportsDolbyVision(const std::string& condition,
                         const std::string& value,
                         const SettingConstPtr& setting,
                         void* data)
{
  return CServiceBroker::GetWinSystem()->GetDisplayHDRCapabilities().SupportsDolbyVision();
}

bool SupportsScreenMove(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  return CServiceBroker::GetWinSystem()->SupportsScreenMove();
}

bool IsHDRDisplay(const std::string& condition,
                  const std::string& value,
                  const SettingConstPtr& setting,
                  void* data)
{
  return CServiceBroker::GetWinSystem()->IsHDRDisplay();
}

bool IsMasterUser(const std::string& condition,
                  const std::string& value,
                  const SettingConstPtr& setting,
                  void* data)
{
  return g_passwordManager.bMasterUser;
}

bool HasSubtitlesFontExtensions(const std::string& condition,
                                const std::string& value,
                                const SettingConstPtr& setting,
                                void* data)
{
  auto settingStr = std::dynamic_pointer_cast<const CSettingString>(setting);
  if (!settingStr)
    return false;

  return UTILS::FONT::IsSupportedFontExtension(settingStr->GetValue());
}

bool ProfileCanWriteDatabase(const std::string& condition,
                             const std::string& value,
                             const SettingConstPtr& setting,
                             void* data)
{
  return CSettingConditions::GetCurrentProfile().canWriteDatabases();
}

bool ProfileCanWriteSources(const std::string& condition,
                            const std::string& value,
                            const SettingConstPtr& setting,
                            void* data)
{
  return CSettingConditions::GetCurrentProfile().canWriteSources();
}

bool ProfileHasAddons(const std::string& condition,
                      const std::string& value,
                      const SettingConstPtr& setting,
                      void* data)
{
  return CSettingConditions::GetCurrentProfile().hasAddons();
}

bool ProfileHasDatabase(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  return CSettingConditions::GetCurrentProfile().hasDatabases();
}

bool ProfileHasSources(const std::string& condition,
                       const std::string& value,
                       const SettingConstPtr& setting,
                       void* data)
{
  return CSettingConditions::GetCurrentProfile().hasSources();
}

bool ProfileHasAddonManagerLocked(const std::string& condition,
                                  const std::string& value,
                                  const SettingConstPtr& setting,
                                  void* data)
{
  return CSettingConditions::GetCurrentProfile().addonmanagerLocked();
}

bool ProfileHasFilesLocked(const std::string& condition,
                           const std::string& value,
                           const SettingConstPtr& setting,
                           void* data)
{
  return CSettingConditions::GetCurrentProfile().filesLocked();
}

bool ProfileHasMusicLocked(const std::string& condition,
                           const std::string& value,
                           const SettingConstPtr& setting,
                           void* data)
{
  return CSettingConditions::GetCurrentProfile().musicLocked();
}

bool ProfileHasPicturesLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
{
  return CSettingConditions::GetCurrentProfile().picturesLocked();
}

bool ProfileHasProgramsLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
{
  return CSettingConditions::GetCurrentProfile().programsLocked();
}

bool ProfileHasSettingsLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
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
  return slValue <= CSettingConditions::GetCurrentProfile().settingsLockLevel();
}

bool ProfileHasVideosLocked(const std::string& condition,
                            const std::string& value,
                            const SettingConstPtr& setting,
                            void* data)
{
  return CSettingConditions::GetCurrentProfile().videoLocked();
}

bool ProfileLockMode(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  char* tmp = nullptr;
  LockType lock = (LockType)strtol(value.c_str(), &tmp, 0);
  if (tmp != NULL && *tmp != '\0')
    return false;

  return CSettingConditions::GetCurrentProfile().getLockMode() == lock;
}

bool GreaterThan(const std::string& condition,
                 const std::string& value,
                 const SettingConstPtr& setting,
                 void* data)
{
  if (setting == NULL)
    return false;

  std::shared_ptr<const CSettingInt> settingInt = std::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = nullptr;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs > rhs;
}

bool GreaterThanOrEqual(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  if (setting == NULL)
    return false;

  std::shared_ptr<const CSettingInt> settingInt = std::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = nullptr;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs >= rhs;
}

bool LessThan(const std::string& condition,
              const std::string& value,
              const SettingConstPtr& setting,
              void* data)
{
  if (setting == NULL)
    return false;

  std::shared_ptr<const CSettingInt> settingInt = std::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = nullptr;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs < rhs;
}

bool LessThanOrEqual(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  if (setting == NULL)
    return false;

  std::shared_ptr<const CSettingInt> settingInt = std::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = nullptr;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs <= rhs;
}
}; // anonymous namespace

const CProfileManager* CSettingConditions::m_profileManager = nullptr;
std::set<std::string> CSettingConditions::m_simpleConditions;
std::map<std::string, SettingConditionCheck> CSettingConditions::m_complexConditions;

void CSettingConditions::Initialize()
{
  if (!m_simpleConditions.empty())
    return;

  // add simple conditions
  m_simpleConditions.emplace("true");
#ifdef HAS_UPNP
  m_simpleConditions.emplace("has_upnp");
#endif
#ifdef HAS_AIRPLAY
  m_simpleConditions.emplace("has_airplay");
#endif
#ifdef HAVE_X11
  m_simpleConditions.emplace("have_x11");
#endif
#ifdef HAVE_WAYLAND
  m_simpleConditions.emplace("have_wayland");
#endif
#ifdef HAS_GL
  m_simpleConditions.emplace("has_gl");
#endif
#ifdef HAS_GLES
  m_simpleConditions.emplace("has_gles");
#endif
#if HAS_GLES >= 2
  m_simpleConditions.emplace("has_glesv2");
#endif
#ifdef HAS_TIME_SERVER
  m_simpleConditions.emplace("has_time_server");
#endif
#ifdef HAS_WEB_SERVER
  m_simpleConditions.emplace("has_web_server");
#endif
#ifdef HAS_FILESYSTEM_SMB
  m_simpleConditions.emplace("has_filesystem_smb");
#endif
#ifdef HAS_FILESYSTEM_NFS
  m_simpleConditions.insert("has_filesystem_nfs");
#endif
#ifdef HAS_ZEROCONF
  m_simpleConditions.emplace("has_zeroconf");
#endif
#ifdef HAVE_LIBVA
  m_simpleConditions.emplace("have_libva");
#endif
#ifdef HAVE_LIBVDPAU
  m_simpleConditions.emplace("have_libvdpau");
#endif
#ifdef TARGET_ANDROID
  m_simpleConditions.emplace("has_mediacodec");
#endif
#ifdef TARGET_DARWIN
  m_simpleConditions.emplace("HasVTB");
#endif
#ifdef TARGET_DARWIN_OSX
  m_simpleConditions.emplace("have_osx");
#endif
#ifdef TARGET_DARWIN_IOS
  m_simpleConditions.emplace("have_ios");
#endif
#ifdef TARGET_DARWIN_TVOS
  m_simpleConditions.emplace("have_tvos");
#endif
#if defined(TARGET_WINDOWS)
  m_simpleConditions.emplace("has_dx");
  m_simpleConditions.emplace("hasdxva2");
#endif
#if defined(TARGET_WEBOS)
  m_simpleConditions.emplace("have_webos");
#endif

#ifdef HAVE_LCMS2
  m_simpleConditions.emplace("have_lcms2");
#endif

#ifdef TARGET_ANDROID
  m_simpleConditions.emplace("isstandalone");
#else
  if (CServiceBroker::GetAppParams()->IsStandAlone())
    m_simpleConditions.emplace("isstandalone");
#endif

  m_simpleConditions.emplace("has_ae_quality_levels");

#ifdef HAS_WEB_SERVER
  if (CWebServer::WebServerSupportsSSL())
    m_simpleConditions.emplace("webserver_has_ssl");
#endif

#ifdef HAVE_LIBBLURAY
  m_simpleConditions.emplace("have_libbluray");
#endif

#ifdef HAS_CDDA_RIPPER
  m_simpleConditions.emplace("has_cdda_ripper");
#endif

#ifdef HAS_OPTICAL_DRIVE
  m_simpleConditions.emplace("has_optical_drive");
#endif

#ifdef HAS_XBMCHELPER
  m_simpleConditions.emplace("has_xbmchelper");
#endif

  // add complex conditions
  m_complexConditions.emplace("addonhassettings", AddonHasSettings);
  m_complexConditions.emplace("checkmasterlock", CheckMasterLock);
  m_complexConditions.emplace("hasperipherals", HasPeripherals);
  m_complexConditions.emplace("hasperipherallibraries", HasPeripheralLibraries);
  m_complexConditions.emplace("hasrumblefeature", HasRumbleFeature);
  m_complexConditions.emplace("hasrumblecontroller", HasRumbleController);
  m_complexConditions.emplace("haspowerofffeature", HasPowerOffFeature);
  m_complexConditions.emplace("hassystemsdrpeakluminance", HasSystemSdrPeakLuminance);
  m_complexConditions.emplace("supportsscreenmove", SupportsScreenMove);
  m_complexConditions.emplace("supportsvideosuperresolution", SupportsVideoSuperResolution);
  m_complexConditions.emplace("supportsdolbyvision", SupportsDolbyVision);
  m_complexConditions.emplace("ishdrdisplay", IsHDRDisplay);
  m_complexConditions.emplace("ismasteruser", IsMasterUser);
  m_complexConditions.emplace("hassubtitlesfontextensions", HasSubtitlesFontExtensions);
  m_complexConditions.emplace("profilecanwritedatabase", ProfileCanWriteDatabase);
  m_complexConditions.emplace("profilecanwritesources", ProfileCanWriteSources);
  m_complexConditions.emplace("profilehasaddons", ProfileHasAddons);
  m_complexConditions.emplace("profilehasdatabase", ProfileHasDatabase);
  m_complexConditions.emplace("profilehassources", ProfileHasSources);
  m_complexConditions.emplace("profilehasaddonmanagerlocked", ProfileHasAddonManagerLocked);
  m_complexConditions.emplace("profilehasfileslocked", ProfileHasFilesLocked);
  m_complexConditions.emplace("profilehasmusiclocked", ProfileHasMusicLocked);
  m_complexConditions.emplace("profilehaspictureslocked", ProfileHasPicturesLocked);
  m_complexConditions.emplace("profilehasprogramslocked", ProfileHasProgramsLocked);
  m_complexConditions.emplace("profilehassettingslocked", ProfileHasSettingsLocked);
  m_complexConditions.emplace("profilehasvideoslocked", ProfileHasVideosLocked);
  m_complexConditions.emplace("profilelockmode", ProfileLockMode);
  m_complexConditions.emplace("aesettingvisible", ActiveAE::CActiveAESettings::IsSettingVisible);
  m_complexConditions.emplace("gt", GreaterThan);
  m_complexConditions.emplace("gte", GreaterThanOrEqual);
  m_complexConditions.emplace("lt", LessThan);
  m_complexConditions.emplace("lte", LessThanOrEqual);
}

void CSettingConditions::Deinitialize()
{
  m_profileManager = nullptr;
}

const CProfile& CSettingConditions::GetCurrentProfile()
{
  if (!m_profileManager)
    m_profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager().get();

  if (m_profileManager)
    return m_profileManager->GetCurrentProfile();

  static CProfile emptyProfile;
  return emptyProfile;
}

bool CSettingConditions::Check(const std::string& condition,
                               const std::string& value /* = "" */,
                               const SettingConstPtr& setting /* = NULL */)
{
  if (m_simpleConditions.find(condition) != m_simpleConditions.end())
    return true;

  std::map<std::string, SettingConditionCheck>::const_iterator itCondition = m_complexConditions.find(condition);
  if (itCondition != m_complexConditions.end())
    return itCondition->second(condition, value, setting, NULL);

  return Check(condition);
}
