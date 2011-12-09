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
#include "utils/log.h"

#include <cec.h>

using namespace PERIPHERALS;
using namespace ANNOUNCEMENT;
using namespace CEC;

#define CEC_LIB_SUPPORTED_VERSION 1

/* time in seconds to ignore standby commands from devices after the screensaver has been activated */
#define SCREENSAVER_TIMEOUT       10

class DllLibCECInterface
{
public:
  virtual ~DllLibCECInterface() {}
  virtual ICECAdapter* CECInit(const char *interfaceName, cec_device_type_list types)=0;
  virtual void* CECDestroy(ICECAdapter *adapter)=0;
};

class DllLibCEC : public DllDynamic, DllLibCECInterface
{
  DECLARE_DLL_WRAPPER(DllLibCEC, DLL_PATH_LIBCEC)

  DEFINE_METHOD2(ICECAdapter*, CECInit,   (const char *p1, cec_device_type_list p2))
  DEFINE_METHOD1(void*       , CECDestroy,  (ICECAdapter *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(CECInit,  CECInit)
    RESOLVE_METHOD_RENAME(CECDestroy, CECDestroy)
  END_METHOD_RESOLVE()
};

CPeripheralCecAdapter::CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheralHID(type, busType, strLocation, strDeviceName, iVendorId, iProductId),
  CThread("CEC Adapter"),
  m_bStarted(false),
  m_bHasButton(false),
  m_bIsReady(false),
  m_strMenuLanguage("???")
{
  m_button.iButton = 0;
  m_button.iDuration = 0;
  m_screensaverLastActivated.SetValid(false);
  m_dll = new DllLibCEC;
  if (m_dll->Load() && m_dll->IsLoaded())
  {
    cec_device_type_list typeList;
    typeList.clear();
    typeList.add(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    m_cecAdapter = m_dll->CECInit("XBMC", typeList);
  }
  else
    m_cecAdapter = NULL;

  if (!m_cecAdapter || m_cecAdapter->GetMinLibVersion() > CEC_LIB_SUPPORTED_VERSION)
  {
    /* unsupported libcec version */
    CLog::Log(LOGERROR, g_localizeStrings.Get(36013).c_str(), CEC_LIB_SUPPORTED_VERSION, m_cecAdapter ? m_cecAdapter->GetMinLibVersion() : -1);

    CStdString strMessage;
    strMessage.Format(g_localizeStrings.Get(36013).c_str(), CEC_LIB_SUPPORTED_VERSION, m_cecAdapter ? m_cecAdapter->GetMinLibVersion() : -1);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), strMessage);
    m_bError = true;
    if (m_cecAdapter)
      m_dll->CECDestroy(m_cecAdapter);
    m_cecAdapter = NULL;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - using libCEC v%d.%d", __FUNCTION__, m_cecAdapter->GetLibVersionMajor(), m_cecAdapter->GetLibVersionMinor());
    m_features.push_back(FEATURE_CEC);
  }
}

CPeripheralCecAdapter::~CPeripheralCecAdapter(void)
{
  CAnnouncementManager::RemoveAnnouncer(this);

  m_bStop = true;
  StopThread(true);

  if (m_dll && m_cecAdapter)
  {
    FlushLog();
    m_dll->CECDestroy(m_cecAdapter);
    m_cecAdapter = NULL;
    delete m_dll;
    m_dll = NULL;
  }
}

void CPeripheralCecAdapter::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnQuit") && m_bIsReady)
  {
    if (GetSettingBool("cec_power_off_shutdown"))
      m_cecAdapter->StandbyDevices();
    else if (GetSettingBool("cec_mark_inactive_shutdown"))
      m_cecAdapter->SetInactiveView();
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverDeactivated") && GetSettingBool("cec_standby_screensaver") && m_bIsReady)
  {
    m_cecAdapter->PowerOnDevices();
  }
  else if (flag == GUI && !strcmp(sender, "xbmc") && !strcmp(message, "OnScreensaverActivated") && GetSettingBool("cec_standby_screensaver"))
  {
    m_screensaverLastActivated = CDateTime::GetCurrentDateTime();
    m_cecAdapter->StandbyDevices();
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnSleep"))
  {
    if (GetSettingBool("cec_power_off_shutdown") && m_bIsReady)
      m_cecAdapter->StandbyDevices();
    CSingleLock lock(m_critSection);
    m_bStop = true;
    WaitForThreadExit(0);
  }
  else if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "OnWake"))
  {
    CSingleLock lock(m_critSection);
    CLog::Log(LOGDEBUG, "%s - reconnecting to the CEC adapter after standby mode", __FUNCTION__);
    m_cecAdapter->Close();

    CStdString strPort = GetComPort();
    if (!strPort.empty())
    {
      if (!m_cecAdapter->Open(strPort.c_str(), 10000))
      {
        CLog::Log(LOGERROR, "%s - failed to reconnect to the CEC adapter", __FUNCTION__);
        FlushLog();
        m_bStop = true;
      }
      else
      {
        if (GetSettingBool("cec_power_on_startup"))
          PowerOnCecDevices(CECDEVICE_TV);
        m_cecAdapter->SetActiveView();
      }
    }
  }
  else if (flag == Player && !strcmp(sender, "xbmc") && !strcmp(message, "OnStop"))
  {
    m_cecAdapter->SetDeckControlMode(CEC_DECK_CONTROL_MODE_STOP, false);
    m_cecAdapter->SetDeckInfo(CEC_DECK_INFO_STOP);
  }
  else if (flag == Player && !strcmp(sender, "xbmc") && !strcmp(message, "OnPause"))
  {
    m_cecAdapter->SetDeckControlMode(CEC_DECK_CONTROL_MODE_SKIP_FORWARD_WIND, false);
    m_cecAdapter->SetDeckInfo(CEC_DECK_INFO_STILL);
  }
  else if (flag == Player && !strcmp(sender, "xbmc") && !strcmp(message, "OnPlay"))
  {
    m_cecAdapter->SetDeckControlMode(CEC_DECK_CONTROL_MODE_SKIP_FORWARD_WIND, false);
    m_cecAdapter->SetDeckInfo(CEC_DECK_INFO_PLAY);
  }
}

bool CPeripheralCecAdapter::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_CEC && !m_bStarted)
  {
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

void CPeripheralCecAdapter::Process(void)
{
  if (!GetSettingBool("enabled"))
  {
    CLog::Log(LOGDEBUG, "%s - CEC adapter is disabled in peripheral settings", __FUNCTION__);
    m_bStarted = false;
    return;
  }
  
  CStdString strPort = GetComPort();
  if (strPort.empty())
    return;

  // open the CEC adapter
  CLog::Log(LOGDEBUG, "%s - opening a connection to the CEC adapter: %s", __FUNCTION__, strPort.c_str());

  if (!m_cecAdapter->Open(strPort.c_str(), 10000))
  {
    FlushLog();
    CLog::Log(LOGERROR, "%s - could not opening a connection to the CEC adapter", __FUNCTION__);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(36000), g_localizeStrings.Get(36012));
    m_bStarted = false;
    return;
  }

  CLog::Log(LOGDEBUG, "%s - connection to the CEC adapter opened", __FUNCTION__);

  m_bIsReady = true;
  CAnnouncementManager::AddAnnouncer(this);

  if (GetSettingBool("cec_power_on_startup"))
  {
    PowerOnCecDevices(CECDEVICE_TV);
    FlushLog();
  }

  /* get the vendor id directly after connecting, because the TV might be using a non-standard CEC implementation */
  m_cecAdapter->GetDeviceVendorId(CECDEVICE_TV);

  // set correct physical address from peripheral settings
  int iHdmiPort = GetSettingInt("cec_hdmi_port");
  if (iHdmiPort <= 0 || iHdmiPort > 16)
    iHdmiPort = 1;
  m_cecAdapter->SetPhysicalAddress((uint16_t) (iHdmiPort << 12));
  FlushLog();

  if (GetSettingBool("use_tv_menu_language"))
  {
    cec_menu_language language;
    if (m_cecAdapter->GetDeviceMenuLanguage(CECDEVICE_TV, &language))
      SetMenuLanguage(language.language);
  }

  m_cecAdapter->SetOSDString(CECDEVICE_TV, CEC_DISPLAY_CONTROL_DISPLAY_FOR_DEFAULT_TIME, g_localizeStrings.Get(36016).c_str());

  while (!m_bStop)
  {
    FlushLog();
    if (!m_bStop)
      ProcessNextCommand();
    if (!m_bStop)
      Sleep(5);
  }

  m_cecAdapter->Close();

  CLog::Log(LOGDEBUG, "%s - CEC adapter processor thread ended", __FUNCTION__);
  m_bStarted = false;
}

bool CPeripheralCecAdapter::PowerOnCecDevices(cec_logical_address iLogicalAddress)
{
  bool bReturn(false);

  if (m_cecAdapter && m_bIsReady)
  {
    CLog::Log(LOGDEBUG, "%s - powering on CEC capable devices with address %1x", __FUNCTION__, iLogicalAddress);
    bReturn = m_cecAdapter->PowerOnDevices(iLogicalAddress);
  }

  return bReturn;
}

bool CPeripheralCecAdapter::StandbyCecDevices(cec_logical_address iLogicalAddress)
{
  bool bReturn(false);

  if (m_cecAdapter && m_bIsReady)
  {
    CLog::Log(LOGDEBUG, "%s - putting CEC capable devices with address %1x in standby mode", __FUNCTION__, iLogicalAddress);
    bReturn = m_cecAdapter->StandbyDevices(iLogicalAddress);
  }

  return bReturn;
}

bool CPeripheralCecAdapter::SendPing(void)
{
  bool bReturn(false);
  if (m_cecAdapter && m_bIsReady)
  {
    CLog::Log(LOGDEBUG, "%s - sending ping to the CEC adapter", __FUNCTION__);
    bReturn = m_cecAdapter->PingAdapter();
  }

  return bReturn;
}

bool CPeripheralCecAdapter::SetHdmiPort(int iHdmiPort)
{
  bool bReturn(false);
  if (m_cecAdapter && m_bIsReady)
  {
    if (iHdmiPort <= 0 || iHdmiPort > 16)
      iHdmiPort = 1;
    CLog::Log(LOGDEBUG, "%s - changing active HDMI port to %d", __FUNCTION__, iHdmiPort);
    bReturn = m_cecAdapter->SetPhysicalAddress(iHdmiPort << 12);
  }

  return bReturn;
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

void CPeripheralCecAdapter::ProcessNextCommand(void)
{
  cec_command command;
  if (m_cecAdapter && m_bIsReady && m_cecAdapter->GetNextCommand(&command))
  {
    CLog::Log(LOGDEBUG, "%s - processing command: initiator=%1x destination=%1x opcode=%02x", __FUNCTION__, command.initiator, command.destination, command.opcode);

    switch (command.opcode)
    {
    case CEC_OPCODE_STANDBY:
      /* a device was put in standby mode */
      CLog::Log(LOGDEBUG, "%s - device %1x was put in standby mode", __FUNCTION__, command.initiator);
      if (command.initiator == CECDEVICE_TV && GetSettingBool("standby_pc_on_tv_standby") &&
          (!m_screensaverLastActivated.IsValid() || CDateTime::GetCurrentDateTime() - m_screensaverLastActivated > CDateTimeSpan(0, 0, 0, SCREENSAVER_TIMEOUT)))
      {
        m_bStarted = false;
        g_application.getApplicationMessenger().Suspend();
      }
      break;
    case CEC_OPCODE_SET_MENU_LANGUAGE:
      if (GetSettingBool("use_tv_menu_language") && command.initiator == CECDEVICE_TV && command.parameters.size == 3)
      {
        char strNewLanguage[4];
        for (int iPtr = 0; iPtr < 3; iPtr++)
          strNewLanguage[iPtr] = command.parameters[iPtr];
        strNewLanguage[3] = 0;
        SetMenuLanguage(strNewLanguage);
      }
      break;
    case CEC_OPCODE_DECK_CONTROL:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1 &&
          command.parameters[0] == CEC_DECK_CONTROL_MODE_STOP)
      {
        CSingleLock lock(m_critSection);
        cec_keypress key;
        key.duration = 500;
        key.keycode = CEC_USER_CONTROL_CODE_STOP;
        m_buttonQueue.push(key);
      }
      break;
    case CEC_OPCODE_PLAY:
      if (command.initiator == CECDEVICE_TV &&
          command.parameters.size == 1)
      {
        if (command.parameters[0] == CEC_PLAY_MODE_PLAY_FORWARD)
        {
          CSingleLock lock(m_critSection);
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PLAY;
          m_buttonQueue.push(key);
        }
        else if (command.parameters[0] == CEC_PLAY_MODE_PLAY_STILL)
        {
          CSingleLock lock(m_critSection);
          cec_keypress key;
          key.duration = 500;
          key.keycode = CEC_USER_CONTROL_CODE_PAUSE;
          m_buttonQueue.push(key);
        }
      }
      break;
    default:
      break;
    }
  }
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
  else if (m_cecAdapter->GetNextKeypress(&key))
  {
    bReturn = true;
  }

  return bReturn;
}

bool CPeripheralCecAdapter::GetNextKey(void)
{
  CSingleLock lock(m_critSection);
  if (m_bHasButton && m_button.iDuration > 0)
    return false;

  cec_keypress key;
  if (!m_bIsReady || !GetNextCecKey(key))
    return false;

  CLog::Log(LOGDEBUG, "%s - received key %2x", __FUNCTION__, key.keycode);
  DWORD iButton = 0;

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
    m_bHasButton = false;
    return false;
  }

  if (!m_bHasButton && iButton == m_button.iButton && m_button.iDuration == 0 && key.duration > 0)
  {
    /* released button of the previous keypress */
    m_bHasButton = false;
    return false;
  }

  m_bHasButton = true;
  m_button.iDuration = key.duration;
  m_button.iButton = iButton;

  return true;
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
  else if (strChangedSetting.Equals("cec_hdmi_port"))
  {
    SetHdmiPort(GetSettingInt("cec_hdmi_port"));
  }
}

void CPeripheralCecAdapter::FlushLog(void)
{
  cec_log_message message;
  while (m_cecAdapter && m_cecAdapter->GetNextLogMessage(&message))
  {
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
      if (GetSettingBool("cec_debug_logging"))
        iLevel = LOGDEBUG;
      break;
    default:
      break;
    }

    if (iLevel >= 0)
      CLog::Log(iLevel, "%s - %s", __FUNCTION__, message.message);
  }
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
#endif
