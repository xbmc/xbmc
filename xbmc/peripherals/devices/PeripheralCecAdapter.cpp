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
#if defined(HAVE_LIBCEC)
#include "PeripheralCecAdapter.h"
#include "input/XBIRRemote.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "DynamicDll.h"
#include "threads/SingleLock.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "pictures/GUIWindowSlideShow.h"
#include "settings/AdvancedSettings.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Variant.h"

#include <libcec/cec.h>

using namespace KODI::MESSAGING;
using namespace PERIPHERALS;
using namespace ANNOUNCEMENT;
using namespace CEC;

#define CEC_LIB_SUPPORTED_VERSION LIBCEC_VERSION_TO_UINT(3, 0, 0)

/* time in seconds to ignore standby commands from devices after the screensaver has been activated */
#define SCREENSAVER_TIMEOUT       20
#define VOLUME_CHANGE_TIMEOUT     250
#define VOLUME_REFRESH_TIMEOUT    100

#define LOCALISED_ID_TV           36037
#define LOCALISED_ID_AVR          36038
#define LOCALISED_ID_TV_AVR       36039
#define LOCALISED_ID_STOP         36044
#define LOCALISED_ID_PAUSE        36045

#define LOCALISED_ID_NONE         231

/* time in seconds to suppress source activation after receiving OnStop */
#define CEC_SUPPRESS_ACTIVATE_SOURCE_AFTER_ON_STOP 2

class DllLibCECInterface
{
public:
  virtual ~DllLibCECInterface() {}
  virtual ICECAdapter* CECInitialise(libcec_configuration *configuration)=0;
  virtual void*        CECDestroy(ICECAdapter *adapter)=0;
};

class DllLibCEC : public DllDynamic, DllLibCECInterface
{
  DECLARE_DLL_WRAPPER(DllLibCEC, DLL_PATH_LIBCEC)

  DEFINE_METHOD1(ICECAdapter*, CECInitialise, (libcec_configuration *p1))
  DEFINE_METHOD1(void*       , CECDestroy,    (ICECAdapter *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(CECInitialise,  CECInitialise)
    RESOLVE_METHOD_RENAME(CECDestroy, CECDestroy)
  END_METHOD_RESOLVE()
};

CPeripheralCecAdapter::CPeripheralCecAdapter(const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheralHID(scanResult, bus),
  CThread("CECAdapter"),
  m_dll(NULL),
  m_cecAdapter(NULL)
{
  ResetMembers();
  m_features.push_back(FEATURE_CEC);
  m_strComPort = scanResult.m_strLocation;
}

CPeripheralCecAdapter::~CPeripheralCecAdapter(void)
{
  {
    CSingleLock lock(m_critSection);
    CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
    m_bStop = true;
  }

  StopThread(true);
  delete m_queryThread;

  if (m_dll && m_cecAdapter)
  {
    m_dll->CECDestroy(m_cecAdapter);
    m_cecAdapter = NULL;
    delete m_dll;
    m_dll = NULL;
  }
}

void CPeripheralCecAdapter::ResetMembers(void)
{
  if (m_cecAdapter && m_dll)
    m_dll->CECDestroy(m_cecAdapter);
  m_cecAdapter               = NULL;
  delete m_dll;
  m_dll                      = NULL;
  m_bStarted                 = false;
  m_bHasButton               = false;
  m_bIsReady                 = false;
  m_bHasConnectedAudioSystem = false;
  m_strMenuLanguage          = "???";
  m_lastKeypress             = 0;
  m_lastChange               = VOLUME_CHANGE_NONE;
  m_iExitCode                = EXITCODE_QUIT;
  m_bIsMuted                 = false; //! @todo fetch the correct initial value when system audiostatus is implemented in libCEC
  m_bGoingToStandby          = false;
  m_bIsRunning               = false;
  m_bDeviceRemoved           = false;
  m_bActiveSourcePending     = false;
  m_bStandbyPending          = false;
  m_bActiveSourceBeforeStandby = false;
  m_bOnPlayReceived          = false;
  m_bPlaybackPaused          = false;
  m_queryThread              = NULL;

  m_currentButton.iButton    = 0;
  m_currentButton.iDuration  = 0;
  m_standbySent.SetValid(false);
  m_configuration.Clear();
}

void CPeripheralCecAdapter::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnQuit") && m_bIsReady)
  {
    CSingleLock lock(m_critSection);
    m_iExitCode = static_cast<int>(data["exitcode"].asInteger(EXITCODE_QUIT));
    CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
    StopThread(false);
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverDeactivated") && m_bIsReady)
  {
    bool bIgnoreDeactivate(false);
    if (data["shuttingdown"].isBoolean())
    {
      // don't respond to the deactivation if we are just going to suspend/shutdown anyway
      // the tv will not have time to switch on before being told to standby and
      // may not action the standby command.
      bIgnoreDeactivate = data["shuttingdown"].asBoolean();
      if (bIgnoreDeactivate)
        CLog::Log(LOGDEBUG, "%s - ignoring OnScreensaverDeactivated for power action", __FUNCTION__);
    }
    if (m_configuration.bPowerOnScreensaver == 1 && !bIgnoreDeactivate &&
        m_configuration.bActivateSource == 1)
    {
      ActivateSource();
    }
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverActivated") && m_bIsReady)
  {
    // Don't put devices to standby if application is currently playing
    if (!g_application.m_pPlayer->IsPlaying() && m_configuration.bPowerOffScreensaver == 1)
    {
      // only power off when we're the active source
      if (m_cecAdapter->IsLibCECActiveSource())
        StandbyDevices();
    }
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnSleep"))
  {
    // this will also power off devices when we're the active source
    {
      CSingleLock lock(m_critSection);
      m_bGoingToStandby = true;
    }
    StopThread();
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnWake"))
  {
    CLog::Log(LOGDEBUG, "%s - reconnecting to the CEC adapter after standby mode", __FUNCTION__);
    if (ReopenConnection())
    {
      bool bActivate(false);
      {
        CSingleLock lock(m_critSection);
        bActivate = m_bActiveSourceBeforeStandby;
        m_bActiveSourceBeforeStandby = false;
      }
      if (bActivate)
        ActivateSource();
    }
  }
  else if (flag == Player && !strcmp(sender, "xbmc") && !strcmp(message, "OnStop"))
  {
    CSingleLock lock(m_critSection);
    m_preventActivateSourceOnPlay = CDateTime::GetCurrentDateTime();
    m_bOnPlayReceived = false;
  }
  else if (flag == Player && !strcmp(sender, "xbmc") && !strcmp(message, "OnPlay"))
  {
    // activate the source when playback started, and the option is enabled
    bool bActivateSource(false);
    {
      CSingleLock lock(m_critSection);
      bActivateSource = (m_configuration.bActivateSource &&
          !m_bOnPlayReceived &&
          !m_cecAdapter->IsLibCECActiveSource() &&
          (!m_preventActivateSourceOnPlay.IsValid() || CDateTime::GetCurrentDateTime() - m_preventActivateSourceOnPlay > CDateTimeSpan(0, 0, 0, CEC_SUPPRESS_ACTIVATE_SOURCE_AFTER_ON_STOP)));
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
    m_callbacks.CBCecLogMessage           = &CecLogMessage;
    m_callbacks.CBCecKeyPress             = &CecKeyPress;
    m_callbacks.CBCecCommand              = &CecCommand;
    m_callbacks.CBCecConfigurationChanged = &CecConfiguration;
    m_callbacks.CBCecAlert                = &CecAlert;
    m_callbacks.CBCecSourceActivated      = &CecSourceActivated;
    m_configuration.callbackParam         = this;
    m_configuration.callbacks             = &m_callbacks;

    m_dll = new DllLibCEC;
    if (m_dll->Load() && m_dll->IsLoaded())
      m_cecAdapter = m_dll->CECInitialise(&m_configuration);
    else
    {
      // display warning: libCEC could not be loaded
      CLog::Log(LOGERROR, "%s", g_localizeStrings.Get(36017).c_str());
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), g_localizeStrings.Get(36017));
      delete m_dll;
      m_dll = NULL;
      m_features.clear();
      return false;
    }

    if (m_configuration.serverVersion < CEC_LIB_SUPPORTED_VERSION)
    {
      /* unsupported libcec version */
      CLog::Log(LOGERROR, g_localizeStrings.Get(36040).c_str(), m_cecAdapter ? m_configuration.serverVersion : -1, CEC_LIB_SUPPORTED_VERSION);

      // display warning: incompatible libCEC
      std::string strMessage = StringUtils::Format(g_localizeStrings.Get(36040).c_str(), m_cecAdapter ? m_configuration.serverVersion : -1, CEC_LIB_SUPPORTED_VERSION);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), strMessage);
      m_bError = true;
      if (m_cecAdapter)
        m_dll->CECDestroy(m_cecAdapter);
      m_cecAdapter = NULL;

      m_features.clear();
      return false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - using libCEC v%s", __FUNCTION__, m_cecAdapter->VersionToString(m_configuration.serverVersion).c_str());
      SetVersionInfo(m_configuration);
    }

    m_bStarted = true;
    Create();
  }

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralCecAdapter::SetVersionInfo(const libcec_configuration &configuration)
{
  m_strVersionInfo = StringUtils::Format("libCEC %s - firmware v%d", m_cecAdapter->VersionToString(configuration.serverVersion).c_str(), configuration.iFirmwareVersion);

  // append firmware build date
  if (configuration.iFirmwareBuildDate != CEC_FW_BUILD_UNKNOWN)
  {
    CDateTime dt((time_t)configuration.iFirmwareBuildDate);
    m_strVersionInfo += StringUtils::Format(" (%s)", dt.GetAsDBDate().c_str());
  }
}

bool CPeripheralCecAdapter::OpenConnection(void)
{
  bool bIsOpen(false);

  if (!GetSettingBool("enabled"))
  {
    CLog::Log(LOGDEBUG, "%s - CEC adapter is disabled in peripheral settings", __FUNCTION__);
    m_bStarted = false;
    return bIsOpen;
  }
  
  // open the CEC adapter
  CLog::Log(LOGDEBUG, "%s - opening a connection to the CEC adapter: %s", __FUNCTION__, m_strComPort.c_str());

  // scanning the CEC bus takes about 5 seconds, so display a notification to inform users that we're busy
  std::string strMessage = StringUtils::Format(g_localizeStrings.Get(21336).c_str(), g_localizeStrings.Get(36000).c_str());
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), strMessage);

  bool bConnectionFailedDisplayed(false);

  while (!m_bStop && !bIsOpen)
  {
    if ((bIsOpen = m_cecAdapter->Open(m_strComPort.c_str(), 10000)) == false)
    {
      // display warning: couldn't initialise libCEC
      CLog::Log(LOGERROR, "%s - could not opening a connection to the CEC adapter", __FUNCTION__);
      if (!bConnectionFailedDisplayed)
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), g_localizeStrings.Get(36012));
      bConnectionFailedDisplayed = true;

      Sleep(10000);
    }
  }

  if (bIsOpen)
  {
    CLog::Log(LOGDEBUG, "%s - connection to the CEC adapter opened", __FUNCTION__);

    // read the configuration
    libcec_configuration config;
    if (m_cecAdapter->GetCurrentConfiguration(&config))
    {
      // update the local configuration
      CSingleLock lock(m_critSection);
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
    CSingleLock lock(m_critSection);
    m_iExitCode = EXITCODE_QUIT;
    m_bGoingToStandby = false;
    m_bIsRunning = true;
    m_bActiveSourceBeforeStandby = false;
  }

  CAnnouncementManager::GetInstance().AddAnnouncer(this);

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
      Sleep(5);
  }

  m_queryThread->StopThread(true);

  bool bSendStandbyCommands(false);
  {
    CSingleLock lock(m_critSection);
    bSendStandbyCommands = m_iExitCode != EXITCODE_REBOOT &&
                           m_iExitCode != EXITCODE_RESTARTAPP &&
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
        CLog::Log(LOGDEBUG, "%s - sending standby commands", __FUNCTION__);
        m_standbySent = CDateTime::GetCurrentDateTime();
        m_cecAdapter->StandbyDevices();
      }
      else if (m_configuration.bSendInactiveSource == 1)
      {
        CLog::Log(LOGDEBUG, "%s - sending inactive source commands", __FUNCTION__);
        m_cecAdapter->SetInactiveView();
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - XBMC is not the active source, not sending any standby commands", __FUNCTION__);
    }
  }

  m_cecAdapter->Close();

  CLog::Log(LOGDEBUG, "%s - CEC adapter processor thread ended", __FUNCTION__);

  {
    CSingleLock lock(m_critSection);
    m_bStarted = false;
    m_bIsRunning = false;
  }
}

bool CPeripheralCecAdapter::HasAudioControl(void)
{
  CSingleLock lock(m_critSection);
  return m_bHasConnectedAudioSystem;
}

void CPeripheralCecAdapter::SetAudioSystemConnected(bool bSetTo)
{
  CSingleLock lock(m_critSection);
  m_bHasConnectedAudioSystem = bSetTo;
}

void CPeripheralCecAdapter::ProcessVolumeChange(void)
{
  bool bSendRelease(false);
  CecVolumeChange pendingVolumeChange = VOLUME_CHANGE_NONE;
  {
    CSingleLock lock(m_critSection);
    if (!m_volumeChangeQueue.empty())
    {
      /* get the first change from the queue */
      pendingVolumeChange = m_volumeChangeQueue.front();
      m_volumeChangeQueue.pop();

      /* remove all dupe entries */
      while (!m_volumeChangeQueue.empty() && m_volumeChangeQueue.front() == pendingVolumeChange)
        m_volumeChangeQueue.pop();

      /* send another keypress after VOLUME_REFRESH_TIMEOUT ms */
      bool bRefresh(XbmcThreads::SystemClockMillis() - m_lastKeypress > VOLUME_REFRESH_TIMEOUT);

      /* only send the keypress when it hasn't been sent yet */
      if (pendingVolumeChange != m_lastChange)
      {
        m_lastKeypress = XbmcThreads::SystemClockMillis();
        m_lastChange = pendingVolumeChange;
      }
      else if (bRefresh)
      {
        m_lastKeypress = XbmcThreads::SystemClockMillis();
        pendingVolumeChange = m_lastChange;
      }
      else
        pendingVolumeChange = VOLUME_CHANGE_NONE;
    }
    else if (m_lastKeypress > 0 && XbmcThreads::SystemClockMillis() - m_lastKeypress > VOLUME_CHANGE_TIMEOUT)
    {
      /* send a key release */
      m_lastKeypress = 0;
      bSendRelease = true;
      m_lastChange = VOLUME_CHANGE_NONE;
    }
  }

  switch (pendingVolumeChange)
  {
  case VOLUME_CHANGE_UP:
    m_cecAdapter->SendKeypress(CECDEVICE_AUDIOSYSTEM, CEC_USER_CONTROL_CODE_VOLUME_UP, false);
    break;
  case VOLUME_CHANGE_DOWN:
    m_cecAdapter->SendKeypress(CECDEVICE_AUDIOSYSTEM, CEC_USER_CONTROL_CODE_VOLUME_DOWN, false);
    break;
  case VOLUME_CHANGE_MUTE:
    m_cecAdapter->SendKeypress(CECDEVICE_AUDIOSYSTEM, CEC_USER_CONTROL_CODE_MUTE, false);
    {
      CSingleLock lock(m_critSection);
      m_bIsMuted = !m_bIsMuted;
    }
    break;
  case VOLUME_CHANGE_NONE:
    if (bSendRelease)
      m_cecAdapter->SendKeyRelease(CECDEVICE_AUDIOSYSTEM, false);
    break;
  }
}

void CPeripheralCecAdapter::VolumeUp(void)
{
  if (HasAudioControl())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_UP);
  }
}

void CPeripheralCecAdapter::VolumeDown(void)
{
  if (HasAudioControl())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_DOWN);
  }
}

void CPeripheralCecAdapter::ToggleMute(void)
{
  if (HasAudioControl())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_MUTE);
  }
}

bool CPeripheralCecAdapter::IsMuted(void)
{
  if (HasAudioControl())
  {
    CSingleLock lock(m_critSection);
    return m_bIsMuted;
  }
  return false;
}

void CPeripheralCecAdapter::SetMenuLanguage(const char *strLanguage)
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
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SETLANGUAGE, -1, -1, nullptr, strGuiLanguage);
    CLog::Log(LOGDEBUG, "%s - language set to '%s'", __FUNCTION__, strGuiLanguage.c_str());
  }
  else
    CLog::Log(LOGWARNING, "%s - TV menu language set to unknown value '%s'", __FUNCTION__, strLanguage);
}

int CPeripheralCecAdapter::CecCommand(void *cbParam, const cec_command command)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  if (adapter->m_bIsReady)
  {
    switch (command.opcode)
    {
    case CEC_OPCODE_STANDBY:
      /* a device was put in standby mode */
      if (command.initiator == CECDEVICE_TV &&
          (adapter->m_configuration.bPowerOffOnStandby == 1 || adapter->m_configuration.bShutdownOnStandby == 1) &&
          (!adapter->m_standbySent.IsValid() || CDateTime::GetCurrentDateTime() - adapter->m_standbySent > CDateTimeSpan(0, 0, 0, SCREENSAVER_TIMEOUT)))
      {
        adapter->m_bStarted = false;
        if (adapter->m_configuration.bPowerOffOnStandby == 1)
          g_application.ExecuteXBMCAction("Suspend");
        else if (adapter->m_configuration.bShutdownOnStandby == 1)
          g_application.ExecuteXBMCAction("Shutdown");
      }
      break;
    case CEC_OPCODE_SET_MENU_LANGUAGE:
      if (adapter->m_configuration.bUseTVMenuLanguage == 1 && command.initiator == CECDEVICE_TV && command.parameters.size == 3)
      {
        char strNewLanguage[4];
        for (int iPtr = 0; iPtr < 3; iPtr++)
          strNewLanguage[iPtr] = command.parameters[iPtr];
        strNewLanguage[3] = 0;
        adapter->SetMenuLanguage(strNewLanguage);
      }
      break;
    case CEC_OPCODE_DECK_CONTROL:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1 &&
          command.parameters[0] == CEC_DECK_CONTROL_MODE_STOP)
      {
        cec_keypress key;
        key.duration = 500;
        key.keycode = CEC_USER_CONTROL_CODE_STOP;
        adapter->PushCecKeypress(key);
      }
      break;
    case CEC_OPCODE_PLAY:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1)
      {
        if (command.parameters[0] == CEC_PLAY_MODE_PLAY_FORWARD)
        {
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PLAY;
          adapter->PushCecKeypress(key);
        }
        else if (command.parameters[0] == CEC_PLAY_MODE_PLAY_STILL)
        {
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PAUSE;
          adapter->PushCecKeypress(key);
        }
      }
      break;
    default:
      break;
    }
  }
  return 1;
}

int CPeripheralCecAdapter::CecConfiguration(void *cbParam, const libcec_configuration config)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  CSingleLock lock(adapter->m_critSection);
  adapter->SetConfigurationFromLibCEC(config);
  return 1;
}

int CPeripheralCecAdapter::CecAlert(void *cbParam, const libcec_alert alert, const libcec_parameter data)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

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
    std::string strLog(g_localizeStrings.Get(iAlertString));
    if (data.paramType == CEC_PARAMETER_TYPE_STRING && data.paramData)
      strLog += StringUtils::Format(" - %s", (const char *)data.paramData);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), strLog);
  }

  if (bReopenConnection)
  {
    // Reopen the connection asynchronously. Otherwise a deadlock may occure.
    // Reconnect means destruction and recreation of our libcec instance, but libcec
    // calls this callback function synchronously and must not be destroyed meanwhile.
    adapter->ReopenConnection(true);
  }

  return 1;
}

int CPeripheralCecAdapter::CecKeyPress(void *cbParam, const cec_keypress key)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  adapter->PushCecKeypress(key);
  return 1;
}

void CPeripheralCecAdapter::GetNextKey(void)
{
  CSingleLock lock(m_critSection);
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

void CPeripheralCecAdapter::PushCecKeypress(const CecButtonPress &key)
{
  CLog::Log(LOGDEBUG, "%s - received key %2x duration %d", __FUNCTION__, key.iButton, key.iDuration);

  CSingleLock lock(m_critSection);
  if (key.iDuration > 0)
  {
    if (m_currentButton.iButton == key.iButton && m_currentButton.iDuration == 0)
    {
      // update the duration
      if (m_bHasButton)
        m_currentButton.iDuration = key.iDuration;
      // ignore this one, since it's already been handled by xbmc
      return;
    }
    // if we received a keypress with a duration set, try to find the same one without a duration set, and replace it
    for (std::vector<CecButtonPress>::reverse_iterator it = m_buttonQueue.rbegin(); it != m_buttonQueue.rend(); ++it)
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

void CPeripheralCecAdapter::PushCecKeypress(const cec_keypress &key)
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
    xbmcKey.iButton = XINPUT_IR_REMOTE_MENU;
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
    xbmcKey.iButton = XINPUT_IR_REMOTE_TELETEXT;
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
  CSingleLock lock(m_critSection);
  if (!m_bHasButton)
    GetNextKey();

  return m_bHasButton ? m_currentButton.iButton : 0;
}

unsigned int CPeripheralCecAdapter::GetHoldTime(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bHasButton)
    GetNextKey();

  return m_bHasButton ? m_currentButton.iDuration : 0;
}

void CPeripheralCecAdapter::ResetButton(void)
{
  CSingleLock lock(m_critSection);
  m_bHasButton = false;

  // wait for the key release if the duration isn't 0
  if (m_currentButton.iDuration > 0)
  {
    m_currentButton.iButton   = 0;
    m_currentButton.iDuration = 0;
  }
}

void CPeripheralCecAdapter::OnSettingChanged(const std::string &strChangedSetting)
{
  if (StringUtils::EqualsNoCase(strChangedSetting, "enabled"))
  {
    bool bEnabled(GetSettingBool("enabled"));
    if (!bEnabled && IsRunning())
    {
      CLog::Log(LOGDEBUG, "%s - closing the CEC connection", __FUNCTION__);
      StopThread(true);
    }
    else if (bEnabled && !IsRunning())
    {
      CLog::Log(LOGDEBUG, "%s - starting the CEC connection", __FUNCTION__);
      SetConfigurationFromSettings();
      InitialiseFeature(FEATURE_CEC);
    }
  }
  else if (IsRunning())
  {
    if (m_queryThread->IsRunning())
    {
      CLog::Log(LOGDEBUG, "%s - sending the updated configuration to libCEC", __FUNCTION__);
      SetConfigurationFromSettings();
      m_queryThread->UpdateConfiguration(&m_configuration);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - restarting the CEC connection", __FUNCTION__);
    SetConfigurationFromSettings();
    InitialiseFeature(FEATURE_CEC);
  }
}

void CPeripheralCecAdapter::CecSourceActivated(void *cbParam, const CEC::cec_logical_address address, const uint8_t activated)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return;

  // wake up the screensaver, so the user doesn't switch to a black screen
  if (activated == 1)
    g_application.WakeUpScreenSaverAndDPMS();

  if (adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") != LOCALISED_ID_NONE)
  {
    bool bShowingSlideshow = (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW);
    CGUIWindowSlideShow *pSlideShow = bShowingSlideshow ? (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW) : NULL;
    bool bPlayingAndDeactivated = activated == 0 && (
        (pSlideShow && pSlideShow->IsPlaying()) || !g_application.m_pPlayer->IsPausedPlayback());
    bool bPausedAndActivated = activated == 1 && adapter->m_bPlaybackPaused && (
        (pSlideShow && pSlideShow->IsPaused()) || g_application.m_pPlayer->IsPausedPlayback());
    if (bPlayingAndDeactivated)
      adapter->m_bPlaybackPaused = true;
    else if (bPausedAndActivated)
      adapter->m_bPlaybackPaused = false;

    if ((bPlayingAndDeactivated || bPausedAndActivated)
      && adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") == LOCALISED_ID_PAUSE)
    {
      if (pSlideShow)
        // pause/resume slideshow
        pSlideShow->OnAction(CAction(ACTION_PAUSE));
      else
        // pause/resume player
        CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE);
    }
    else if (bPlayingAndDeactivated
      && adapter->GetSettingInt("pause_or_stop_playback_on_deactivate") == LOCALISED_ID_STOP)
    {
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_STOP));
      else
        CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    }
  }
}

int CPeripheralCecAdapter::CecLogMessage(void *cbParam, const cec_log_message message)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  int iLevel = -1;
  switch (message.level)
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

  if (iLevel >= CEC_LOG_NOTICE || (iLevel >= 0 && CLog::IsLogLevelLogged(LOGDEBUG) && g_advancedSettings.CanLogComponent(LOGCEC)))
    CLog::Log(iLevel, "%s - %s", __FUNCTION__, message.message);

  return 1;
}

void CPeripheralCecAdapter::SetConfigurationFromLibCEC(const CEC::libcec_configuration &config)
{
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
  bChanged |= SetSetting("connected_device", config.baseDevice == CECDEVICE_AUDIOSYSTEM ? LOCALISED_ID_AVR : LOCALISED_ID_TV);

  // set the HDMI port number
  m_configuration.iHDMIPort = config.iHDMIPort;
  bChanged |= SetSetting("cec_hdmi_port", config.iHDMIPort);

  // set the physical address, when baseDevice or iHDMIPort are not set
  std::string strPhysicalAddress("0");
  if (!bPAAutoDetected && (m_configuration.baseDevice == CECDEVICE_UNKNOWN ||
      m_configuration.iHDMIPort < CEC_MIN_HDMI_PORTNUMBER ||
      m_configuration.iHDMIPort > CEC_MAX_HDMI_PORTNUMBER))
  {
    m_configuration.iPhysicalAddress = config.iPhysicalAddress;
    strPhysicalAddress = StringUtils::Format("%x", config.iPhysicalAddress);
  }
  bChanged |= SetSetting("physical_address", strPhysicalAddress);

  // set the devices to wake when starting
  m_configuration.wakeDevices = config.wakeDevices;
  bChanged |= WriteLogicalAddresses(config.wakeDevices, "wake_devices", "wake_devices_advanced");

  // set the devices to power off when stopping
  m_configuration.powerOffDevices = config.powerOffDevices;
  bChanged |= WriteLogicalAddresses(config.powerOffDevices, "standby_devices", "standby_devices_advanced");

  // set the boolean settings
  m_configuration.bUseTVMenuLanguage = config.bUseTVMenuLanguage;
  bChanged |= SetSetting("use_tv_menu_language", m_configuration.bUseTVMenuLanguage == 1);

  m_configuration.bActivateSource = config.bActivateSource;
  bChanged |= SetSetting("activate_source", m_configuration.bActivateSource == 1);

  m_configuration.bPowerOffScreensaver = config.bPowerOffScreensaver;
  bChanged |= SetSetting("cec_standby_screensaver", m_configuration.bPowerOffScreensaver == 1);

  m_configuration.bPowerOnScreensaver = config.bPowerOnScreensaver;
  bChanged |= SetSetting("cec_wake_screensaver", m_configuration.bPowerOnScreensaver == 1);

  m_configuration.bPowerOffOnStandby = config.bPowerOffOnStandby;

  m_configuration.bSendInactiveSource = config.bSendInactiveSource;
  bChanged |= SetSetting("send_inactive_source", m_configuration.bSendInactiveSource == 1);

  m_configuration.iFirmwareVersion = config.iFirmwareVersion;
  m_configuration.bShutdownOnStandby = config.bShutdownOnStandby;

  memcpy(m_configuration.strDeviceLanguage, config.strDeviceLanguage, 3);
  m_configuration.iFirmwareBuildDate = config.iFirmwareBuildDate;

  SetVersionInfo(m_configuration);

  bChanged |= SetSetting("standby_pc_on_tv_standby",
             m_configuration.bPowerOffOnStandby == 1 ? 13011 :
             m_configuration.bShutdownOnStandby == 1 ? 13005 : 36028);

  if (bChanged)
    CLog::Log(LOGDEBUG, "SetConfigurationFromLibCEC - settings updated by libCEC");
}

void CPeripheralCecAdapter::SetConfigurationFromSettings(void)
{
  // client version matches the version of libCEC that we originally used the API from
  m_configuration.clientVersion = LIBCEC_VERSION_TO_UINT(3, 0, 0);

  // device name 'XBMC'
  snprintf(m_configuration.strDeviceName, 13, "%s", GetSettingString("device_name").c_str());

  // set the primary device type
  m_configuration.deviceTypes.Clear();
  int iDeviceType = GetSettingInt("device_type");
  if (iDeviceType != (int)CEC_DEVICE_TYPE_RECORDING_DEVICE &&
      iDeviceType != (int)CEC_DEVICE_TYPE_PLAYBACK_DEVICE &&
      iDeviceType != (int)CEC_DEVICE_TYPE_TUNER)
    iDeviceType = (int)CEC_DEVICE_TYPE_RECORDING_DEVICE;
  m_configuration.deviceTypes.Add((cec_device_type)iDeviceType);

  // always try to autodetect the address.
  // when the firmware supports this, it will override the physical address, connected device and hdmi port settings
  m_configuration.bAutodetectAddress = CEC_DEFAULT_SETTING_AUTODETECT_ADDRESS;

  // set the physical address
  // when set, it will override the connected device and hdmi port settings
  std::string strPhysicalAddress = GetSettingString("physical_address");
  int iPhysicalAddress;
  if (sscanf(strPhysicalAddress.c_str(), "%x", &iPhysicalAddress) &&
      iPhysicalAddress >= CEC_PHYSICAL_ADDRESS_TV &&
      iPhysicalAddress <= CEC_MAX_PHYSICAL_ADDRESS)
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
  if (iHDMIPort >= CEC_MIN_HDMI_PORTNUMBER &&
      iHDMIPort <= CEC_MAX_HDMI_PORTNUMBER)
    m_configuration.iHDMIPort = iHDMIPort;

  // set the tv vendor override
  int iVendor = GetSettingInt("tv_vendor");
  if (iVendor >= CEC_MIN_VENDORID &&
      iVendor <= CEC_MAX_VENDORID)
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
  m_configuration.bUseTVMenuLanguage   = GetSettingBool("use_tv_menu_language") ? 1 : 0;
  m_configuration.bActivateSource      = GetSettingBool("activate_source") ? 1 : 0;
  m_configuration.bPowerOffScreensaver = GetSettingBool("cec_standby_screensaver") ? 1 : 0;
  m_configuration.bPowerOnScreensaver  = GetSettingBool("cec_wake_screensaver") ? 1 : 0;
  m_configuration.bSendInactiveSource  = GetSettingBool("send_inactive_source") ? 1 : 0;

  // read the mutually exclusive boolean settings
  int iStandbyAction(GetSettingInt("standby_pc_on_tv_standby"));
  m_configuration.bPowerOffOnStandby = iStandbyAction == 13011 ? 1 : 0;
  m_configuration.bShutdownOnStandby = iStandbyAction == 13005 ? 1 : 0;

#if defined(CEC_DOUBLE_TAP_TIMEOUT_MS_OLD)
  // double tap prevention timeout in ms. libCEC uses 50ms units for this in 2.2.0, so divide by 50
  m_configuration.iDoubleTapTimeout50Ms = GetSettingInt("double_tap_timeout_ms") / 50;
#else
  // backwards compatibility. will be removed once the next major release of libCEC is out
  m_configuration.iDoubleTapTimeoutMs = GetSettingInt("double_tap_timeout_ms");
#endif

  if (GetSettingBool("pause_playback_on_deactivate"))
  {
    SetSetting("pause_or_stop_playback_on_deactivate", 36045);
    SetSetting("pause_playback_on_deactivate", false);
  }
}

void CPeripheralCecAdapter::ReadLogicalAddresses(const std::string &strString, cec_logical_addresses &addresses)
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

void CPeripheralCecAdapter::ReadLogicalAddresses(int iLocalisedId, cec_logical_addresses &addresses)
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

bool CPeripheralCecAdapter::WriteLogicalAddresses(const cec_logical_addresses& addresses, const std::string& strSettingName, const std::string& strAdvancedSettingName)
{
  bool bChanged(false);

  // only update the advanced setting if it was set by the user
  if (!GetSettingString(strAdvancedSettingName).empty())
  {
    std::string strPowerOffDevices;
    for (unsigned int iPtr = CECDEVICE_TV; iPtr <= CECDEVICE_BROADCAST; iPtr++)
      if (addresses[iPtr])
        strPowerOffDevices += StringUtils::Format(" %X", iPtr);
    StringUtils::Trim(strPowerOffDevices);
    bChanged = SetSetting(strAdvancedSettingName, strPowerOffDevices);
  }

  int iSettingPowerOffDevices = LOCALISED_ID_NONE;
  if (addresses[CECDEVICE_TV] && addresses[CECDEVICE_AUDIOSYSTEM])
    iSettingPowerOffDevices = LOCALISED_ID_TV_AVR;
  else if (addresses[CECDEVICE_TV])
    iSettingPowerOffDevices = LOCALISED_ID_TV;
  else if (addresses[CECDEVICE_AUDIOSYSTEM])
    iSettingPowerOffDevices = LOCALISED_ID_AVR;
  return SetSetting(strSettingName, iSettingPowerOffDevices) || bChanged;
}

CPeripheralCecAdapterUpdateThread::CPeripheralCecAdapterUpdateThread(CPeripheralCecAdapter *adapter, libcec_configuration *configuration) :
    CThread("CECAdapterUpdate"),
    m_adapter(adapter),
    m_configuration(*configuration),
    m_bNextConfigurationScheduled(false),
    m_bIsUpdating(true)
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

bool CPeripheralCecAdapterUpdateThread::UpdateConfiguration(libcec_configuration *configuration)
{
  CSingleLock lock(m_critSection);
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
  cec_logical_address waitFor = (m_configuration.bActivateSource == 1) ?
      CECDEVICE_TV :
      m_configuration.wakeDevices.primary;

  cec_power_status powerStatus(CEC_POWER_STATUS_UNKNOWN);
  bool bContinue(true);
  while (bContinue && !m_adapter->m_bStop && !m_bStop && powerStatus != CEC_POWER_STATUS_ON)
  {
    powerStatus = m_adapter->m_cecAdapter->GetDevicePowerStatus(waitFor);
    if (powerStatus != CEC_POWER_STATUS_ON)
      bContinue = !m_event.WaitMSec(1000);
  }

  return powerStatus == CEC_POWER_STATUS_ON;
}

void CPeripheralCecAdapterUpdateThread::UpdateMenuLanguage(void)
{
  // request the menu language of the TV
  if (m_configuration.bUseTVMenuLanguage == 1)
  {
    CLog::Log(LOGDEBUG, "%s - requesting the menu language of the TV", __FUNCTION__);
    cec_menu_language language;
    if (m_adapter->m_cecAdapter->GetDeviceMenuLanguage(CECDEVICE_TV, &language))
      m_adapter->SetMenuLanguage(language.language);
    else
      CLog::Log(LOGDEBUG, "%s - unknown menu language", __FUNCTION__);
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - using TV menu language is disabled", __FUNCTION__);
  }
}

std::string CPeripheralCecAdapterUpdateThread::UpdateAudioSystemStatus(void)
{
  std::string strAmpName;

  /* disable the mute setting when an amp is found, because the amp handles the mute setting and
       set PCM output to 100% */
  if (m_adapter->m_cecAdapter->IsActiveDeviceType(CEC_DEVICE_TYPE_AUDIO_SYSTEM))
  {
    // request the OSD name of the amp
    cec_osd_name ampName = m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_AUDIOSYSTEM);
    CLog::Log(LOGDEBUG, "%s - CEC capable amplifier found (%s). volume will be controlled on the amp", __FUNCTION__, ampName.name);
    strAmpName += StringUtils::Format("%s", ampName.name);

    // set amp present
    m_adapter->SetAudioSystemConnected(true);
    g_application.SetMute(false);
    g_application.SetVolume(VOLUME_MAXIMUM, false);
  }
  else
  {
    // set amp present
    CLog::Log(LOGDEBUG, "%s - no CEC capable amplifier found", __FUNCTION__);
    m_adapter->SetAudioSystemConnected(false);
  }

  return strAmpName;
}

bool CPeripheralCecAdapterUpdateThread::SetInitialConfiguration(void)
{
  // the option to make XBMC the active source is set
  if (m_configuration.bActivateSource == 1)
    m_adapter->m_cecAdapter->SetActiveSource();

  // devices to wake are set
  cec_logical_addresses tvOnly;
  tvOnly.Clear(); tvOnly.Set(CECDEVICE_TV);
  if (!m_configuration.wakeDevices.IsEmpty() && (m_configuration.wakeDevices != tvOnly || m_configuration.bActivateSource == 0))
    m_adapter->m_cecAdapter->PowerOnDevices(CECDEVICE_BROADCAST);

  // wait until devices are powered up
  if (!WaitReady())
    return false;

  UpdateMenuLanguage();

  // request the OSD name of the TV
  std::string strNotification;
  cec_osd_name tvName = m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_TV);
  strNotification = StringUtils::Format("%s: %s", g_localizeStrings.Get(36016).c_str(), tvName.name);

  std::string strAmpName = UpdateAudioSystemStatus();
  if (!strAmpName.empty())
    strNotification += StringUtils::Format("- %s", strAmpName.c_str());

  m_adapter->m_bIsReady = true;

  // and let the gui know that we're done
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), strNotification);

  CSingleLock lock(m_critSection);
  m_bIsUpdating = false;
  return true;
}

bool CPeripheralCecAdapter::IsRunning(void) const
{
  CSingleLock lock(m_critSection);
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
    if (bUpdate || m_event.WaitMSec(500))
    {
      if (m_bStop)
        return;
      // set the new configuration
      libcec_configuration configuration;
      {
        CSingleLock lock(m_critSection);
        configuration = m_configuration;
        m_bIsUpdating = false;
      }

      CLog::Log(LOGDEBUG, "%s - updating the configuration", __FUNCTION__);
      bool bConfigSet(m_adapter->m_cecAdapter->SetConfiguration(&configuration));
      // display message: config updated / failed to update
      if (!bConfigSet)
        CLog::Log(LOGERROR, "%s - libCEC couldn't set the new configuration", __FUNCTION__);
      else
      {
        UpdateMenuLanguage();
        UpdateAudioSystemStatus();
      }

      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), g_localizeStrings.Get(bConfigSet ? 36023 : 36024));

      {
        CSingleLock lock(m_critSection);
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
  CSingleLock lock(m_critSection);
  m_bDeviceRemoved = true;
}

namespace PERIPHERALS
{

class CPeripheralCecAdapterReopenJob : public CJob
{
public:
  CPeripheralCecAdapterReopenJob(CPeripheralCecAdapter *adapter)
    : m_adapter(adapter) {}
  virtual ~CPeripheralCecAdapterReopenJob() {}

  bool DoWork(void)
  {
    return m_adapter->ReopenConnection(false);
  }

private:
  CPeripheralCecAdapter *m_adapter;
};

};

bool CPeripheralCecAdapter::ReopenConnection(bool bAsync /* = false */)
{
  if (bAsync)
  {
    CJobManager::GetInstance().AddJob(new CPeripheralCecAdapterReopenJob(this), nullptr, CJob::PRIORITY_NORMAL);
    return true;
  }

  // stop running thread
  {
    CSingleLock lock(m_critSection);
    m_iExitCode = EXITCODE_RESTARTAPP;
    CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
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
  CSingleLock lock(m_critSection);
  m_bActiveSourcePending = true;
}

void CPeripheralCecAdapter::ProcessActivateSource(void)
{
  bool bActivate(false);

  {
    CSingleLock lock(m_critSection);
    bActivate = m_bActiveSourcePending;
    m_bActiveSourcePending = false;
  }

  if (bActivate)
    m_cecAdapter->SetActiveSource();
}

void CPeripheralCecAdapter::StandbyDevices(void)
{
  CSingleLock lock(m_critSection);
  m_bStandbyPending = true;
}

void CPeripheralCecAdapter::ProcessStandbyDevices(void)
{
  bool bStandby(false);

  {
    CSingleLock lock(m_critSection);
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
    else if (m_configuration.bSendInactiveSource == 1)
    {
      CLog::Log(LOGDEBUG, "%s - sending inactive source commands", __FUNCTION__);
      m_cecAdapter->SetInactiveView();
    }
  }
}

bool CPeripheralCecAdapter::ToggleDeviceState(CecStateChange mode /*= STATE_SWITCH_TOGGLE */, bool forceType /*= false */)
{
  if (!IsRunning())
    return false;
  if (m_cecAdapter->IsLibCECActiveSource() && (mode == STATE_SWITCH_TOGGLE || mode == STATE_STANDBY))
  {
    CLog::Log(LOGDEBUG, "%s - putting CEC device on standby...", __FUNCTION__);
    StandbyDevices();
    return false;
  }
  else if (mode == STATE_SWITCH_TOGGLE || mode == STATE_ACTIVATE_SOURCE)
  {
    CLog::Log(LOGDEBUG, "%s - waking up CEC device...", __FUNCTION__);
    ActivateSource();
    return true;
  }

  return false;
}

#endif
