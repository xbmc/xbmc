/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#if defined(HAVE_LIBCEC)
#include "PeripheralCecAdapter.h"
#include "input/XBIRRemote.h"
#include "Application.h"
#include "DynamicDll.h"
#include "threads/SingleLock.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/PeripheralBus.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <libcec/cec.h>

using namespace PERIPHERALS;
using namespace ANNOUNCEMENT;
using namespace CEC;

#define CEC_LIB_SUPPORTED_VERSION 0x1500

/* time in seconds to ignore standby commands from devices after the screensaver has been activated */
#define SCREENSAVER_TIMEOUT       10
#define VOLUME_CHANGE_TIMEOUT     250
#define VOLUME_REFRESH_TIMEOUT    100

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

CPeripheralCecAdapter::CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheralHID(type, busType, strLocation, strDeviceName, iVendorId, iProductId),
  CThread("CEC Adapter"),
  m_bStarted(false),
  m_bHasButton(false),
  m_bIsReady(false),
  m_bHasConnectedAudioSystem(false),
  m_strMenuLanguage("???"),
  m_lastKeypress(0),
  m_lastChange(VOLUME_CHANGE_NONE)
{
  m_button.iButton = 0;
  m_button.iDuration = 0;
  m_screensaverLastActivated.SetValid(false);

  m_configuration.Clear();
  m_features.push_back(FEATURE_CEC);
}

CPeripheralCecAdapter::~CPeripheralCecAdapter(void)
{
  CAnnouncementManager::RemoveAnnouncer(this);

  m_bStop = true;
  StopThread(true);

  if (m_dll && m_cecAdapter)
  {
    m_dll->CECDestroy(m_cecAdapter);
    m_cecAdapter = NULL;
    delete m_dll;
    m_dll = NULL;
  }
}

void CPeripheralCecAdapter::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnQuit") && m_bIsReady)
  {
    // only send power off and inactive source command when we're currently active
    if (m_cecAdapter->IsLibCECActiveSource())
    {
      // if there are any devices to power off set, power them off
      if (!m_configuration.powerOffDevices.IsEmpty())
        m_cecAdapter->StandbyDevices(CECDEVICE_BROADCAST);
      // send an inactive source command otherwise
      else
        m_cecAdapter->SetInactiveView();
    }
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverDeactivated") && m_bIsReady)
  {
    if (m_configuration.bPowerOffScreensaver == 1)
    {
      // power off/on on screensaver is set, and devices to wake are set
      if (!m_configuration.wakeDevices.IsEmpty())
        m_cecAdapter->PowerOnDevices(CECDEVICE_BROADCAST);

      // the option to make XBMC the active source is set
      if (m_configuration.bActivateSource == 1)
        m_cecAdapter->SetActiveSource();
    }
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverActivated") && m_bIsReady)
  {
    // Don't put devices to standby if application is currently playing
    if ((!g_application.IsPlaying() || g_application.IsPaused()) && m_configuration.bPowerOffScreensaver == 1)
    {
      m_screensaverLastActivated = CDateTime::GetCurrentDateTime();
      // only power off when we're the active source
      if (m_cecAdapter->IsLibCECActiveSource())
        m_cecAdapter->StandbyDevices(CECDEVICE_BROADCAST);
    }
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnSleep"))
  {
    // this will also power off devices when we're the active source
    CSingleLock lock(m_critSection);
    m_bStop = true;
    WaitForThreadExit(0);
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnWake"))
  {
    // reconnect to the device
    CSingleLock lock(m_critSection);
    CLog::Log(LOGDEBUG, "%s - reconnecting to the CEC adapter after standby mode", __FUNCTION__);

    // close the previous connection
    m_cecAdapter->Close();

    // and open a new one
    m_bStop = false;
    Create();
  }
}

bool CPeripheralCecAdapter::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_CEC && !m_bStarted)
  {
    SetConfigurationFromSettings();
    m_callbacks.CBCecLogMessage           = &CecLogMessage;
    m_callbacks.CBCecKeyPress             = &CecKeyPress;
    m_callbacks.CBCecCommand              = &CecCommand;
    m_callbacks.CBCecConfigurationChanged = &CecConfiguration;
    m_configuration.callbackParam         = this;
    m_configuration.callbacks             = &m_callbacks;

    m_dll = new DllLibCEC;
    if (m_dll->Load() && m_dll->IsLoaded())
      m_cecAdapter = m_dll->CECInitialise(&m_configuration);
    else
      return false;

    if (m_configuration.serverVersion < CEC_LIB_SUPPORTED_VERSION)
    {
      /* unsupported libcec version */
      CLog::Log(LOGERROR, g_localizeStrings.Get(36013).c_str(), CEC_LIB_SUPPORTED_VERSION, m_cecAdapter ? m_configuration.serverVersion : -1);

      CStdString strMessage;
      strMessage.Format(g_localizeStrings.Get(36013).c_str(), CEC_LIB_SUPPORTED_VERSION, m_cecAdapter ? m_configuration.serverVersion : -1);
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
      CLog::Log(LOGDEBUG, "%s - using libCEC v%s", __FUNCTION__, m_cecAdapter->ToString((cec_server_version)m_configuration.serverVersion));
    }

    m_bStarted = true;
    Create();
  }

  return CPeripheral::InitialiseFeature(feature);
}

CStdString CPeripheralCecAdapter::GetComPort(void)
{
  CStdString strPort = GetSettingString("port");
  if (strPort.IsEmpty())
  {
    strPort = m_strFileLocation;
    cec_adapter deviceList[10];
    TranslateComPort(strPort);
    uint8_t iFound = m_cecAdapter->FindAdapters(deviceList, 10, strPort.c_str());

    if (iFound <= 0)
    {
      CLog::Log(LOGWARNING, "%s - no CEC adapters found on %s", __FUNCTION__, strPort.c_str());
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), g_localizeStrings.Get(36011));
      strPort = "";
    }
    else
    {
      cec_adapter *dev = &deviceList[0];
      if (iFound > 1)
        CLog::Log(LOGDEBUG, "%s - multiple com ports found for device. taking the first one", __FUNCTION__);
      else
        CLog::Log(LOGDEBUG, "%s - autodetect com port '%s'", __FUNCTION__, dev->comm);

      strPort = dev->comm;
    }
  }

  return strPort;
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
  
  CStdString strPort = GetComPort();
  if (strPort.empty())
    return bIsOpen;

  // open the CEC adapter
  CLog::Log(LOGDEBUG, "%s - opening a connection to the CEC adapter: %s", __FUNCTION__, strPort.c_str());

  // scanning the CEC bus takes about 5 seconds, so display a notification to inform users that we're busy
  CStdString strMessage;
  strMessage.Format(g_localizeStrings.Get(21336), g_localizeStrings.Get(36000));
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), strMessage);

  bool bConnectionFailedDisplayed(false);

  while (!m_bStop && !bIsOpen)
  {
    if ((bIsOpen = m_cecAdapter->Open(strPort.c_str(), 10000)) == false)
    {
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

    if (!m_configuration.wakeDevices.IsEmpty())
      m_cecAdapter->PowerOnDevices(CECDEVICE_BROADCAST);

    if (m_configuration.bActivateSource == 1)
      m_cecAdapter->SetActiveSource();
  }

  return bIsOpen;
}

void CPeripheralCecAdapter::Process(void)
{
  if (!OpenConnection())
    return;

  CAnnouncementManager::AddAnnouncer(this);

  m_queryThread = new CPeripheralCecAdapterUpdateThread(this, &m_configuration);
  m_queryThread->Create(false);

  while (!m_bStop)
  {
    if (!m_bStop)
      ProcessVolumeChange();

    if (!m_bStop)
      Sleep(5);
  }

  delete m_queryThread;

  if (m_cecAdapter->IsLibCECActiveSource())
  {
    if (m_configuration.bPowerOffOnStandby == 1)
      m_cecAdapter->StandbyDevices();
    else
      m_cecAdapter->SetInactiveView();
  }

  m_cecAdapter->Close();

  CLog::Log(LOGDEBUG, "%s - CEC adapter processor thread ended", __FUNCTION__);
  m_bStarted = false;
}

bool CPeripheralCecAdapter::HasConnectedAudioSystem(void)
{
  CSingleLock lock(m_critSection);
  return m_bHasConnectedAudioSystem;
}

void CPeripheralCecAdapter::SetAudioSystemConnected(bool bSetTo)
{
  CSingleLock lock(m_critSection);
  m_bHasConnectedAudioSystem = bSetTo;
}

void CPeripheralCecAdapter::ScheduleVolumeUp(void)
{
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_UP);
  }
  Sleep(5);
}

void CPeripheralCecAdapter::ScheduleVolumeDown(void)
{
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_DOWN);
  }
  Sleep(5);
}

void CPeripheralCecAdapter::ScheduleMute(void)
{
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_MUTE);
  }
  Sleep(5);
}

void CPeripheralCecAdapter::ProcessVolumeChange(void)
{
  bool bSendRelease(false);
  CecVolumeChange pendingVolumeChange = VOLUME_CHANGE_NONE;
  {
    CSingleLock lock(m_critSection);
    if (m_volumeChangeQueue.size() > 0)
    {
      /* get the first change from the queue */
      pendingVolumeChange = m_volumeChangeQueue.front();
      m_volumeChangeQueue.pop();

      /* remove all dupe entries */
      while (m_volumeChangeQueue.size() > 0 && m_volumeChangeQueue.front() == pendingVolumeChange)
        m_volumeChangeQueue.pop();

      /* send another keypress after VOLUME_REFRESH_TIMEOUT ms */
      bool bRefresh(m_lastKeypress + VOLUME_REFRESH_TIMEOUT < XbmcThreads::SystemClockMillis());

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
    else if (m_lastKeypress > 0 && m_lastKeypress + VOLUME_CHANGE_TIMEOUT < XbmcThreads::SystemClockMillis())
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
    break;
  case VOLUME_CHANGE_NONE:
    if (bSendRelease)
      m_cecAdapter->SendKeyRelease(CECDEVICE_AUDIOSYSTEM, false);
    break;
  }
}

void CPeripheralCecAdapter::VolumeUp(void)
{
  if (HasConnectedAudioSystem())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_UP);
  }
}

void CPeripheralCecAdapter::VolumeDown(void)
{
  if (HasConnectedAudioSystem())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_DOWN);
  }
}

void CPeripheralCecAdapter::Mute(void)
{
  if (HasConnectedAudioSystem())
  {
    CSingleLock lock(m_critSection);
    m_volumeChangeQueue.push(VOLUME_CHANGE_MUTE);
  }
}

void CPeripheralCecAdapter::SetMenuLanguage(const char *strLanguage)
{
  if (m_strMenuLanguage.Equals(strLanguage))
    return;

  CStdString strGuiLanguage;

  if (!strcmp(strLanguage, "bul"))
    strGuiLanguage = "Bulgarian";
  else if (!strcmp(strLanguage, "hrv"))
    strGuiLanguage = "Croatian";
  else if (!strcmp(strLanguage, "cze"))
    strGuiLanguage = "Czech";
  else if (!strcmp(strLanguage, "dan"))
    strGuiLanguage = "Danish";
  else if (!strcmp(strLanguage, "dut"))
    strGuiLanguage = "Dutch";
  else if (!strcmp(strLanguage, "eng"))
    strGuiLanguage = "English";
  else if (!strcmp(strLanguage, "fin"))
    strGuiLanguage = "Finnish";
  else if (!strcmp(strLanguage, "fre"))
    strGuiLanguage = "French";
  else if (!strcmp(strLanguage, "ger"))
    strGuiLanguage = "German";
  else if (!strcmp(strLanguage, "gre"))
    strGuiLanguage = "Greek";
  else if (!strcmp(strLanguage, "hun"))
    strGuiLanguage = "Hungarian";
  else if (!strcmp(strLanguage, "ita"))
    strGuiLanguage = "Italian";
  else if (!strcmp(strLanguage, "nor"))
    strGuiLanguage = "Norwegian";
  else if (!strcmp(strLanguage, "pol"))
    strGuiLanguage = "Polish";
  else if (!strcmp(strLanguage, "por"))
    strGuiLanguage = "Portuguese";
  else if (!strcmp(strLanguage, "rum"))
    strGuiLanguage = "Romanian";
  else if (!strcmp(strLanguage, "rus"))
    strGuiLanguage = "Russian";
  else if (!strcmp(strLanguage, "srp"))
    strGuiLanguage = "Serbian";
  else if (!strcmp(strLanguage, "slo"))
    strGuiLanguage = "Slovenian";
  else if (!strcmp(strLanguage, "spa"))
    strGuiLanguage = "Spanish";
  else if (!strcmp(strLanguage, "swe"))
    strGuiLanguage = "Swedish";
  else if (!strcmp(strLanguage, "tur"))
    strGuiLanguage = "Turkish";

  if (!strGuiLanguage.IsEmpty())
  {
    g_application.getApplicationMessenger().SetGUILanguage(strGuiLanguage);
    CLog::Log(LOGDEBUG, "%s - language set to '%s'", __FUNCTION__, strGuiLanguage.c_str());
  }
  else
    CLog::Log(LOGWARNING, "%s - TV menu language set to unknown value '%s'", __FUNCTION__, strLanguage);
}

int CPeripheralCecAdapter::CecCommand(void *cbParam, const cec_command &command)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  if (adapter->m_bIsReady)
  {
    CLog::Log(LOGDEBUG, "%s - processing command: initiator=%1x destination=%1x opcode=%02x", __FUNCTION__, command.initiator, command.destination, command.opcode);

    switch (command.opcode)
    {
    case CEC_OPCODE_STANDBY:
      /* a device was put in standby mode */
      CLog::Log(LOGDEBUG, "%s - device %1x was put in standby mode", __FUNCTION__, command.initiator);
      if (command.initiator == CECDEVICE_TV && adapter->m_configuration.bPowerOffOnStandby == 1 &&
          (!adapter->m_screensaverLastActivated.IsValid() || CDateTime::GetCurrentDateTime() - adapter->m_screensaverLastActivated > CDateTimeSpan(0, 0, 0, SCREENSAVER_TIMEOUT)))
      {
        adapter->m_bStarted = false;
        g_application.getApplicationMessenger().Suspend();
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
        CSingleLock lock(adapter->m_critSection);
        cec_keypress key;
        key.duration = 500;
        key.keycode = CEC_USER_CONTROL_CODE_STOP;
        adapter->m_buttonQueue.push(key);
      }
      break;
    case CEC_OPCODE_PLAY:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1)
      {
        if (command.parameters[0] == CEC_PLAY_MODE_PLAY_FORWARD)
        {
          CSingleLock lock(adapter->m_critSection);
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PLAY;
          adapter->m_buttonQueue.push(key);
        }
        else if (command.parameters[0] == CEC_PLAY_MODE_PLAY_STILL)
        {
          CSingleLock lock(adapter->m_critSection);
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PAUSE;
          adapter->m_buttonQueue.push(key);
        }
      }
      break;
    case CEC_OPCODE_REPORT_POWER_STATUS:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1 &&
          command.parameters[0] == CEC_POWER_STATUS_ON &&
          adapter->m_queryThread)
      {
        adapter->m_queryThread->Signal();
      }
      break;
    default:
      break;
    }
  }
  return 1;
}

int CPeripheralCecAdapter::CecConfiguration(void *cbParam, const libcec_configuration &config)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  CSingleLock lock(adapter->m_critSection);
  adapter->SetConfigurationFromLibCEC(config);
  return 1;
}

int CPeripheralCecAdapter::CecKeyPress(void *cbParam, const cec_keypress &key)
{
  CPeripheralCecAdapter *adapter = (CPeripheralCecAdapter *)cbParam;
  if (!adapter)
    return 0;

  CSingleLock lock(adapter->m_critSection);
  adapter->m_buttonQueue.push(key);
  return 1;
}

bool CPeripheralCecAdapter::GetNextCecKey(cec_keypress &key)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  if (!m_buttonQueue.empty())
  {
    key = m_buttonQueue.front();
    m_buttonQueue.pop();
    bReturn = true;
  }

  return bReturn;
}

bool CPeripheralCecAdapter::GetNextKey(void)
{
  bool bHasButton(false);
  CSingleLock lock(m_critSection);
  if (m_bHasButton && m_button.iDuration > 0)
    return bHasButton;

  cec_keypress key;
  if (!m_bIsReady || !GetNextCecKey(key))
    return bHasButton;

  CLog::Log(LOGDEBUG, "%s - received key %2x", __FUNCTION__, key.keycode);
  WORD iButton = 0;
  bHasButton = true;

  switch (key.keycode)
  {
  case CEC_USER_CONTROL_CODE_SELECT:
    iButton = XINPUT_IR_REMOTE_SELECT;
    break;
  case CEC_USER_CONTROL_CODE_UP:
    iButton = XINPUT_IR_REMOTE_UP;
    break;
  case CEC_USER_CONTROL_CODE_DOWN:
    iButton = XINPUT_IR_REMOTE_DOWN;
    break;
  case CEC_USER_CONTROL_CODE_LEFT:
  case CEC_USER_CONTROL_CODE_LEFT_UP:
  case CEC_USER_CONTROL_CODE_LEFT_DOWN:
    iButton = XINPUT_IR_REMOTE_LEFT;
    break;
  case CEC_USER_CONTROL_CODE_RIGHT:
  case CEC_USER_CONTROL_CODE_RIGHT_UP:
  case CEC_USER_CONTROL_CODE_RIGHT_DOWN:
    iButton = XINPUT_IR_REMOTE_RIGHT;
    break;
  case CEC_USER_CONTROL_CODE_ROOT_MENU:
    iButton = XINPUT_IR_REMOTE_MENU;
    break;
  case CEC_USER_CONTROL_CODE_EXIT:
    iButton = XINPUT_IR_REMOTE_BACK;
    break;
  case CEC_USER_CONTROL_CODE_ENTER:
    iButton = XINPUT_IR_REMOTE_ENTER;
    break;
  case CEC_USER_CONTROL_CODE_CHANNEL_DOWN:
    iButton = XINPUT_IR_REMOTE_CHANNEL_MINUS;
    break;
  case CEC_USER_CONTROL_CODE_CHANNEL_UP:
    iButton = XINPUT_IR_REMOTE_CHANNEL_PLUS;
    break;
  case CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL:
    iButton = XINPUT_IR_REMOTE_BACK;
    break;
  case CEC_USER_CONTROL_CODE_SOUND_SELECT:
    iButton = XINPUT_IR_REMOTE_LANGUAGE;
    break;
  case CEC_USER_CONTROL_CODE_POWER:
    iButton = XINPUT_IR_REMOTE_POWER;
    break;
  case CEC_USER_CONTROL_CODE_VOLUME_UP:
    iButton = XINPUT_IR_REMOTE_VOLUME_PLUS;
    break;
  case CEC_USER_CONTROL_CODE_VOLUME_DOWN:
    iButton = XINPUT_IR_REMOTE_VOLUME_MINUS;
    break;
  case CEC_USER_CONTROL_CODE_MUTE:
    iButton = XINPUT_IR_REMOTE_MUTE;
    break;
  case CEC_USER_CONTROL_CODE_PLAY:
    iButton = XINPUT_IR_REMOTE_PLAY;
    break;
  case CEC_USER_CONTROL_CODE_STOP:
    iButton = XINPUT_IR_REMOTE_STOP;
    break;
  case CEC_USER_CONTROL_CODE_PAUSE:
    iButton = XINPUT_IR_REMOTE_PAUSE;
    break;
  case CEC_USER_CONTROL_CODE_REWIND:
    iButton = XINPUT_IR_REMOTE_REVERSE;
    break;
  case CEC_USER_CONTROL_CODE_FAST_FORWARD:
    iButton = XINPUT_IR_REMOTE_FORWARD;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER0:
    iButton = XINPUT_IR_REMOTE_0;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER1:
    iButton = XINPUT_IR_REMOTE_1;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER2:
    iButton = XINPUT_IR_REMOTE_2;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER3:
    iButton = XINPUT_IR_REMOTE_3;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER4:
    iButton = XINPUT_IR_REMOTE_4;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER5:
    iButton = XINPUT_IR_REMOTE_5;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER6:
    iButton = XINPUT_IR_REMOTE_6;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER7:
    iButton = XINPUT_IR_REMOTE_7;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER8:
    iButton = XINPUT_IR_REMOTE_8;
    break;
  case CEC_USER_CONTROL_CODE_NUMBER9:
    iButton = XINPUT_IR_REMOTE_9;
    break;
  case CEC_USER_CONTROL_CODE_RECORD:
    iButton = XINPUT_IR_REMOTE_RECORD;
    break;
  case CEC_USER_CONTROL_CODE_CLEAR:
    iButton = XINPUT_IR_REMOTE_CLEAR;
    break;
  case CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION:
    iButton = XINPUT_IR_REMOTE_INFO;
    break;
  case CEC_USER_CONTROL_CODE_PAGE_UP:
    iButton = XINPUT_IR_REMOTE_CHANNEL_PLUS;
    break;
  case CEC_USER_CONTROL_CODE_PAGE_DOWN:
    iButton = XINPUT_IR_REMOTE_CHANNEL_MINUS;
    break;
  case CEC_USER_CONTROL_CODE_FORWARD:
    iButton = XINPUT_IR_REMOTE_SKIP_PLUS;
    break;
  case CEC_USER_CONTROL_CODE_BACKWARD:
    iButton = XINPUT_IR_REMOTE_SKIP_MINUS;
    break;
  case CEC_USER_CONTROL_CODE_F1_BLUE:
    iButton = XINPUT_IR_REMOTE_BLUE;
    break;
  case CEC_USER_CONTROL_CODE_F2_RED:
    iButton = XINPUT_IR_REMOTE_RED;
    break;
  case CEC_USER_CONTROL_CODE_F3_GREEN:
    iButton = XINPUT_IR_REMOTE_GREEN;
    break;
  case CEC_USER_CONTROL_CODE_F4_YELLOW:
    iButton = XINPUT_IR_REMOTE_YELLOW;
    break;
  case CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION:
  case CEC_USER_CONTROL_CODE_EJECT:
  case CEC_USER_CONTROL_CODE_SETUP_MENU:
  case CEC_USER_CONTROL_CODE_CONTENTS_MENU:
  case CEC_USER_CONTROL_CODE_FAVORITE_MENU:
  case CEC_USER_CONTROL_CODE_DOT:
  case CEC_USER_CONTROL_CODE_NEXT_FAVORITE:
  case CEC_USER_CONTROL_CODE_INPUT_SELECT:
  case CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION:
  case CEC_USER_CONTROL_CODE_HELP:
  case CEC_USER_CONTROL_CODE_STOP_RECORD:
  case CEC_USER_CONTROL_CODE_PAUSE_RECORD:
  case CEC_USER_CONTROL_CODE_ANGLE:
  case CEC_USER_CONTROL_CODE_SUB_PICTURE:
  case CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND:
  case CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE:
  case CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING:
  case CEC_USER_CONTROL_CODE_PLAY_FUNCTION:
  case CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION:
  case CEC_USER_CONTROL_CODE_RECORD_FUNCTION:
  case CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION:
  case CEC_USER_CONTROL_CODE_STOP_FUNCTION:
  case CEC_USER_CONTROL_CODE_MUTE_FUNCTION:
  case CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION:
  case CEC_USER_CONTROL_CODE_TUNE_FUNCTION:
  case CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION:
  case CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION:
  case CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION:
  case CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION:
  case CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION:
  case CEC_USER_CONTROL_CODE_F5:
  case CEC_USER_CONTROL_CODE_DATA:
  case CEC_USER_CONTROL_CODE_UNKNOWN:
  default:
    bHasButton = false;
    return bHasButton;
  }

  if (!m_bHasButton && bHasButton && iButton == m_button.iButton && m_button.iDuration == 0 && key.duration > 0)
  {
    /* released button of the previous keypress */
    return m_bHasButton;
  }

  if (bHasButton)
  {
    m_bHasButton = true;
    m_button.iDuration = key.duration;
    m_button.iButton = iButton;
  }

  return m_bHasButton;
}

WORD CPeripheralCecAdapter::GetButton(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bHasButton)
    GetNextKey();

  return m_bHasButton ? m_button.iButton : 0;
}

unsigned int CPeripheralCecAdapter::GetHoldTime(void)
{
  CSingleLock lock(m_critSection);
  if (m_bHasButton && m_button.iDuration == 0)
    GetNextKey();

  return m_bHasButton ? m_button.iDuration : 0;
}

void CPeripheralCecAdapter::ResetButton(void)
{
  CSingleLock lock(m_critSection);
  m_bHasButton = false;
}

void CPeripheralCecAdapter::OnSettingChanged(const CStdString &strChangedSetting)
{
  if (strChangedSetting.Equals("enabled"))
  {
    bool bEnabled(GetSettingBool("enabled"));
    if (!bEnabled && m_cecAdapter && m_bStarted)
      StopThread(true);
    else if (bEnabled && !m_cecAdapter && m_bStarted)
      InitialiseFeature(FEATURE_CEC);
  }
  else
  {
    SetConfigurationFromSettings();
    m_queryThread->UpdateConfiguration(&m_configuration);
  }
}

int CPeripheralCecAdapter::CecLogMessage(void *cbParam, const cec_log_message &message)
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

  if (iLevel >= 0)
    CLog::Log(iLevel, "%s - %s", __FUNCTION__, message.message);

  return 1;
}

bool CPeripheralCecAdapter::TranslateComPort(CStdString &strLocation)
{
  if (strLocation.Left(18).Equals("peripherals://usb/") && strLocation.Right(4).Equals(".dev"))
  {
    strLocation = strLocation.Right(strLocation.length() - 18);
    strLocation = strLocation.Left(strLocation.length() - 4);
    return true;
  }

  return false;
}

void CPeripheralCecAdapter::SetConfigurationFromLibCEC(const CEC::libcec_configuration &config)
{
  // set the primary device type
  m_configuration.deviceTypes.Clear();
  m_configuration.deviceTypes.Add(config.deviceTypes[0]);
  SetSetting("device_type", (int)config.deviceTypes[0]);

  // set the connected device
  m_configuration.baseDevice = config.baseDevice;
  SetSetting("connected_device", (int)config.baseDevice);

  // set the HDMI port number
  m_configuration.iHDMIPort = config.iHDMIPort;
  SetSetting("cec_hdmi_port", config.iHDMIPort);

  // set the physical address, when baseDevice or iHDMIPort are not set
  if (m_configuration.baseDevice == CECDEVICE_UNKNOWN ||
      m_configuration.iHDMIPort == 0 || m_configuration.iHDMIPort > 4)
  {
    m_configuration.iPhysicalAddress = config.iPhysicalAddress;
    CStdString strPhysicalAddress;
    strPhysicalAddress.Format("%x", config.iPhysicalAddress);
    SetSetting("physical_address", strPhysicalAddress);
  }

  // set the tv vendor override
  m_configuration.tvVendor = config.tvVendor;
  SetSetting("tv_vendor", (int)config.tvVendor);

  // set the devices to wake when starting
  m_configuration.wakeDevices = config.wakeDevices;
  CStdString strWakeDevices;
  for (unsigned int iPtr = 0; iPtr <= 16; iPtr++)
    if (config.wakeDevices[iPtr])
      strWakeDevices.AppendFormat(" %X", iPtr);
  SetSetting("wake_devices", strWakeDevices.Trim());

  // set the devices to power off when stopping
  m_configuration.powerOffDevices = config.powerOffDevices;
  CStdString strPowerOffDevices;
  for (unsigned int iPtr = 0; iPtr <= 16; iPtr++)
    if (config.powerOffDevices[iPtr])
      strPowerOffDevices.AppendFormat(" %X", iPtr);
  SetSetting("wake_devices", strPowerOffDevices.Trim());

  // set the boolean settings
  m_configuration.bUseTVMenuLanguage = m_configuration.bUseTVMenuLanguage;
  SetSetting("use_tv_menu_language", m_configuration.bUseTVMenuLanguage == 1);

  m_configuration.bActivateSource = m_configuration.bActivateSource;
  SetSetting("activate_source", m_configuration.bActivateSource == 1);

  m_configuration.bPowerOffScreensaver = m_configuration.bPowerOffScreensaver;
  SetSetting("cec_standby_screensaver", m_configuration.bPowerOffScreensaver == 1);

  m_configuration.bPowerOffOnStandby = m_configuration.bPowerOffOnStandby;
  SetSetting("standby_pc_on_tv_standby", m_configuration.bPowerOffOnStandby == 1);
}

void CPeripheralCecAdapter::SetConfigurationFromSettings(void)
{
  // client version 1.5.0
  m_configuration.clientVersion = CEC_CLIENT_VERSION_1_5_0;

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
  m_configuration.bAutodetectAddress = 1;

  // set the physical address
  // when set, it will override the connected device and hdmi port settings
  CStdString strPhysicalAddress = GetSettingString("physical_address");
  int iPhysicalAddress;
  if (sscanf(strPhysicalAddress.c_str(), "%x", &iPhysicalAddress) == 1 && iPhysicalAddress > 0 && iPhysicalAddress < 0xFFFF)
    m_configuration.iPhysicalAddress = iPhysicalAddress;

  // set the connected device
  int iConnectedDevice = GetSettingInt("connected_device");
  if (iConnectedDevice == 0 || iConnectedDevice == 5)
    m_configuration.baseDevice = (cec_logical_address)iConnectedDevice;

  // set the HDMI port number
  int iHDMIPort = GetSettingInt("cec_hdmi_port");
  if (iHDMIPort >= 0 && iHDMIPort <= 4)
    m_configuration.iHDMIPort = iHDMIPort;

  // set the tv vendor override
  int iVendor = GetSettingInt("tv_vendor");
  if (iVendor > 0 && iVendor < 0xFFFFFF)
    m_configuration.tvVendor = iVendor;

  // read the devices to wake when starting
  CStdString strWakeDevices = CStdString(GetSettingString("wake_devices")).Trim();
  m_configuration.wakeDevices.Clear();
  ReadLogicalAddresses(strWakeDevices, m_configuration.wakeDevices);

  // read the devices to power off when stopping
  CStdString strStandbyDevices = CStdString(GetSettingString("standby_devices")).Trim();
  m_configuration.powerOffDevices.Clear();
  ReadLogicalAddresses(strStandbyDevices, m_configuration.powerOffDevices);

  // always get the settings from the rom, when supported by the firmware
  m_configuration.bGetSettingsFromROM = 1;

  // read the boolean settings
  m_configuration.bUseTVMenuLanguage   = GetSettingBool("use_tv_menu_language") ? 1 : 0;
  m_configuration.bActivateSource      = GetSettingBool("activate_source") ? 1 : 0;
  m_configuration.bPowerOffScreensaver = GetSettingBool("cec_standby_screensaver") ? 1 : 0;
  m_configuration.bPowerOffOnStandby   = GetSettingBool("standby_pc_on_tv_standby") ? 1 : 0;
}

void CPeripheralCecAdapter::ReadLogicalAddresses(const CStdString &strString, cec_logical_addresses &addresses)
{
  for (size_t iPtr = 0; iPtr < strString.size(); iPtr++)
  {
    CStdString strDevice = CStdString(strString.substr(iPtr, 1)).Trim();
    if (!strDevice.IsEmpty())
    {
      int iDevice(0);
      if (sscanf(strDevice.c_str(), "%x", &iDevice) == 1 && iDevice >= 0 && iDevice <= 0xF)
        addresses.Set((cec_logical_address)iDevice);
    }
  }
}

CPeripheralCecAdapterUpdateThread::CPeripheralCecAdapterUpdateThread(CPeripheralCecAdapter *adapter, libcec_configuration *configuration) :
    CThread("CEC Adapter Update Thread"),
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

bool CPeripheralCecAdapterUpdateThread::SetInitialConfiguration(void)
{
  // devices to wake are set
  if (!m_configuration.wakeDevices.IsEmpty())
    m_adapter->m_cecAdapter->PowerOnDevices(CECDEVICE_BROADCAST);

  // the option to make XBMC the active source is set
  if (m_configuration.bActivateSource == 1)
    m_adapter->m_cecAdapter->SetActiveSource();

  // wait until devices are powered up
  if (!WaitReady())
    return false;

  // request the menu language of the TV
  if (m_configuration.bUseTVMenuLanguage == 1)
  {
    cec_menu_language language;
    if (m_adapter->m_cecAdapter->GetDeviceMenuLanguage(CECDEVICE_TV, &language))
      m_adapter->SetMenuLanguage(language.language);
  }

  // request the OSD name of the TV
  CStdString strNotification;
  cec_osd_name tvName = m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_TV);
  strNotification.Format("%s: %s", g_localizeStrings.Get(36016), tvName.name);

  /* disable the mute setting when an amp is found, because the amp handles the mute setting and
     set PCM output to 100% */
  if (m_adapter->m_cecAdapter->IsActiveDeviceType(CEC_DEVICE_TYPE_AUDIO_SYSTEM))
  {
    // request the OSD name of the amp
    cec_osd_name ampName = m_adapter->m_cecAdapter->GetDeviceOSDName(CECDEVICE_AUDIOSYSTEM);
    CLog::Log(LOGDEBUG, "%s - CEC capable amplifier found (%s). volume will be controlled on the amp", __FUNCTION__, ampName.name);
    strNotification.AppendFormat(" - %s", ampName.name);

    // set amp present
    m_adapter->SetAudioSystemConnected(true);
    g_settings.m_bMute = false;
    g_settings.m_nVolumeLevel = VOLUME_MAXIMUM;
  }

  m_adapter->m_bIsReady = true;

  // try to send an OSD string to the TV
  m_adapter->m_cecAdapter->SetOSDString(CECDEVICE_TV, CEC_DISPLAY_CONTROL_DISPLAY_FOR_DEFAULT_TIME, g_localizeStrings.Get(36016).c_str());
  // and let the gui know that we're done
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), strNotification);

  CSingleLock lock(m_critSection);
  m_bIsUpdating = false;
  return true;
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
    if (m_event.WaitMSec(500) || bUpdate)
    {
      // set the new configuration
      bool bConfigSet(m_adapter->m_cecAdapter->SetConfiguration(&m_configuration));
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36000), g_localizeStrings.Get(bConfigSet ? 36023 : 36024));
      {
        CSingleLock lock(m_critSection);
        bUpdate = m_bNextConfigurationScheduled;
        if (bUpdate)
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

#endif
