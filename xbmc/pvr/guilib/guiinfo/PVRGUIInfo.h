/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h"
#include "guilib/guiinfo/GUIInfoProvider.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/guilib/guiinfo/PVRGUITimerInfo.h"
#include "pvr/guilib/guiinfo/PVRGUITimesInfo.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <atomic>
#include <string>
#include <vector>

class CFileItem;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
class CGUIInfo;
}
} // namespace GUILIB
} // namespace KODI

namespace PVR
{
enum class PVREvent;
struct PVRChannelNumberInputChangedEvent;
struct PVRPreviewAndPlayerShowInfoChangedEvent;

class CPVRGUIInfo : public KODI::GUILIB::GUIINFO::CGUIInfoProvider, private CThread
{
public:
  CPVRGUIInfo();
  ~CPVRGUIInfo() override = default;

  void Start();
  void Stop();

  /*!
   * @brief CEventStream callback for PVR events.
   * @param event The event.
   */
  void Notify(const PVREvent& event);

  /*!
   * @brief CEventStream callback for channel number input changes.
   * @param event The event.
   */
  void Notify(const PVRChannelNumberInputChangedEvent& event);

  /*!
   * @brief CEventStream callback for channel preview and player show info changes.
   * @param event The event.
   */
  void Notify(const PVRPreviewAndPlayerShowInfoChangedEvent& event);

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem* item) override;
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                std::string* fallback) const override;
  bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                        std::string* fallback) override;
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override;
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override;

private:
  void ResetProperties();
  void ClearQualityInfo(PVR_SIGNAL_STATUS& qualityInfo);
  void ClearDescrambleInfo(PVR_DESCRAMBLE_INFO& descrambleInfo);

  void Process() override;

  void UpdateTimersCache();
  void UpdateBackendCache();
  void UpdateQualityData();
  void UpdateDescrambleData();
  void UpdateMisc();
  void UpdateNextTimer();
  void UpdateTimeshiftData();
  void UpdateTimeshiftProgressData();

  void UpdateTimersToggle();

  bool GetListItemAndPlayerLabel(const CFileItem* item,
                                 const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                                 std::string& strValue) const;
  bool GetPVRLabel(const CFileItem* item,
                   const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                   std::string& strValue) const;
  bool GetRadioRDSLabel(const CFileItem* item,
                        const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                        std::string& strValue) const;

  bool GetListItemAndPlayerInt(const CFileItem* item,
                               const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                               int& iValue) const;
  bool GetPVRInt(const CFileItem* item,
                 const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                 int& iValue) const;
  int GetTimeShiftSeekPercent() const;

  bool GetListItemAndPlayerBool(const CFileItem* item,
                                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                                bool& bValue) const;
  bool GetPVRBool(const CFileItem* item,
                  const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                  bool& bValue) const;
  bool GetRadioRDSBool(const CFileItem* item,
                       const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                       bool& bValue) const;

  void CharInfoBackendNumber(std::string& strValue) const;
  void CharInfoTotalDiskSpace(std::string& strValue) const;
  void CharInfoSignal(std::string& strValue) const;
  void CharInfoSNR(std::string& strValue) const;
  void CharInfoBER(std::string& strValue) const;
  void CharInfoUNC(std::string& strValue) const;
  void CharInfoFrontendName(std::string& strValue) const;
  void CharInfoFrontendStatus(std::string& strValue) const;
  void CharInfoBackendName(std::string& strValue) const;
  void CharInfoBackendVersion(std::string& strValue) const;
  void CharInfoBackendHost(std::string& strValue) const;
  void CharInfoBackendDiskspace(std::string& strValue) const;
  void CharInfoBackendProviders(std::string& strValue) const;
  void CharInfoBackendChannelGroups(std::string& strValue) const;
  void CharInfoBackendChannels(std::string& strValue) const;
  void CharInfoBackendTimers(std::string& strValue) const;
  void CharInfoBackendRecordings(std::string& strValue) const;
  void CharInfoBackendDeletedRecordings(std::string& strValue) const;
  void CharInfoPlayingClientName(std::string& strValue) const;
  void CharInfoEncryption(std::string& strValue) const;
  void CharInfoService(std::string& strValue) const;
  void CharInfoMux(std::string& strValue) const;
  void CharInfoProvider(std::string& strValue) const;

  /** @name PVRGUIInfo data */
  //@{
  CPVRGUIAnyTimerInfo m_anyTimersInfo; // tv + radio
  CPVRGUITVTimerInfo m_tvTimersInfo;
  CPVRGUIRadioTimerInfo m_radioTimersInfo;

  CPVRGUITimesInfo m_timesInfo;

  bool m_bHasTVRecordings;
  bool m_bHasRadioRecordings;
  unsigned int m_iCurrentActiveClient;
  std::string m_strPlayingClientName;
  std::string m_strBackendName;
  std::string m_strBackendVersion;
  std::string m_strBackendHost;
  std::string m_strBackendTimers;
  std::string m_strBackendRecordings;
  std::string m_strBackendDeletedRecordings;
  std::string m_strBackendProviders;
  std::string m_strBackendChannelGroups;
  std::string m_strBackendChannels;
  long long m_iBackendDiskTotal;
  long long m_iBackendDiskUsed;
  bool m_bIsPlayingTV;
  bool m_bIsPlayingRadio;
  bool m_bIsPlayingRecording;
  bool m_bIsPlayingEpgTag;
  bool m_bIsPlayingEncryptedStream;
  bool m_bHasTVChannels;
  bool m_bHasRadioChannels;
  bool m_bCanRecordPlayingChannel;
  bool m_bIsRecordingPlayingChannel;
  bool m_bIsPlayingActiveRecording;
  std::string m_strPlayingTVGroup;
  std::string m_strPlayingRadioGroup;

  //@}

  PVR_SIGNAL_STATUS m_qualityInfo; /*!< stream quality information */
  PVR_DESCRAMBLE_INFO m_descrambleInfo; /*!< stream descramble information */
  std::vector<SBackend> m_backendProperties;

  std::string m_channelNumberInput;
  bool m_previewAndPlayerShowInfo{false};

  mutable CCriticalSection m_critSection;

  /**
   * The various backend-related fields will only be updated when this
   * flag is set. This is done to limit the amount of unnecessary
   * backend querying when we're not displaying any of the queried
   * information.
   */
  mutable std::atomic<bool> m_updateBackendCacheRequested;

  bool m_bRegistered;
};
} // namespace PVR
