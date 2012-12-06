#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if !defined(HAVE_LIBCEC)
#include "Peripheral.h"

// an empty implementation, so CPeripherals can be compiled without a bunch of #ifdef's when libCEC is not available
namespace PERIPHERALS
{
  class CPeripheralCecAdapter : public CPeripheral
  {
  public:
    bool HasConnectedAudioSystem(void) { return false; }
    void ScheduleVolumeUp(void) {}
    void ScheduleVolumeDown(void) {}
    bool IsMuted(void) { return false; }
    void ScheduleMute(void) {}

    int GetButton(void) { return 0; }
    unsigned int GetHoldTime(void) { return 0; }
    void ResetButton(void) {}
  };
}

#else

#include "PeripheralHID.h"
#include "interfaces/AnnouncementManager.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include <queue>

// undefine macro isset, it collides with function in cectypes.h
#ifdef isset
#undef isset
#endif
#include <libcec/cectypes.h>

class DllLibCEC;

namespace CEC
{
  class ICECAdapter;
};

namespace PERIPHERALS
{
  class CPeripheralCecAdapterUpdateThread;

  typedef struct
  {
    int         iButton;
    unsigned int iDuration;
  } CecButtonPress;

  typedef enum
  {
    VOLUME_CHANGE_NONE,
    VOLUME_CHANGE_UP,
    VOLUME_CHANGE_DOWN,
    VOLUME_CHANGE_MUTE
  } CecVolumeChange;

  class CPeripheralCecAdapter : public CPeripheralHID, public ANNOUNCEMENT::IAnnouncer, private CThread
  {
    friend class CPeripheralCecAdapterUpdateThread;

  public:
    CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralCecAdapter(void);

    void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    bool HasConnectedAudioSystem(void);
    void SetAudioSystemConnected(bool bSetTo);
    void ScheduleVolumeUp(void);
    void VolumeUp(void);
    void ScheduleVolumeDown(void);
    void VolumeDown(void);
    void ScheduleMute(void);
    void Mute(void);
    bool IsMuted(void);

    void OnSettingChanged(const CStdString &strChangedSetting);
    void OnDeviceRemoved(void);

    int GetButton(void);
    unsigned int GetHoldTime(void);
    void ResetButton(void);
    CStdString GetComPort(void);

    void PushCecKeypress(const CEC::cec_keypress &key);

    void ActivateSource(void);
    void StandbyDevices(void);

  protected:
    bool OpenConnection(void);
    void SetConfigurationFromSettings(void);
    void SetConfigurationFromLibCEC(const CEC::libcec_configuration &config);
    void SetVersionInfo(const CEC::libcec_configuration &configuration);
    static void ReadLogicalAddresses(const CStdString &strString, CEC::cec_logical_addresses &addresses);
    static void ReadLogicalAddresses(int iLocalisedId, CEC::cec_logical_addresses &addresses);
    bool WriteLogicalAddresses(const CEC::cec_logical_addresses& addresses, const std::string& strSettingName, const std::string& strAdvancedSettingName);
    static int CecKeyPress(void *cbParam, const CEC::cec_keypress key);
    void PushCecKeypress(const CecButtonPress &key);
    static int CecLogMessage(void *cbParam, const CEC::cec_log_message message);
    static int CecCommand(void *cbParam, const CEC::cec_command command);
    static int CecConfiguration(void *cbParam, const CEC::libcec_configuration config);
    static int CecAlert(void *cbParam, const CEC::libcec_alert alert, const CEC::libcec_parameter data);
    static void CecSourceActivated(void *param, const CEC::cec_logical_address address, const uint8_t activated);
    bool IsRunning(void) const;
    void ReopenConnection(void);
    void ProcessActivateSource(void);
    void ProcessStandbyDevices(void);

    void GetNextKey(void);
    bool InitialiseFeature(const PeripheralFeature feature);
    void Process(void);
    void ProcessVolumeChange(void);
    void SetMenuLanguage(const char *strLanguage);
    static bool FindConfigLocation(CStdString &strString);
    static bool TranslateComPort(CStdString &strPort);

    void ResetMembers(void);

    DllLibCEC*                        m_dll;
    CEC::ICECAdapter*                 m_cecAdapter;
    bool                              m_bStarted;
    bool                              m_bHasButton;
    bool                              m_bIsReady;
    bool                              m_bHasConnectedAudioSystem;
    CStdString                        m_strMenuLanguage;
    CDateTime                         m_screensaverLastActivated;
    std::vector<CecButtonPress>       m_buttonQueue;
    CecButtonPress                    m_currentButton;
    std::queue<CecVolumeChange>       m_volumeChangeQueue;
    unsigned int                      m_lastKeypress;
    CecVolumeChange                   m_lastChange;
    int                               m_iExitCode;
    bool                              m_bIsMuted;
    bool                              m_bGoingToStandby;
    bool                              m_bIsRunning;
    bool                              m_bDeviceRemoved;
    CPeripheralCecAdapterUpdateThread*m_queryThread;
    CEC::ICECCallbacks                m_callbacks;
    CCriticalSection                  m_critSection;
    CEC::libcec_configuration         m_configuration;
    bool                              m_bActiveSourcePending;
    bool                              m_bStandbyPending;
    CDateTime                         m_preventActivateSourceOnPlay;
  };

  class CPeripheralCecAdapterUpdateThread : public CThread
  {
  public:
    CPeripheralCecAdapterUpdateThread(CPeripheralCecAdapter *adapter, CEC::libcec_configuration *configuration);
    virtual ~CPeripheralCecAdapterUpdateThread(void);

    void Signal(void);
    bool UpdateConfiguration(CEC::libcec_configuration *configuration);

  protected:
    void UpdateMenuLanguage(void);
    CStdString UpdateAudioSystemStatus(void);
    bool WaitReady(void);
    bool SetInitialConfiguration(void);
    void Process(void);

    CPeripheralCecAdapter *    m_adapter;
    CEvent                     m_event;
    CCriticalSection           m_critSection;
    CEC::libcec_configuration  m_configuration;
    CEC::libcec_configuration  m_nextConfiguration;
    bool                       m_bNextConfigurationScheduled;
    bool                       m_bIsUpdating;
  };
}

#endif
