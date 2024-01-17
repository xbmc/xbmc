/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if !defined(HAVE_LIBCEC)
#include "Peripheral.h"

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 *
 * An empty implementation, so CPeripherals can be compiled without a bunch of
 * #ifdef's when libCEC is not available.
 */
class CPeripheralCecAdapter : public CPeripheral
{
public:
  bool HasAudioControl(void) { return false; }
  void VolumeUp(void) {}
  void VolumeDown(void) {}
  bool IsMuted(void) { return false; }
  void ToggleMute(void) {}
  bool ToggleDeviceState(CecStateChange mode = STATE_SWITCH_TOGGLE, bool forceType = false)
  {
    return false;
  }

  int GetButton(void) { return 0; }
  unsigned int GetHoldTime(void) { return 0; }
  void ResetButton(void) {}
};
} // namespace PERIPHERALS

#else

#include "PeripheralHID.h"
#include "XBDateTime.h"
#include "interfaces/AnnouncementManager.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <chrono>
#include <queue>
#include <vector>

// undefine macro isset, it collides with function in cectypes.h
#ifdef isset
#undef isset
#endif
#include <libcec/cectypes.h>

class CVariant;

namespace CEC
{
class ICECAdapter;
};

namespace PERIPHERALS
{
class CPeripheralCecAdapterUpdateThread;
class CPeripheralCecAdapterReopenJob;

typedef struct
{
  int iButton;
  unsigned int iDuration;
} CecButtonPress;

typedef enum
{
  VOLUME_CHANGE_NONE,
  VOLUME_CHANGE_UP,
  VOLUME_CHANGE_DOWN,
  VOLUME_CHANGE_MUTE
} CecVolumeChange;

class CPeripheralCecAdapter : public CPeripheralHID,
                              public ANNOUNCEMENT::IAnnouncer,
                              private CThread
{
  friend class CPeripheralCecAdapterUpdateThread;
  friend class CPeripheralCecAdapterReopenJob;

public:
  CPeripheralCecAdapter(CPeripherals& manager,
                        const PeripheralScanResult& scanResult,
                        CPeripheralBus* bus);
  ~CPeripheralCecAdapter(void) override;

  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;

  // audio control
  bool HasAudioControl(void);
  void VolumeUp(void);
  void VolumeDown(void);
  void ToggleMute(void);
  bool IsMuted(void);

  // CPeripheral callbacks
  void OnSettingChanged(const std::string& strChangedSetting) override;
  void OnDeviceRemoved(void) override;

  // input
  int GetButton(void);
  unsigned int GetHoldTime(void);
  void ResetButton(void);

  // public CEC methods
  void ActivateSource(void);
  void StandbyDevices(void);
  bool ToggleDeviceState(CecStateChange mode = STATE_SWITCH_TOGGLE, bool forceType = false);

private:
  bool InitialiseFeature(const PeripheralFeature feature) override;
  void ResetMembers(void);
  void Process(void) override;
  bool IsRunning(void) const;

  bool OpenConnection(void);
  bool ReopenConnection(bool bAsync = false);

  void SetConfigurationFromSettings(void);
  void SetConfigurationFromLibCEC(const CEC::libcec_configuration& config);
  void SetVersionInfo(const CEC::libcec_configuration& configuration);

  static void ReadLogicalAddresses(const std::string& strString,
                                   CEC::cec_logical_addresses& addresses);
  static void ReadLogicalAddresses(int iLocalisedId, CEC::cec_logical_addresses& addresses);
  bool WriteLogicalAddresses(const CEC::cec_logical_addresses& addresses,
                             const std::string& strSettingName,
                             const std::string& strAdvancedSettingName);

  void ProcessActivateSource(void);
  void ProcessStandbyDevices(void);
  void ProcessVolumeChange(void);

  void PushCecKeypress(const CEC::cec_keypress& key);
  void PushCecKeypress(const CecButtonPress& key);
  void GetNextKey(void);

  void SetAudioSystemConnected(bool bSetTo);
  void SetMenuLanguage(const char* strLanguage);
  void OnTvStandby(void);

  // callbacks from libCEC
  static void CecLogMessage(void* cbParam, const CEC::cec_log_message* message);
  static void CecCommand(void* cbParam, const CEC::cec_command* command);
  static void CecConfiguration(void* cbParam, const CEC::libcec_configuration* config);
  static void CecAlert(void* cbParam,
                       const CEC::libcec_alert alert,
                       const CEC::libcec_parameter data);
  static void CecSourceActivated(void* param,
                                 const CEC::cec_logical_address address,
                                 const uint8_t activated);
  static void CecKeyPress(void* cbParam, const CEC::cec_keypress* key);

  CEC::ICECAdapter* m_cecAdapter;
  bool m_bStarted;
  bool m_bHasButton;
  bool m_bIsReady;
  bool m_bHasConnectedAudioSystem;
  std::string m_strMenuLanguage;
  CDateTime m_standbySent;
  std::vector<CecButtonPress> m_buttonQueue;
  CecButtonPress m_currentButton;
  std::queue<CecVolumeChange> m_volumeChangeQueue;
  std::chrono::time_point<std::chrono::steady_clock> m_lastKeypress;
  CecVolumeChange m_lastChange;
  int m_iExitCode;
  bool m_bIsMuted;
  bool m_bGoingToStandby;
  bool m_bIsRunning;
  bool m_bDeviceRemoved;
  CPeripheralCecAdapterUpdateThread* m_queryThread;
  CEC::ICECCallbacks m_callbacks;
  mutable CCriticalSection m_critSection;
  CEC::libcec_configuration m_configuration;
  bool m_bActiveSourcePending;
  bool m_bStandbyPending;
  CDateTime m_preventActivateSourceOnPlay;
  bool m_bActiveSourceBeforeStandby;
  bool m_bOnPlayReceived;
  bool m_bPlaybackPaused;
  std::string m_strComPort;
  bool m_bPowerOnScreensaver;
  bool m_bUseTVMenuLanguage;
  bool m_bSendInactiveSource;
  bool m_bPowerOffScreensaver;
  bool m_bShutdownOnStandby;
};

class CPeripheralCecAdapterUpdateThread : public CThread
{
public:
  CPeripheralCecAdapterUpdateThread(CPeripheralCecAdapter* adapter,
                                    CEC::libcec_configuration* configuration);
  ~CPeripheralCecAdapterUpdateThread(void) override;

  void Signal(void);
  bool UpdateConfiguration(CEC::libcec_configuration* configuration);

protected:
  void UpdateMenuLanguage(void);
  std::string UpdateAudioSystemStatus(void);
  bool WaitReady(void);
  bool SetInitialConfiguration(void);
  void Process(void) override;

  CPeripheralCecAdapter* m_adapter;
  CEvent m_event;
  CCriticalSection m_critSection;
  CEC::libcec_configuration m_configuration;
  CEC::libcec_configuration m_nextConfiguration;
  bool m_bNextConfigurationScheduled = false;
  bool m_bIsUpdating = true;
};
} // namespace PERIPHERALS

#endif
