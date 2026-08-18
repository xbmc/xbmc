/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralCecAdapter.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationEnums.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/Key.h"
#include "input/keymaps/remote/IRRemoteIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "jobs/JobManager.h"
#include "messaging/ApplicationMessenger.h"
#include "peripherals/Peripherals.h"
#include "pictures/GUIWindowSlideShow.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <mutex>

#include <libcec/cec.h>

using namespace PERIPHERALS;
using namespace CEC;
using namespace ANNOUNCEMENT;
using namespace std::chrono_literals;

#define CEC_LIB_SUPPORTED_VERSION LIBCEC_VERSION_TO_UINT(4, 0, 0)

/* SystemAudioModeStatus() was added in libCEC 7.1.0 */
#define CEC_LIB_HAS_SYSTEM_AUDIO_MODE_STATUS \
  (CEC_LIB_VERSION_MAJOR > 7 || (CEC_LIB_VERSION_MAJOR == 7 && CEC_LIB_VERSION_MINOR >= 1))

/* time in seconds to ignore standby commands from devices after the screensaver has been activated
 */
#define SCREENSAVER_TIMEOUT 20
#define VOLUME_CHANGE_TIMEOUT 250
#define VOLUME_REFRESH_TIMEOUT 100

#define LOCALISED_ID_TV 36037
#define LOCALISED_ID_AVR 36038
#define LOCALISED_ID_TV_AVR 36039
#define LOCALISED_ID_STOP 36044
#define LOCALISED_ID_PAUSE 36045
#define LOCALISED_ID_POWEROFF 13005
#define LOCALISED_ID_SUSPEND 13011
#define LOCALISED_ID_HIBERNATE 13010
#define LOCALISED_ID_QUIT 13009
#define LOCALISED_ID_IGNORE 36028
#define LOCALISED_ID_RECORDING_DEVICE 36051
#define LOCALISED_ID_PLAYBACK_DEVICE 36052
#define LOCALISED_ID_TUNER_DEVICE 36053
#define LOCALISED_ID_ALWAYS 36055
#define LOCALISED_ID_UNLESS_PLAYING 36056
#define LOCALISED_ID_VOLUME_ALWAYS 20422

#define LOCALISED_ID_NONE 231

/* time in seconds to suppress source activation after receiving OnStop */
#define CEC_SUPPRESS_ACTIVATE_SOURCE_AFTER_ON_STOP 2

#if CEC_LIB_VERSION_MAJOR < 8
/* delay in ms before a held button starts repeating. libCEC gained a field of its own for
   this in 8.0.0; older versions take it from iDoubleTapTimeoutMs */
#define CEC_BUTTON_REPEAT_DELAY_MS 300
#endif

CPeripheralCecAdapter::CPeripheralCecAdapter(CPeripherals& manager,
                                             const PeripheralScanResult& scanResult,
                                             CPeripheralBus* bus)
  : CPeripheralHID(manager, scanResult, bus),
    CThread("CECAdapter")
{
  ResetMembers();
  m_features.push_back(FEATURE_CEC);
  m_strComPort = scanResult.m_strLocation;
}

CPeripheralCecAdapter::~CPeripheralCecAdapter(void)
{
  m_manager.GetInputManager().UnregisterCecInputProvider(this);

  {
    std::unique_lock lock(m_critSection);
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    m_bStop = true;
  }

  StopThread(true);
  delete m_queryThread;

  if (m_cecAdapter)
  {
    CECDestroy(m_cecAdapter);
    m_cecAdapter = nullptr;
  }
}

void CPeripheralCecAdapter::ResetMembers(void)
{
  if (m_cecAdapter)
    CECDestroy(m_cecAdapter);
  m_cecAdapter = nullptr;
  m_bStarted = false;
  m_bHasButton = false;
  m_bIsReady = false;
  m_volumeTarget = CECDEVICE_UNKNOWN;
  m_tvVolumeTarget = CECDEVICE_UNKNOWN;
  m_strMenuLanguage = "???";
  m_lastKeypress = {};
  m_lastChange = VOLUME_CHANGE_NONE;
  m_iExitCode = EXITCODE_QUIT;

  /* replaced by the amp's own mute state once it reports one */
  m_bIsMuted = false;

  m_bGoingToStandby = false;
  m_bIsRunning = false;
  m_bDeviceRemoved = false;
  m_bActiveSourcePending = false;
  m_bStandbyPending = false;
  m_bActiveSourceBeforeStandby = false;
  m_bOnPlayReceived = false;
  m_bPlaybackPaused = false;
  m_queryThread = nullptr;
  m_bPowerOnScreensaver = false;
  m_bUseTVMenuLanguage = false;
  m_bSendInactiveSource = false;
  m_bShutdownOnStandby = false;

  m_currentButton.iButton = 0;
  m_currentButton.iDuration = 0;
  m_standbySent.SetValid(false);
  m_configuration.Clear();
}

void CPeripheralCecAdapter::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                     const std::string& sender,
                                     const std::string& message,
                                     const CVariant& data)
{
  if (flag == ANNOUNCEMENT::System && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
      message == "OnQuit" && m_bIsReady)
  {
    std::unique_lock lock(m_critSection);
    m_iExitCode = static_cast<int>(data["exitcode"].asInteger(EXITCODE_QUIT));
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    StopThread(false);
  }
  else if (flag == ANNOUNCEMENT::GUI && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           message == "OnScreensaverDeactivated" && m_bIsReady)
  {
    bool bIgnoreDeactivate(false);
    if (data["shuttingdown"].isBoolean())
    {
      // don't respond to the deactivation if we are just going to suspend/shutdown anyway
      // the tv will not have time to switch on before being told to standby and
      // may not action the standby command.
      bIgnoreDeactivate = data["shuttingdown"].asBoolean();
      if (bIgnoreDeactivate)
        CLog::Log(LOGDEBUG, "{} - ignoring OnScreensaverDeactivated for power action",
                  __FUNCTION__);
    }
    if (m_bPowerOnScreensaver && !bIgnoreDeactivate && m_configuration.bActivateSource)
    {
      ActivateSource();
    }
  }
  else if (flag == ANNOUNCEMENT::GUI && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           message == "OnScreensaverActivated" && m_bIsReady)
  {
    const int iStandbyMode = GetSettingInt("cec_standby_screensaver_mode");
    if (iStandbyMode != LOCALISED_ID_NONE)
    {
      // the screensaver doesn't activate while a video is playing, so this mainly keeps the
      // devices on for audio playback over HDMI. paused playback doesn't count as playing.
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      const bool bPlaying = (appPlayer->IsPlayingVideo() || appPlayer->IsPlayingAudio()) &&
                            !appPlayer->IsPausedPlayback();

      // only power off when we're the active source
      if ((iStandbyMode == LOCALISED_ID_ALWAYS || !bPlaying) &&
          m_cecAdapter->IsLibCECActiveSource())
        StandbyDevices();
    }
  }
  else if (flag == ANNOUNCEMENT::System && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           message == "OnSleep")
  {
    // this will also power off devices when we're the active source
    {
      std::unique_lock lock(m_critSection);
      m_bGoingToStandby = true;
    }
    StopThread();
  }
  else if (flag == ANNOUNCEMENT::System && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           message == "OnWake")
  {
    CLog::Log(LOGDEBUG, "{} - reconnecting to the CEC adapter after standby mode", __FUNCTION__);
    if (ReopenConnection())
    {
      bool bActivate(false);
      {
        std::unique_lock lock(m_critSection);
        bActivate = m_bActiveSourceBeforeStandby;
        m_bActiveSourceBeforeStandby = false;
      }
      if (bActivate)
        ActivateSource();
    }
  }
  else if (flag == ANNOUNCEMENT::Player && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           message == "OnStop")
  {
    std::unique_lock lock(m_critSection);
    m_preventActivateSourceOnPlay = CDateTime::GetCurrentDateTime();
    m_bOnPlayReceived = false;
  }
  else if (flag == ANNOUNCEMENT::Player && sender == CAnnouncementManager::ANNOUNCEMENT_SENDER &&
           (message == "OnPlay" || message == "OnResume"))
  {
    // activate the source when playback started, and the option is enabled
    bool bActivateSource(false);
    {
      std::unique_lock lock(m_critSection);
      bActivateSource = (m_configuration.bActivateSource && !m_bOnPlayReceived &&
                         !m_cecAdapter->IsLibCECActiveSource() &&
                         (!m_preventActivateSourceOnPlay.IsValid() ||
                          CDateTime::GetCurrentDateTime() - m_preventActivateSourceOnPlay >
                              CDateTimeSpan(0, 0, 0, CEC_SUPPRESS_ACTIVATE_SOURCE_AFTER_ON_STOP)));
      m_bOnPlayReceived = true;
    }
    if (bActivateSource)
      ActivateSource();
  }
}

bool CPeripheralCecAdapter::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_CEC && !m_bStarted && GetSettingBool("enabled"))
  {
    // hide settings that have an override set
    if (!GetSettingString("wake_devices_advanced").empty())
      SetSettingVisible("wake_devices", false);
    if (!GetSettingString("standby_devices_advanced").empty())
      SetSettingVisible("standby_devices", false);

    SetConfigurationFromSettings();
    m_callbacks.Clear();
    m_callbacks.logMessage = &CecLogMessage;
    m_callbacks.keyPress = &CecKeyPress;
    m_callbacks.commandReceived = &CecCommand;
    m_callbacks.configurationChanged = &CecConfiguration;
    m_callbacks.alert = &CecAlert;
    m_callbacks.sourceActivated = &CecSourceActivated;
    m_configuration.callbackParam = this;
    m_configuration.callbacks = &m_callbacks;

    m_cecAdapter = CECInitialise(&m_configuration);

    if (m_configuration.serverVersion < CEC_LIB_SUPPORTED_VERSION)
    {
      /* unsupported libcec version */
      CLog::Log(
          LOGERROR,
          "Detected version of libCEC interface ({0:x}) is lower than the supported version {1:x}",
          m_cecAdapter ? m_configuration.serverVersion : -1, CEC_LIB_SUPPORTED_VERSION);

      // display warning: incompatible libCEC
      std::string strMessage = StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36040),
          m_cecAdapter ? m_configuration.serverVersion : -1, CEC_LIB_SUPPORTED_VERSION);
      CGUIDialogKaiToast::QueueNotification(
          CGUIDialogKaiToast::Error,
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000), strMessage);
      m_bError = true;
      if (m_cecAdapter)
        CECDestroy(m_cecAdapter);
      m_cecAdapter = nullptr;

      m_features.clear();
      return false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "{} - using libCEC v{}", __FUNCTION__,
                m_cecAdapter->VersionToString(m_configuration.serverVersion));
      SetVersionInfo(m_configuration);
    }

    m_bStarted = true;
    Create();

    m_manager.GetInputManager().RegisterCecInputProvider(this);
  }

  return CPeripheral::InitialiseFeature(feature);
}

std::optional<CKey> CPeripheralCecAdapter::GetCecKey()
{
  const int buttonCode = GetButton();
  const unsigned int holdTime = GetHoldTime();

  if (buttonCode <= 0)
    return {};

  std::optional<CKey> key = CKey{static_cast<uint32_t>(buttonCode), holdTime};

  ResetButton();
  return key;
}

void CPeripheralCecAdapter::SetVersionInfo(const libcec_configuration& configuration)
{
  m_strVersionInfo = StringUtils::Format("libCEC {} - firmware v{}",
                                         m_cecAdapter->VersionToString(configuration.serverVersion),
                                         configuration.iFirmwareVersion);

  // append firmware build date
  if (configuration.iFirmwareBuildDate != CEC_FW_BUILD_UNKNOWN)
  {
    CDateTime dt((time_t)configuration.iFirmwareBuildDate);
    m_strVersionInfo += StringUtils::Format(" ({})", dt.GetAsDBDate());
  }
}

bool CPeripheralCecAdapter::OpenConnection(void)
{
  bool bIsOpen(false);

  if (!GetSettingBool("enabled"))
  {
    CLog::Log(LOGDEBUG, "{} - CEC adapter is disabled in peripheral settings", __FUNCTION__);
    m_bStarted = false;
    return bIsOpen;
  }

  // open the CEC adapter
  CLog::Log(LOGDEBUG, "{} - opening a connection to the CEC adapter: {}", __FUNCTION__,
            m_strComPort);

  // scanning the CEC bus takes about 5 seconds, so display a notification to inform users that
  // we're busy
  std::string strMessage =
      StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21336),
                          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000));
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Info,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000), strMessage);

  bool bConnectionFailedDisplayed(false);

  while (!m_bStop && !bIsOpen)
  {
    if ((bIsOpen = m_cecAdapter->Open(m_strComPort.c_str(), 10000)) == false)
    {
      // display warning: couldn't initialise libCEC
      CLog::Log(LOGERROR, "{} - could not opening a connection to the CEC adapter", __FUNCTION__);
      if (!bConnectionFailedDisplayed)
        CGUIDialogKaiToast::QueueNotification(
            CGUIDialogKaiToast::Error,
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000),
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36012));
      bConnectionFailedDisplayed = true;

      CThread::Sleep(10000ms);
    }
  }

  if (bIsOpen)
  {
    CLog::Log(LOGDEBUG, "{} - connection to the CEC adapter opened", __FUNCTION__);

    // read the configuration
    libcec_configuration config;
    if (m_cecAdapter->GetCurrentConfiguration(&config))
    {
      // update the local configuration
      std::unique_lock lock(m_critSection);
      SetConfigurationFromLibCEC(config);
    }
  }

  return bIsOpen;
}

void CPeripheralCecAdapter::Process(void)
{
  if (!OpenConnection())
    return;

  {
    std::unique_lock lock(m_critSection);
    m_iExitCode = EXITCODE_QUIT;
    m_bGoingToStandby = false;
    m_bIsRunning = true;
    m_bActiveSourceBeforeStandby = false;
  }

  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(
      this, ANNOUNCEMENT::System | ANNOUNCEMENT::GUI | ANNOUNCEMENT::Player);

  m_queryThread = new CPeripheralCecAdapterUpdateThread(this, &m_configuration);
  m_queryThread->Create(false);

  while (!m_bStop)
  {
    if (!m_bStop)
      ProcessVolumeChange();

    if (!m_bStop)
      ProcessActivateSource();

    if (!m_bStop)
      ProcessStandbyDevices();

    if (!m_bStop)
      CThread::Sleep(5ms);
  }

  m_queryThread->StopThread(true);

  bool bSendStandbyCommands(false);
  {
    std::unique_lock lock(m_critSection);
    bSendStandbyCommands = m_iExitCode != EXITCODE_REBOOT && m_iExitCode != EXITCODE_RESTARTAPP &&
                           !m_bDeviceRemoved &&
                           (!m_bGoingToStandby || GetSettingBool("standby_tv_on_pc_standby")) &&
                           GetSettingBool("enabled");

    if (m_bGoingToStandby)
      m_bActiveSourceBeforeStandby = m_cecAdapter->IsLibCECActiveSource();
  }

  if (bSendStandbyCommands)
  {
    if (m_cecAdapter->IsLibCECActiveSource())
    {
      if (!m_configuration.powerOffDevices.IsEmpty())
      {
        CLog::Log(LOGDEBUG, "{} - sending standby commands", __FUNCTION__);
        m_standbySent = CDateTime::GetCurrentDateTime();
        m_cecAdapter->StandbyDevices();
      }
      else if (m_bSendInactiveSource)
      {
        CLog::Log(LOGDEBUG, "{} - sending inactive source commands", __FUNCTION__);
        m_cecAdapter->SetInactiveView();
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "{} - XBMC is not the active source, not sending any standby commands",
                __FUNCTION__);
    }
  }

  m_cecAdapter->Close();

  CLog::Log(LOGDEBUG, "{} - CEC adapter processor thread ended", __FUNCTION__);

  {
    std::unique_lock lock(m_critSection);
    m_bStarted = false;
    m_bIsRunning = false;
  }
}

bool CPeripheralCecAdapter::HasAudioControl(void)
{
  return GetVolumeTarget() != CECDEVICE_UNKNOWN;
}

cec_logical_address CPeripheralCecAdapter::GetVolumeTarget(void)
{
  std::unique_lock lock(m_critSection);
  return m_volumeTarget;
}

void CPeripheralCecAdapter::SetVolumeTarget(cec_logical_address address)
{
  {
    std::unique_lock lock(m_critSection);
    if (m_volumeTarget == address)
      return;
  }

  CLog::Log(LOGDEBUG, "{} - volume will be controlled {}", __FUNCTION__,
            address == CECDEVICE_AUDIOSYSTEM ? "on the amp"
            : address == CECDEVICE_TV        ? "on the TV"
                                             : "by Kodi");

  /* hand Kodi's volume over before the target takes it: from that point on a mute keypress is
     forwarded over CEC rather than clearing Kodi's own mute, which would leave Kodi muted
     with no way to unmute it. setting the volume to maximum lets Kodi pass its audio through
     unchanged, so the target is the only thing attenuating it */
  if (address != CECDEVICE_UNKNOWN)
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetMute(false);
    appVolume->SetVolume(CApplicationVolumeHandling::VOLUME_MAXIMUM, false);
  }

  std::unique_lock lock(m_critSection);
  m_volumeTarget = address;
}

cec_logical_address CPeripheralCecAdapter::GetTvVolumeTarget(void)
{
  std::unique_lock lock(m_critSection);
  return m_tvVolumeTarget;
}

void CPeripheralCecAdapter::SetTvVolumeTarget(cec_logical_address address)
{
  std::unique_lock lock(m_critSection);
  m_tvVolumeTarget = address;
}

void CPeripheralCecAdapter::SetAmpControlsVolume(bool bSetTo)
{
  /* the setting rules the amp out even while it has system audio mode on */
  const bool bToAmp = bSetTo && GetSettingInt("volume_control") != LOCALISED_ID_NONE;
  SetVolumeTarget(bToAmp ? CECDEVICE_AUDIOSYSTEM : GetTvVolumeTarget());
}

void CPeripheralCecAdapter::SetAmpMuted(bool bSetTo)
{
  std::unique_lock lock(m_critSection);
  /* the mute state belongs to whichever device handles volume, so leave it alone while the amp
     reports one it is not acting on */
  if (m_volumeTarget == CECDEVICE_AUDIOSYSTEM)
    m_bIsMuted = bSetTo;
}

void CPeripheralCecAdapter::ProcessVolumeChange(void)
{
  bool bSendRelease(false);
  CecVolumeChange pendingVolumeChange = VOLUME_CHANGE_NONE;
  cec_logical_address target = CECDEVICE_UNKNOWN;
  {
    std::unique_lock lock(m_critSection);
    target = m_volumeTarget;
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastKeypress);
    if (!m_volumeChangeQueue.empty())
    {
      /* get the first change from the queue */
      pendingVolumeChange = m_volumeChangeQueue.front();
      m_volumeChangeQueue.pop();

      /* remove all dupe entries */
      while (!m_volumeChangeQueue.empty() && m_volumeChangeQueue.front() == pendingVolumeChange)
        m_volumeChangeQueue.pop();

      /* send another keypress after VOLUME_REFRESH_TIMEOUT ms */

      bool bRefresh(duration.count() > VOLUME_REFRESH_TIMEOUT);

      /* only send the keypress when it hasn't been sent yet */
      if (pendingVolumeChange != m_lastChange)
      {
        m_lastKeypress = std::chrono::steady_clock::now();
        m_lastChange = pendingVolumeChange;
      }
      else if (bRefresh)
      {
        m_lastKeypress = std::chrono::steady_clock::now();
        pendingVolumeChange = m_lastChange;
      }
      else
        pendingVolumeChange = VOLUME_CHANGE_NONE;
    }
    else if (m_lastKeypress.time_since_epoch().count() > 0 &&
             duration.count() > VOLUME_CHANGE_TIMEOUT)
    {
      /* send a key release */
      m_lastKeypress = {};
      bSendRelease = true;
      m_lastChange = VOLUME_CHANGE_NONE;
    }
  }

  /* the target can be cleared while changes are queued, so drain the queue but send nothing */
  if (target == CECDEVICE_UNKNOWN)
    return;

  switch (pendingVolumeChange)
  {
    case VOLUME_CHANGE_UP:
      m_cecAdapter->SendKeypress(target, CEC_USER_CONTROL_CODE_VOLUME_UP, false);
      break;
    case VOLUME_CHANGE_DOWN:
      m_cecAdapter->SendKeypress(target, CEC_USER_CONTROL_CODE_VOLUME_DOWN, false);
      break;
    case VOLUME_CHANGE_MUTE:
      m_cecAdapter->SendKeypress(target, CEC_USER_CONTROL_CODE_MUTE, false);
      {
        std::unique_lock lock(m_critSection);
        m_bIsMuted = !m_bIsMuted;
      }
      break;
    case VOLUME_CHANGE_NONE:
      if (bSendRelease)
        m_cecAdapter->SendKeyRelease(target, false);
      break;
  }
}

void CPeripheralCecAdapter::VolumeUp(void)
{
  if (HasAudioControl())
  {
    std::unique_lock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_UP);
  }
}

void CPeripheralCecAdapter::VolumeDown(void)
{
  if (HasAudioControl())
  {
    std::unique_lock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_DOWN);
  }
}

void CPeripheralCecAdapter::ToggleMute(void)
{
  if (HasAudioControl())
  {
    std::unique_lock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_MUTE);
  }
}

bool CPeripheralCecAdapter::IsMuted(void)
{
  if (HasAudioControl())
  {
    std::unique_lock lock(m_critSection);
    return m_bIsMuted;
  }
  return false;
}

void CPeripheralCecAdapter::SetMenuLanguage(const char* strLanguage)
{
  if (StringUtils::EqualsNoCase(m_strMenuLanguage, strLanguage))
    return;

  std::string strGuiLanguage;

  if (!strcmp(strLanguage, "bul"))
    strGuiLanguage = "bg_bg";
  else if (!strcmp(strLanguage, "hrv"))
    strGuiLanguage = "hr_hr";
  else if (!strcmp(strLanguage, "cze"))
    strGuiLanguage = "cs_cz";
  else if (!strcmp(strLanguage, "dan"))
    strGuiLanguage = "da_dk";
  else if (!strcmp(strLanguage, "deu"))
    strGuiLanguage = "de_de";
  else if (!strcmp(strLanguage, "dut"))
    strGuiLanguage = "nl_nl";
  else if (!strcmp(strLanguage, "eng"))
    strGuiLanguage = "en_gb";
  else if (!strcmp(strLanguage, "fin"))
    strGuiLanguage = "fi_fi";
  else if (!strcmp(strLanguage, "fre"))
    strGuiLanguage = "fr_fr";
  else if (!strcmp(strLanguage, "ger"))
    strGuiLanguage = "de_de";
  else if (!strcmp(strLanguage, "gre"))
    strGuiLanguage = "el_gr";
  else if (!strcmp(strLanguage, "hun"))
    strGuiLanguage = "hu_hu";
  else if (!strcmp(strLanguage, "ita"))
    strGuiLanguage = "it_it";
  else if (!strcmp(strLanguage, "nor"))
    strGuiLanguage = "nb_no";
  else if (!strcmp(strLanguage, "pol"))
    strGuiLanguage = "pl_pl";
  else if (!strcmp(strLanguage, "por"))
    strGuiLanguage = "pt_pt";
  else if (!strcmp(strLanguage, "rum"))
    strGuiLanguage = "ro_ro";
  else if (!strcmp(strLanguage, "rus"))
    strGuiLanguage = "ru_ru";
  else if (!strcmp(strLanguage, "srp"))
    strGuiLanguage = "sr_rs@latin";
  else if (!strcmp(strLanguage, "slo"))
    strGuiLanguage = "sk_sk";
  else if (!strcmp(strLanguage, "slv"))
    strGuiLanguage = "sl_si";
  else if (!strcmp(strLanguage, "spa"))
    strGuiLanguage = "es_es";
  else if (!strcmp(strLanguage, "swe"))
    strGuiLanguage = "sv_se";
  else if (!strcmp(strLanguage, "tur"))
    strGuiLanguage = "tr_tr";

  if (!strGuiLanguage.empty())
  {
    strGuiLanguage = "resource.language." + strGuiLanguage;
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SETLANGUAGE, -1, -1, nullptr, strGuiLanguage);
    CLog::Log(LOGDEBUG, "{} - language set to '{}'", __FUNCTION__, strGuiLanguage);
  }
  else
    CLog::Log(LOGWARNING, "{} - TV menu language set to unknown value '{}'", __FUNCTION__,
              strLanguage);
}

void CPeripheralCecAdapter::OnTvStandby(void)
{
  int iActionOnTvStandby = GetSettingInt("standby_pc_on_tv_standby");
  switch (iActionOnTvStandby)
  {
    case LOCALISED_ID_POWEROFF:
      m_bStarted = false;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SYSTEM_POWERDOWN, TMSG_SHUTDOWN);
      break;
    case LOCALISED_ID_SUSPEND:
      m_bStarted = false;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SYSTEM_POWERDOWN, TMSG_SUSPEND);
      break;
    case LOCALISED_ID_HIBERNATE:
      m_bStarted = false;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SYSTEM_POWERDOWN, TMSG_HIBERNATE);
      break;
    case LOCALISED_ID_QUIT:
      m_bStarted = false;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
      break;
    case LOCALISED_ID_PAUSE:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PAUSE_IF_PLAYING);
      break;
    case LOCALISED_ID_STOP:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->IsPlaying())
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_STOP);
      break;
    }
    case LOCALISED_ID_IGNORE:
      break;
    default:
      CLog::Log(LOGERROR, "{} - Unexpected [standby_pc_on_tv_standby] setting value", __FUNCTION__);
      break;
  }
}

void CPeripheralCecAdapter::CecCommand(void* cbParam, const cec_command* command)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!adapter)
    return;

  if (adapter->m_bIsReady)
  {
    switch (command->opcode)
    {
      case CEC_OPCODE_STANDBY:
        if (command->initiator == CECDEVICE_TV &&
            (!adapter->m_standbySent.IsValid() ||
             CDateTime::GetCurrentDateTime() - adapter->m_standbySent >
                 CDateTimeSpan(0, 0, 0, SCREENSAVER_TIMEOUT)))
        {
          adapter->OnTvStandby();
        }
        break;
      case CEC_OPCODE_SET_MENU_LANGUAGE:
        if (adapter->m_bUseTVMenuLanguage == 1 && command->initiator == CECDEVICE_TV &&
            command->parameters.size == 3)
        {
          char strNewLanguage[4];
          for (int iPtr = 0; iPtr < 3; iPtr++)
            strNewLanguage[iPtr] = command->parameters[iPtr];
          strNewLanguage[3] = 0;
          adapter->SetMenuLanguage(strNewLanguage);
        }
        break;
      case CEC_OPCODE_DECK_CONTROL:
        if (command->initiator == CECDEVICE_TV && command->parameters.size == 1 &&
            command->parameters[0] == CEC_DECK_CONTROL_MODE_STOP)
        {
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_STOP;
          adapter->PushCecKeypress(key);
        }
        break;
      case CEC_OPCODE_PLAY:
        if (command->initiator == CECDEVICE_TV && command->parameters.size == 1)
        {
          if (command->parameters[0] == CEC_PLAY_MODE_PLAY_FORWARD)
          {
            cec_keypress key;
            key.duration = 500;
            key.keycode = CEC_USER_CONTROL_CODE_PLAY;
            adapter->PushCecKeypress(key);
          }
          else if (command->parameters[0] == CEC_PLAY_MODE_PLAY_STILL)
          {
            cec_keypress key;
            key.duration = 500;
            key.keycode = CEC_USER_CONTROL_CODE_PAUSE;
            adapter->PushCecKeypress(key);
          }
        }
        break;
      case CEC_OPCODE_SET_SYSTEM_AUDIO_MODE:
      case CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:
        /* the amp only acts on volume keypresses while system audio mode is on, so follow it for
           as long as it is: volume falls back to Kodi's own the moment the amp bows out */
        if (command->initiator == CECDEVICE_AUDIOSYSTEM && command->parameters.size == 1)
          adapter->SetAmpControlsVolume(command->parameters[0] == CEC_SYSTEM_AUDIO_STATUS_ON);
        break;
      case CEC_OPCODE_REPORT_AUDIO_STATUS:
        if (command->initiator == CECDEVICE_AUDIOSYSTEM && command->parameters.size == 1)
          adapter->SetAmpMuted((command->parameters[0] & CEC_AUDIO_MUTE_STATUS_MASK) ==
                               CEC_AUDIO_MUTE_STATUS_MASK);
        break;
      default:
        break;
    }
  }
}

void CPeripheralCecAdapter::CecConfiguration(void* cbParam, const libcec_configuration* config)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!adapter)
    return;

  std::unique_lock lock(adapter->m_critSection);
  adapter->SetConfigurationFromLibCEC(*config);
}

void CPeripheralCecAdapter::CecAlert(void* cbParam,
                                     const libcec_alert alert,
                                     const libcec_parameter data)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!adapter)
    return;

  bool bReopenConnection(false);
  int iAlertString(0);
  switch (alert)
  {
    case CEC_ALERT_SERVICE_DEVICE:
      iAlertString = 36027;
      break;
    case CEC_ALERT_CONNECTION_LOST:
      bReopenConnection = true;
      iAlertString = 36030;
      break;
#if defined(CEC_ALERT_PERMISSION_ERROR)
    case CEC_ALERT_PERMISSION_ERROR:
      bReopenConnection = true;
      iAlertString = 36031;
      break;
    case CEC_ALERT_PORT_BUSY:
      bReopenConnection = true;
      iAlertString = 36032;
      break;
#endif
    default:
      break;
  }

  // display the alert
  if (iAlertString)
  {
    std::string strLog(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(iAlertString));
    if (data.paramType == CEC_PARAMETER_TYPE_STRING && data.paramData)
      strLog += StringUtils::Format(" - {}", (const char*)data.paramData);
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info,
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000), strLog);
  }

  if (bReopenConnection)
  {
    // Reopen the connection asynchronously. Otherwise a deadlock may occur.
    // Reconnect means destruction and recreation of our libcec instance, but libcec
    // calls this callback function synchronously and must not be destroyed meanwhile.
    adapter->ReopenConnection(true);
  }
}

void CPeripheralCecAdapter::CecKeyPress(void* cbParam, const cec_keypress* key)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!!adapter)
    adapter->PushCecKeypress(*key);
}

void CPeripheralCecAdapter::GetNextKey(void)
{
  std::unique_lock lock(m_critSection);
  m_bHasButton = false;
  if (m_bIsReady)
  {
    std::vector<CecButtonPress>::iterator it = m_buttonQueue.begin();
    if (it != m_buttonQueue.end())
    {
      m_currentButton = (*it);
      m_buttonQueue.erase(it);
      m_bHasButton = true;
    }
  }
}

void CPeripheralCecAdapter::PushCecKeypress(const CecButtonPress& key)
{
  CLog::Log(LOGDEBUG, "{} - received key {:2x} duration {}", __FUNCTION__, key.iButton,
            key.iDuration);

  std::unique_lock lock(m_critSection);
  // avoid the queue getting too long
  if (m_configuration.iButtonRepeatRateMs && m_buttonQueue.size() > 5)
    return;
  if (m_configuration.iButtonRepeatRateMs == 0 && key.iDuration > 0)
  {
    if (m_currentButton.iButton == key.iButton && m_currentButton.iDuration == 0)
    {
      // update the duration
      if (m_bHasButton)
        m_currentButton.iDuration = key.iDuration;
      // ignore this one, since it's already been handled by xbmc
      return;
    }
    // if we received a keypress with a duration set, try to find the same one without a duration
    // set, and replace it
    for (std::vector<CecButtonPress>::reverse_iterator it = m_buttonQueue.rbegin();
         it != m_buttonQueue.rend(); ++it)
    {
      if ((*it).iButton == key.iButton)
      {
        if ((*it).iDuration == 0)
        {
          // replace this entry
          (*it).iDuration = key.iDuration;
          return;
        }
        // add a new entry
        break;
      }
    }
  }

  m_buttonQueue.push_back(key);
}

void CPeripheralCecAdapter::PushCecKeypress(const cec_keypress& key)
{
  CecButtonPress xbmcKey;
  xbmcKey.iDuration = key.duration;

  switch (key.keycode)
  {
    case CEC_USER_CONTROL_CODE_SELECT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_SELECT;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_UP;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_DOWN;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_LEFT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LEFT;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_LEFT_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LEFT;
      PushCecKeypress(xbmcKey);
      xbmcKey.iButton = XINPUT_IR_REMOTE_UP;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_LEFT_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LEFT;
      PushCecKeypress(xbmcKey);
      xbmcKey.iButton = XINPUT_IR_REMOTE_DOWN;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_RIGHT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_RIGHT;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_RIGHT_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_RIGHT;
      PushCecKeypress(xbmcKey);
      xbmcKey.iButton = XINPUT_IR_REMOTE_UP;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_RIGHT_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_RIGHT;
      PushCecKeypress(xbmcKey);
      xbmcKey.iButton = XINPUT_IR_REMOTE_DOWN;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_SETUP_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_TITLE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_CONTENTS_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CONTENTS_MENU;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_ROOT_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_ROOT_MENU;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_TOP_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_TOP_MENU;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_DVD_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_DVD_MENU;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_FAVORITE_MENU:
      xbmcKey.iButton = XINPUT_IR_REMOTE_FAVORITE_MENU;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_EXIT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_BACK;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_ENTER:
      xbmcKey.iButton = XINPUT_IR_REMOTE_ENTER;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_CHANNEL_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CHANNEL_MINUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_CHANNEL_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CHANNEL_PLUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LAST;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_SOUND_SELECT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LANGUAGE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_POWER:
    case CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION:
    case CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION:
      xbmcKey.iButton = XINPUT_IR_REMOTE_POWER;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_VOLUME_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_VOLUME_PLUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_VOLUME_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_VOLUME_MINUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_MUTE:
    case CEC_USER_CONTROL_CODE_MUTE_FUNCTION:
    case CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION:
      xbmcKey.iButton = XINPUT_IR_REMOTE_MUTE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_PLAY:
      xbmcKey.iButton = XINPUT_IR_REMOTE_PLAY;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_STOP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_STOP;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_PAUSE:
      xbmcKey.iButton = XINPUT_IR_REMOTE_PAUSE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_REWIND:
      xbmcKey.iButton = XINPUT_IR_REMOTE_REVERSE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_FAST_FORWARD:
      xbmcKey.iButton = XINPUT_IR_REMOTE_FORWARD;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER0:
      xbmcKey.iButton = XINPUT_IR_REMOTE_0;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER1:
      xbmcKey.iButton = XINPUT_IR_REMOTE_1;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER2:
      xbmcKey.iButton = XINPUT_IR_REMOTE_2;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER3:
      xbmcKey.iButton = XINPUT_IR_REMOTE_3;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER4:
      xbmcKey.iButton = XINPUT_IR_REMOTE_4;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER5:
      xbmcKey.iButton = XINPUT_IR_REMOTE_5;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER6:
      xbmcKey.iButton = XINPUT_IR_REMOTE_6;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER7:
      xbmcKey.iButton = XINPUT_IR_REMOTE_7;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER8:
      xbmcKey.iButton = XINPUT_IR_REMOTE_8;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NUMBER9:
      xbmcKey.iButton = XINPUT_IR_REMOTE_9;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_RECORD:
      xbmcKey.iButton = XINPUT_IR_REMOTE_RECORD;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_CLEAR:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CLEAR;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION:
      xbmcKey.iButton = XINPUT_IR_REMOTE_INFO;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_PAGE_UP:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CHANNEL_PLUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_PAGE_DOWN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_CHANNEL_MINUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_FORWARD:
      xbmcKey.iButton = XINPUT_IR_REMOTE_SKIP_PLUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_BACKWARD:
      xbmcKey.iButton = XINPUT_IR_REMOTE_SKIP_MINUS;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_F1_BLUE:
      xbmcKey.iButton = XINPUT_IR_REMOTE_BLUE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_F2_RED:
      xbmcKey.iButton = XINPUT_IR_REMOTE_RED;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_F3_GREEN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_GREEN;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_F4_YELLOW:
      xbmcKey.iButton = XINPUT_IR_REMOTE_YELLOW;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE:
      xbmcKey.iButton = XINPUT_IR_REMOTE_GUIDE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST:
      xbmcKey.iButton = XINPUT_IR_REMOTE_LIVE_TV;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_NEXT_FAVORITE:
    case CEC_USER_CONTROL_CODE_DOT:
    case CEC_USER_CONTROL_CODE_AN_RETURN:
      xbmcKey.iButton = XINPUT_IR_REMOTE_TITLE; // context menu
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_DATA:
      xbmcKey.iButton = XINPUT_IR_REMOTE_TELETEXT;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_SUB_PICTURE:
      xbmcKey.iButton = XINPUT_IR_REMOTE_SUBTITLE;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_EJECT:
      xbmcKey.iButton = XINPUT_IR_REMOTE_EJECT;
      PushCecKeypress(xbmcKey);
      break;
    case CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION:
    case CEC_USER_CONTROL_CODE_INPUT_SELECT:
    case CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION:
    case CEC_USER_CONTROL_CODE_HELP:
    case CEC_USER_CONTROL_CODE_STOP_RECORD:
    case CEC_USER_CONTROL_CODE_PAUSE_RECORD:
    case CEC_USER_CONTROL_CODE_ANGLE:
    case CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND:
    case CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING:
    case CEC_USER_CONTROL_CODE_PLAY_FUNCTION:
    case CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION:
    case CEC_USER_CONTROL_CODE_RECORD_FUNCTION:
    case CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION:
    case CEC_USER_CONTROL_CODE_STOP_FUNCTION:
    case CEC_USER_CONTROL_CODE_TUNE_FUNCTION:
    case CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION:
    case CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION:
    case CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION:
    case CEC_USER_CONTROL_CODE_F5:
    case CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE:
    case CEC_USER_CONTROL_CODE_NUMBER11:
    case CEC_USER_CONTROL_CODE_NUMBER12:
    case CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE:
    case CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION:
    case CEC_USER_CONTROL_CODE_UNKNOWN:
    default:
      break;
  }
}

int CPeripheralCecAdapter::GetButton(void)
{
  std::unique_lock lock(m_critSection);
  if (!m_bHasButton)
    GetNextKey();

  return m_bHasButton ? m_currentButton.iButton : 0;
}

unsigned int CPeripheralCecAdapter::GetHoldTime(void)
{
  std::unique_lock lock(m_critSection);
  if (!m_bHasButton)
    GetNextKey();

  return m_bHasButton ? m_currentButton.iDuration : 0;
}

void CPeripheralCecAdapter::ResetButton(void)
{
  std::unique_lock lock(m_critSection);
  m_bHasButton = false;

  // wait for the key release if the duration isn't 0
  if (m_currentButton.iDuration > 0)
  {
    m_currentButton.iButton = 0;
    m_currentButton.iDuration = 0;
  }
}

void CPeripheralCecAdapter::OnSettingChanged(const std::string& strChangedSetting)
{
  if (StringUtils::EqualsNoCase(strChangedSetting, "enabled"))
  {
    bool bEnabled(GetSettingBool("enabled"));
    if (!bEnabled && IsRunning())
    {
      CLog::Log(LOGDEBUG, "{} - closing the CEC connection", __FUNCTION__);
      StopThread(true);
    }
    else if (bEnabled && !IsRunning())
    {
      CLog::Log(LOGDEBUG, "{} - starting the CEC connection", __FUNCTION__);
      SetConfigurationFromSettings();
      InitialiseFeature(FEATURE_CEC);
    }
  }
  else if (IsRunning())
  {
    if (m_queryThread->IsRunning())
    {
      CLog::Log(LOGDEBUG, "{} - sending the updated configuration to libCEC", __FUNCTION__);
      SetConfigurationFromSettings();
      m_queryThread->UpdateConfiguration(&m_configuration);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - restarting the CEC connection", __FUNCTION__);
    SetConfigurationFromSettings();
    InitialiseFeature(FEATURE_CEC);
  }
}

void CPeripheralCecAdapter::CecSourceActivated(void* cbParam,
                                               const CEC::cec_logical_address address,
                                               const uint8_t activated)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!adapter)
    return;

  // wake up the screensaver, so the user doesn't switch to a black screen
  if (activated == 1)
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->WakeUpScreenSaverAndDPMS();
  }

  if (adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") != LOCALISED_ID_NONE)
  {
    bool bShowingSlideshow =
        (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW);
    CGUIWindowSlideShow* pSlideShow =
        bShowingSlideshow
            ? CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(
                  WINDOW_SLIDESHOW)
            : nullptr;

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();

    bool bPlayingAndDeactivated = activated == 0 && ((pSlideShow && pSlideShow->IsPlaying()) ||
                                                     !appPlayer->IsPausedPlayback());
    bool bPausedAndActivated =
        activated == 1 && adapter->m_bPlaybackPaused &&
        ((pSlideShow && pSlideShow->IsPaused()) || (appPlayer && appPlayer->IsPausedPlayback()));
    if (bPlayingAndDeactivated)
      adapter->m_bPlaybackPaused = true;
    else if (bPausedAndActivated)
      adapter->m_bPlaybackPaused = false;

    if ((bPlayingAndDeactivated || bPausedAndActivated) &&
        adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") == LOCALISED_ID_PAUSE)
    {
      if (pSlideShow)
        // pause/resume slideshow
        pSlideShow->OnAction(CAction(ACTION_PAUSE));
      else
        // pause/resume player
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
    }
    else if (bPlayingAndDeactivated &&
             adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") == LOCALISED_ID_STOP)
    {
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_STOP));
      else
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
    }
  }
}

void CPeripheralCecAdapter::CecLogMessage(void* cbParam, const cec_log_message* message)
{
  CPeripheralCecAdapter* adapter = static_cast<CPeripheralCecAdapter*>(cbParam);
  if (!adapter)
    return;

  int iLevel = -1;
  switch (message->level)
  {
    case CEC_LOG_ERROR:
      iLevel = LOGERROR;
      break;
    case CEC_LOG_WARNING:
      iLevel = LOGWARNING;
      break;
    case CEC_LOG_NOTICE:
      iLevel = LOGDEBUG;
      break;
    case CEC_LOG_TRAFFIC:
    case CEC_LOG_DEBUG:
      iLevel = LOGDEBUG;
      break;
    default:
      break;
  }

  if (iLevel >= CEC_LOG_NOTICE ||
      (iLevel >= 0 && CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG)))
    CLog::Log(iLevel, LOGCEC, "{} - {}", __FUNCTION__, message->message);
}

void CPeripheralCecAdapter::SetConfigurationFromLibCEC(const CEC::libcec_configuration& config)
{
  // these values are reported by libCEC, so they must not be marked as changed. sending them back
  // to libCEC when the settings are persisted would needlessly reconfigure the adapter, and make
  // Kodi the active source again when 'activate_source' is enabled
  bool bChanged(false);

  // set the primary device type
  m_configuration.deviceTypes.Clear();
  m_configuration.deviceTypes.Add(config.deviceTypes[0]);

  // hide the "connected device" and "hdmi port number" settings when the PA was autodetected
  bool bPAAutoDetected(config.bAutodetectAddress == 1);

  SetSettingVisible("connected_device", !bPAAutoDetected);
  SetSettingVisible("cec_hdmi_port", !bPAAutoDetected);

  // set the connected device
  m_configuration.baseDevice = config.baseDevice;
  bChanged |= SetSetting(
      "connected_device",
      config.baseDevice == CECDEVICE_AUDIOSYSTEM ? LOCALISED_ID_AVR : LOCALISED_ID_TV, false);

  // set the HDMI port number
  m_configuration.iHDMIPort = config.iHDMIPort;
  bChanged |= SetSetting("cec_hdmi_port", config.iHDMIPort, false);

  // set the physical address, when baseDevice or iHDMIPort are not set
  std::string strPhysicalAddress("0");
  if (!bPAAutoDetected && (m_configuration.baseDevice == CECDEVICE_UNKNOWN ||
                           m_configuration.iHDMIPort < CEC_MIN_HDMI_PORTNUMBER ||
                           m_configuration.iHDMIPort > CEC_MAX_HDMI_PORTNUMBER))
  {
    m_configuration.iPhysicalAddress = config.iPhysicalAddress;
    strPhysicalAddress = StringUtils::Format("{:x}", config.iPhysicalAddress);
  }
  bChanged |= SetSetting("physical_address", strPhysicalAddress, false);

  // set the devices to wake when starting
  m_configuration.wakeDevices = config.wakeDevices;
  bChanged |= WriteLogicalAddresses(config.wakeDevices, "wake_devices", "wake_devices_advanced");

  // set the devices to power off when stopping
  m_configuration.powerOffDevices = config.powerOffDevices;
  bChanged |=
      WriteLogicalAddresses(config.powerOffDevices, "standby_devices", "standby_devices_advanced");

  // set the boolean settings
  m_configuration.bActivateSource = config.bActivateSource;
  bChanged |= SetSetting("activate_source", m_configuration.bActivateSource == 1, false);

  m_configuration.iDoubleTapTimeoutMs = config.iDoubleTapTimeoutMs;
  bChanged |= SetSetting("double_tap_timeout_ms", (int)m_configuration.iDoubleTapTimeoutMs, false);

  m_configuration.iButtonRepeatRateMs = config.iButtonRepeatRateMs;
  bChanged |= SetSetting("button_repeat_rate_ms", (int)m_configuration.iButtonRepeatRateMs, false);

  m_configuration.iButtonReleaseDelayMs = config.iButtonReleaseDelayMs;
  bChanged |=
      SetSetting("button_release_delay_ms", (int)m_configuration.iButtonReleaseDelayMs, false);

  m_configuration.bPowerOffOnStandby = config.bPowerOffOnStandby;

  m_configuration.iFirmwareVersion = config.iFirmwareVersion;

  memcpy(m_configuration.strDeviceLanguage, config.strDeviceLanguage, 3);
  m_configuration.iFirmwareBuildDate = config.iFirmwareBuildDate;

  SetVersionInfo(m_configuration);

  if (bChanged)
    CLog::Log(LOGDEBUG, "SetConfigurationFromLibCEC - settings updated by libCEC");
}

void CPeripheralCecAdapter::SetConfigurationFromSettings(void)
{
  // client version matches the version of libCEC that we originally used the API from
  m_configuration.clientVersion = LIBCEC_VERSION_TO_UINT(4, 0, 0);

  // device name 'XBMC'
  snprintf(m_configuration.strDeviceName, 13, "%s", GetSettingString("device_name").c_str());

  // set the primary device type
  m_configuration.deviceTypes.Clear();
  switch (GetSettingInt("device_type"))
  {
    case LOCALISED_ID_PLAYBACK_DEVICE:
      m_configuration.deviceTypes.Add(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
      break;
    case LOCALISED_ID_TUNER_DEVICE:
      m_configuration.deviceTypes.Add(CEC_DEVICE_TYPE_TUNER);
      break;
    case LOCALISED_ID_RECORDING_DEVICE:
    default:
      m_configuration.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
      break;
  }

  // always try to autodetect the address.
  // when the firmware supports this, it will override the physical address, connected device and
  // hdmi port settings
  m_configuration.bAutodetectAddress = CEC_DEFAULT_SETTING_AUTODETECT_ADDRESS;

  // set the physical address
  // when set, it will override the connected device and hdmi port settings
  std::string strPhysicalAddress = GetSettingString("physical_address");
  int iPhysicalAddress;
  if (sscanf(strPhysicalAddress.c_str(), "%x", &iPhysicalAddress) &&
      iPhysicalAddress >= CEC_PHYSICAL_ADDRESS_TV && iPhysicalAddress <= CEC_MAX_PHYSICAL_ADDRESS)
    m_configuration.iPhysicalAddress = iPhysicalAddress;
  else
    m_configuration.iPhysicalAddress = CEC_PHYSICAL_ADDRESS_TV;

  // set the connected device
  int iConnectedDevice = GetSettingInt("connected_device");
  if (iConnectedDevice == LOCALISED_ID_AVR)
    m_configuration.baseDevice = CECDEVICE_AUDIOSYSTEM;
  else if (iConnectedDevice == LOCALISED_ID_TV)
    m_configuration.baseDevice = CECDEVICE_TV;

  // set the HDMI port number
  int iHDMIPort = GetSettingInt("cec_hdmi_port");
  if (iHDMIPort >= CEC_MIN_HDMI_PORTNUMBER && iHDMIPort <= CEC_MAX_HDMI_PORTNUMBER)
    m_configuration.iHDMIPort = iHDMIPort;

  // set the tv vendor override
  int iVendor = GetSettingInt("tv_vendor");
  if (iVendor >= CEC_MIN_VENDORID && iVendor <= CEC_MAX_VENDORID)
    m_configuration.tvVendor = iVendor;

  // read the devices to wake when starting
  std::string strWakeDevices = GetSettingString("wake_devices_advanced");
  StringUtils::Trim(strWakeDevices);
  m_configuration.wakeDevices.Clear();
  if (!strWakeDevices.empty())
    ReadLogicalAddresses(strWakeDevices, m_configuration.wakeDevices);
  else
    ReadLogicalAddresses(GetSettingInt("wake_devices"), m_configuration.wakeDevices);

  // read the devices to power off when stopping
  std::string strStandbyDevices = GetSettingString("standby_devices_advanced");
  StringUtils::Trim(strStandbyDevices);
  m_configuration.powerOffDevices.Clear();
  if (!strStandbyDevices.empty())
    ReadLogicalAddresses(strStandbyDevices, m_configuration.powerOffDevices);
  else
    ReadLogicalAddresses(GetSettingInt("standby_devices"), m_configuration.powerOffDevices);

  // read the boolean settings
  m_bUseTVMenuLanguage = GetSettingBool("use_tv_menu_language");
  m_configuration.bActivateSource = GetSettingBool("activate_source") ? 1 : 0;
  m_bPowerOnScreensaver = GetSettingBool("cec_wake_screensaver");
  m_bSendInactiveSource = GetSettingBool("send_inactive_source");
  m_configuration.bAutoWakeAVR = GetSettingBool("power_avr_on_as") ? 1 : 0;

  // read the mutually exclusive boolean settings
  int iStandbyAction(GetSettingInt("standby_pc_on_tv_standby"));
  m_configuration.bPowerOffOnStandby =
      (iStandbyAction == LOCALISED_ID_SUSPEND || iStandbyAction == LOCALISED_ID_HIBERNATE) ? 1 : 0;
  m_bShutdownOnStandby = iStandbyAction == LOCALISED_ID_POWEROFF;

  // double tap prevention timeout in ms
  m_configuration.iDoubleTapTimeoutMs = GetSettingInt("double_tap_timeout_ms");
#if CEC_LIB_VERSION_MAJOR < 8
  // libCEC before 8.0.0 does no double tap prevention and reads this field as the button
  // repeat delay instead, where the setting's "off" would make repeats start immediately
  if (m_configuration.iDoubleTapTimeoutMs == 0)
    m_configuration.iDoubleTapTimeoutMs = CEC_BUTTON_REPEAT_DELAY_MS;
#endif
  m_configuration.iButtonRepeatRateMs = GetSettingInt("button_repeat_rate_ms");
  m_configuration.iButtonReleaseDelayMs = GetSettingInt("button_release_delay_ms");

  if (GetSettingBool("pause_playback_on_deactivate"))
  {
    // migration of a deprecated setting. the new value is read when it's needed, so it doesn't
    // have to be marked as changed
    SetSetting("pause_or_stop_playback_on_deactivate", LOCALISED_ID_PAUSE, false);
    SetSetting("pause_playback_on_deactivate", false, false);
  }

  if (GetSettingBool("cec_standby_screensaver"))
  {
    // migration of a deprecated setting. the new value is read when it's needed, so it doesn't
    // have to be marked as changed
    SetSetting("cec_standby_screensaver_mode", LOCALISED_ID_UNLESS_PLAYING, false);
    SetSetting("cec_standby_screensaver", false, false);
  }
}

void CPeripheralCecAdapter::ReadLogicalAddresses(const std::string& strString,
                                                 cec_logical_addresses& addresses)
{
  for (size_t iPtr = 0; iPtr < strString.size(); iPtr++)
  {
    std::string strDevice = strString.substr(iPtr, 1);
    StringUtils::Trim(strDevice);
    if (!strDevice.empty())
    {
      int iDevice(0);
      if (sscanf(strDevice.c_str(), "%x", &iDevice) == 1 && iDevice >= 0 && iDevice <= 0xF)
        addresses.Set((cec_logical_address)iDevice);
    }
  }
}

void CPeripheralCecAdapter::ReadLogicalAddresses(int iLocalisedId, cec_logical_addresses& addresses)
{
  addresses.Clear();
  switch (iLocalisedId)
  {
    case LOCALISED_ID_TV:
      addresses.Set(CECDEVICE_TV);
      break;
    case LOCALISED_ID_AVR:
      addresses.Set(CECDEVICE_AUDIOSYSTEM);
      break;
    case LOCALISED_ID_TV_AVR:
      addresses.Set(CECDEVICE_TV);
      addresses.Set(CECDEVICE_AUDIOSYSTEM);
      break;
    case LOCALISED_ID_NONE:
    default:
      break;
  }
}

bool CPeripheralCecAdapter::WriteLogicalAddresses(const cec_logical_addresses& addresses,
                                                  const std::string& strSettingName,
                                                  const std::string& strAdvancedSettingName)
{
  bool bChanged(false);

  // only update the advanced setting if it was set by the user
  if (!GetSettingString(strAdvancedSettingName).empty())
  {
    std::string strPowerOffDevices;
    for (unsigned int iPtr = CECDEVICE_TV; iPtr <= CECDEVICE_BROADCAST; iPtr++)
      if (addresses[iPtr])
        strPowerOffDevices += StringUtils::Format(" {:X}", iPtr);
    StringUtils::Trim(strPowerOffDevices);
    // reported by libCEC, so don't mark it as changed
    bChanged = SetSetting(strAdvancedSettingName, strPowerOffDevices, false);
  }

  int iSettingPowerOffDevices = LOCALISED_ID_NONE;
  if (addresses[CECDEVICE_TV] && addresses[CECDEVICE_AUDIOSYSTEM])
    iSettingPowerOffDevices = LOCALISED_ID_TV_AVR;
  else if (addresses[CECDEVICE_TV])
    iSettingPowerOffDevices = LOCALISED_ID_TV;
  else if (addresses[CECDEVICE_AUDIOSYSTEM])
    iSettingPowerOffDevices = LOCALISED_ID_AVR;
  return SetSetting(strSettingName, iSettingPowerOffDevices, false) || bChanged;
}

CPeripheralCecAdapterUpdateThread::CPeripheralCecAdapterUpdateThread(
    CPeripheralCecAdapter* adapter, libcec_configuration* configuration)
  : CThread("CECAdapterUpdate"),
    m_adapter(adapter),
    m_configuration(*configuration)
{
  m_nextConfiguration.Clear();
  m_event.Reset();
}

CPeripheralCecAdapterUpdateThread::~CPeripheralCecAdapterUpdateThread(void)
{
  StopThread(false);
  m_event.Set();
  StopThread(true);
}

void CPeripheralCecAdapterUpdateThread::Signal(void)
{
  m_event.Set();
}

bool CPeripheralCecAdapterUpdateThread::UpdateConfiguration(libcec_configuration* configuration)
{
  std::unique_lock lock(m_critSection);
  if (!configuration)
    return false;

  if (m_bIsUpdating)
  {
    m_bNextConfigurationScheduled = true;
    m_nextConfiguration = *configuration;
  }
  else
  {
    m_configuration = *configuration;
    m_event.Set();
  }
  return true;
}

bool CPeripheralCecAdapterUpdateThread::WaitReady(void)
{
  // don't wait if we're not powering up anything
  if (m_configuration.wakeDevices.IsEmpty() && m_configuration.bActivateSource == 0)
    return true;

  // wait for the TV if we're configured to become the active source.
  // wait for the first device in the wake list otherwise.
  cec_logical_address waitFor =
      (m_configuration.bActivateSource == 1) ? CECDEVICE_TV : m_configuration.wakeDevices.primary;

  cec_power_status powerStatus(CEC_POWER_STATUS_UNKNOWN);
  bool bContinue(true);
  while (bContinue && !m_adapter->m_bStop && !m_bStop && powerStatus != CEC_POWER_STATUS_ON)
  {
    powerStatus = m_adapter->m_cecAdapter->GetDevicePowerStatus(waitFor);
    if (powerStatus != CEC_POWER_STATUS_ON)
      bContinue = !m_event.Wait(1000ms);
  }

  return powerStatus == CEC_POWER_STATUS_ON;
}

void CPeripheralCecAdapterUpdateThread::UpdateMenuLanguage(void)
{
  // request the menu language of the TV
  if (m_adapter->m_bUseTVMenuLanguage == 1)
  {
    CLog::Log(LOGDEBUG, "{} - requesting the menu language of the TV", __FUNCTION__);
    std::string language(m_adapter->m_cecAdapter->GetDeviceMenuLanguage(CECDEVICE_TV));
    m_adapter->SetMenuLanguage(language.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - using TV menu language is disabled", __FUNCTION__);
  }
}

void CPeripheralCecAdapterUpdateThread::UpdateTvVolumeTarget(void)
{
  const int iVolumeControl = m_adapter->GetSettingInt("volume_control");
  cec_logical_address target = CECDEVICE_UNKNOWN;

  if (iVolumeControl != LOCALISED_ID_NONE)
  {
    /* CEC 2.0 is the first version that requires a TV to act on volume commands. TVs that report
       an older version are only addressed when the user opted in. */
    const cec_version version = m_adapter->m_cecAdapter->GetDeviceCecVersion(CECDEVICE_TV);
    if (version >= CEC_VERSION_2_0 || iVolumeControl == LOCALISED_ID_VOLUME_ALWAYS)
      target = CECDEVICE_TV;
    else
      CLog::Log(LOGDEBUG,
                "{} - the TV reports CEC version {}, which does not require it to act on volume "
                "commands",
                __FUNCTION__, m_adapter->m_cecAdapter->ToString(version));
  }

  m_adapter->SetTvVolumeTarget(target);
}

std::string CPeripheralCecAdapterUpdateThread::UpdateAudioSystemStatus(void)
{
  std::string strAmpName;

  /* the TV takes over whenever the amp is not handling volume, so settle on it first */
  UpdateTvVolumeTarget();

  if (!m_adapter->m_cecAdapter->IsActiveDeviceType(CEC_DEVICE_TYPE_AUDIO_SYSTEM))
  {
    CLog::Log(LOGDEBUG, "{} - no CEC capable amplifier found", __FUNCTION__);
    m_adapter->SetAmpControlsVolume(false);
    return strAmpName;
  }

  // request the OSD name of the amp
  std::string ampName(m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_AUDIOSYSTEM));
  strAmpName += ampName;

#if CEC_LIB_HAS_SYSTEM_AUDIO_MODE_STATUS
  /* an amp only acts on volume keypresses while system audio mode is on. handing it the volume
     while it is off leaves the user with no working volume control at all. */
  if (m_adapter->m_cecAdapter->SystemAudioModeStatus() != CEC_SYSTEM_AUDIO_STATUS_ON)
  {
    CLog::Log(LOGDEBUG, "{} - CEC capable amplifier found ({}), but system audio mode is off",
              __FUNCTION__, ampName);
    m_adapter->SetAmpControlsVolume(false);
    return strAmpName;
  }
#endif

  CLog::Log(LOGDEBUG, "{} - CEC capable amplifier found ({})", __FUNCTION__, ampName);

  m_adapter->SetAmpControlsVolume(true);

  /* adopt the amp's mute state, so the first mute keypress does not toggle it the wrong way */
  const uint8_t audioStatus = m_adapter->m_cecAdapter->AudioStatus();
  if (audioStatus != CEC_AUDIO_VOLUME_STATUS_UNKNOWN)
    m_adapter->SetAmpMuted((audioStatus & CEC_AUDIO_MUTE_STATUS_MASK) ==
                           CEC_AUDIO_MUTE_STATUS_MASK);

  return strAmpName;
}

bool CPeripheralCecAdapterUpdateThread::SetInitialConfiguration(void)
{
  // the option to make XBMC the active source is set
  if (m_configuration.bActivateSource == 1)
    m_adapter->m_cecAdapter->SetActiveSource();

  // devices to wake are set
  cec_logical_addresses tvOnly;
  tvOnly.Clear();
  tvOnly.Set(CECDEVICE_TV);
  if (!m_configuration.wakeDevices.IsEmpty() &&
      (m_configuration.wakeDevices != tvOnly || m_configuration.bActivateSource == 0))
    m_adapter->m_cecAdapter->PowerOnDevices(CECDEVICE_BROADCAST);

  // wait until devices are powered up
  if (!WaitReady())
    return false;

  UpdateMenuLanguage();

  // request the OSD name of the TV
  std::string strNotification;
  std::string tvName(m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_TV));
  strNotification = StringUtils::Format(
      "{}: {}", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36016), tvName);

  std::string strAmpName = UpdateAudioSystemStatus();
  if (!strAmpName.empty())
    strNotification += StringUtils::Format("- {}", strAmpName);

  m_adapter->m_bIsReady = true;

  // and let the gui know that we're done
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Info,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000), strNotification);

  std::unique_lock lock(m_critSection);
  m_bIsUpdating = false;
  return true;
}

bool CPeripheralCecAdapter::IsRunning(void) const
{
  std::unique_lock lock(m_critSection);
  return m_bIsRunning;
}

void CPeripheralCecAdapterUpdateThread::Process(void)
{
  // set the initial configuration
  if (!SetInitialConfiguration())
    return;

  // and wait for updates
  bool bUpdate(false);
  while (!m_bStop)
  {
    // update received
    if (bUpdate || m_event.Wait(500ms))
    {
      if (m_bStop)
        return;
      // set the new configuration
      libcec_configuration configuration;
      {
        std::unique_lock lock(m_critSection);
        configuration = m_configuration;
        m_bIsUpdating = false;
      }

      CLog::Log(LOGDEBUG, "{} - updating the configuration", __FUNCTION__);
      bool bConfigSet(m_adapter->m_cecAdapter->SetConfiguration(&configuration));
      // display message: config updated / failed to update
      if (!bConfigSet)
        CLog::Log(LOGERROR, "{} - libCEC couldn't set the new configuration", __FUNCTION__);
      else
      {
        UpdateMenuLanguage();
        UpdateAudioSystemStatus();
      }

      CGUIDialogKaiToast::QueueNotification(
          CGUIDialogKaiToast::Info,
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(36000),
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(bConfigSet ? 36023
                                                                                      : 36024));

      {
        std::unique_lock lock(m_critSection);
        if ((bUpdate = m_bNextConfigurationScheduled) == true)
        {
          // another update is scheduled
          m_bNextConfigurationScheduled = false;
          m_configuration = m_nextConfiguration;
        }
        else
        {
          // nothing left to do, wait for updates
          m_bIsUpdating = false;
          m_event.Reset();
        }
      }
    }
  }
}

void CPeripheralCecAdapter::OnDeviceRemoved(void)
{
  {
    std::unique_lock lock(m_critSection);
    m_bDeviceRemoved = true;
  }

  CPeripheral::OnDeviceRemoved();
}

namespace PERIPHERALS
{

class CPeripheralCecAdapterReopenJob : public CJob
{
public:
  CPeripheralCecAdapterReopenJob(CPeripheralCecAdapter* adapter) : m_adapter(adapter) {}
  ~CPeripheralCecAdapterReopenJob() override = default;

  bool DoWork(void) override { return m_adapter->ReopenConnection(false); }

private:
  CPeripheralCecAdapter* m_adapter;
};

}; // namespace PERIPHERALS

bool CPeripheralCecAdapter::ReopenConnection(bool bAsync /* = false */)
{
  if (bAsync)
  {
    CServiceBroker::GetJobManager()->AddJob(new CPeripheralCecAdapterReopenJob(this), nullptr,
                                            CJob::PRIORITY_NORMAL);
    return true;
  }

  // stop running thread
  {
    std::unique_lock lock(m_critSection);
    m_iExitCode = EXITCODE_RESTARTAPP;
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    StopThread(false);
  }
  StopThread();

  // reset all members to their defaults
  ResetMembers();

  // reopen the connection
  return InitialiseFeature(FEATURE_CEC);
}

void CPeripheralCecAdapter::ActivateSource(void)
{
  std::unique_lock lock(m_critSection);
  m_bActiveSourcePending = true;
}

void CPeripheralCecAdapter::ProcessActivateSource(void)
{
  bool bActivate(false);

  {
    std::unique_lock lock(m_critSection);
    bActivate = m_bActiveSourcePending;
    m_bActiveSourcePending = false;
  }

  if (bActivate)
    m_cecAdapter->SetActiveSource();
}

void CPeripheralCecAdapter::StandbyDevices(void)
{
  std::unique_lock lock(m_critSection);
  m_bStandbyPending = true;
}

void CPeripheralCecAdapter::ProcessStandbyDevices(void)
{
  bool bStandby(false);

  {
    std::unique_lock lock(m_critSection);
    bStandby = m_bStandbyPending;
    m_bStandbyPending = false;
    if (bStandby)
      m_bGoingToStandby = true;
  }

  if (bStandby)
  {
    if (!m_configuration.powerOffDevices.IsEmpty())
    {
      m_standbySent = CDateTime::GetCurrentDateTime();
      m_cecAdapter->StandbyDevices(CECDEVICE_BROADCAST);
    }
    else if (m_bSendInactiveSource == 1)
    {
      CLog::Log(LOGDEBUG, "{} - sending inactive source commands", __FUNCTION__);
      m_cecAdapter->SetInactiveView();
    }
  }
}

bool CPeripheralCecAdapter::ToggleDeviceState(CecStateChange mode /*= STATE_SWITCH_TOGGLE */,
                                              bool forceType /*= false */)
{
  if (!IsRunning())
    return false;
  if (m_cecAdapter->IsLibCECActiveSource() &&
      (mode == STATE_SWITCH_TOGGLE || mode == STATE_STANDBY))
  {
    CLog::Log(LOGDEBUG, "{} - putting CEC device on standby...", __FUNCTION__);
    StandbyDevices();
    return false;
  }
  else if (mode == STATE_SWITCH_TOGGLE || mode == STATE_ACTIVATE_SOURCE)
  {
    CLog::Log(LOGDEBUG, "{} - waking up CEC device...", __FUNCTION__);
    ActivateSource();
    return true;
  }

  return false;
}

CecPowerStatus CPeripheralCecAdapter::GetDevicePowerStatus(void)
{
  if (!IsRunning())
    return CecPowerStatus::NO_ADAPTER;

  // libCEC caches the power status internally and only re-requests it from the device when
  // needed. So this call is cheap.
  switch (m_cecAdapter->GetDevicePowerStatus(CECDEVICE_TV))
  {
    case CEC_POWER_STATUS_ON:
      return CecPowerStatus::ON;
    case CEC_POWER_STATUS_STANDBY:
      return CecPowerStatus::STANDBY;
    case CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON:
      return CecPowerStatus::TRANSITION_TO_ON;
    case CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY:
      return CecPowerStatus::TRANSITION_TO_STANDBY;
    default:
      return CecPowerStatus::UNKNOWN;
  }
}
