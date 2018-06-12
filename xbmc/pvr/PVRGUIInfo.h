/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/guiinfo/GUIInfoProvider.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "pvr/PVRGUITimerInfo.h"
#include "pvr/PVRTypes.h"
#include "pvr/addons/PVRClients.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
  class CGUIInfo;
}
}
}

namespace PVR
{
  class CPVRGUIInfo : public KODI::GUILIB::GUIINFO::CGUIInfoProvider, private CThread, private Observer
  {
  public:
    CPVRGUIInfo(void);
    ~CPVRGUIInfo(void) override;

    void Start(void);
    void Stop(void);

    void Notify(const Observable &obs, const ObservableMessage msg) override;

    // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
    bool InitCurrentItem(CFileItem *item) override;
    bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const KODI::GUILIB::GUIINFO::CGUIInfo &info, std::string *fallback) const override;
    bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const KODI::GUILIB::GUIINFO::CGUIInfo &info) const override;
    bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const KODI::GUILIB::GUIINFO::CGUIInfo &info) const override;

    /*!
     * @brief Clear the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Get the currently playing EPG tag.
     * @return The currently playing EPG tag or NULL if no EPG tag is playing.
     */
    CPVREpgInfoTagPtr GetPlayingTag() const;

  private:
    void ResetProperties(void);
    void ClearQualityInfo(PVR_SIGNAL_STATUS &qualityInfo);
    void ClearDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleInfo);

    void Process(void) override;

    void UpdatePlayingTag(void);
    void UpdateTimersCache(void);
    void UpdateBackendCache(void);
    void UpdateQualityData(void);
    void UpdateDescrambleData(void);
    void UpdateMisc(void);
    void UpdateNextTimer(void);
    void UpdateTimeshift(void);

    void UpdateTimersToggle(void);

    bool GetListItemAndPlayerLabel(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, std::string &strValue) const;
    bool GetPVRLabel(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, std::string &strValue) const;
    bool GetRadioRDSLabel(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, std::string &strValue) const;

    bool GetListItemAndPlayerInt(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, int &iValue) const;
    bool GetPVRInt(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, int& iValue) const;

    bool GetListItemAndPlayerBool(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, bool &bValue) const;
    bool GetPVRBool(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, bool& bValue) const;
    bool GetRadioRDSBool(const CFileItem *item, const KODI::GUILIB::GUIINFO::CGUIInfo &info, bool &bValue) const;

    void CharInfoEpgEventDuration(const CFileItem *item, TIME_FORMAT format, std::string &strValue) const;
    void CharInfoEpgEventElapsedTime(const CFileItem *item, TIME_FORMAT format, std::string &strValue) const;
    void CharInfoEpgEventRemainingTime(const CFileItem *item, TIME_FORMAT format, std::string &strValue) const;
    void CharInfoEpgEventFinishTime(const CFileItem *item, TIME_FORMAT format, std::string &strValue) const;
    void CharInfoBackendNumber(std::string &strValue) const;
    void CharInfoTotalDiskSpace(std::string &strValue) const;
    void CharInfoSignal(std::string &strValue) const;
    void CharInfoSNR(std::string &strValue) const;
    void CharInfoBER(std::string &strValue) const;
    void CharInfoUNC(std::string &strValue) const;
    void CharInfoFrontendName(std::string &strValue) const;
    void CharInfoFrontendStatus(std::string &strValue) const;
    void CharInfoBackendName(std::string &strValue) const;
    void CharInfoBackendVersion(std::string &strValue) const;
    void CharInfoBackendHost(std::string &strValue) const;
    void CharInfoBackendDiskspace(std::string &strValue) const;
    void CharInfoBackendChannels(std::string &strValue) const;
    void CharInfoBackendTimers(std::string &strValue) const;
    void CharInfoBackendRecordings(std::string &strValue) const;
    void CharInfoBackendDeletedRecordings(std::string &strValue) const;
    void CharInfoPlayingClientName(std::string &strValue) const;
    void CharInfoEncryption(std::string &strValue) const;
    void CharInfoService(std::string &strValue) const;
    void CharInfoMux(std::string &strValue) const;
    void CharInfoProvider(std::string &strValue) const;
    void CharInfoTimeshiftStartTime(TIME_FORMAT format, std::string &strValue) const;
    void CharInfoTimeshiftEndTime(TIME_FORMAT format, std::string &strValue) const;
    void CharInfoTimeshiftPlayTime(TIME_FORMAT format, std::string &strValue) const;
    void CharInfoTimeshiftOffset(TIME_FORMAT format, std::string &strValue) const;

    /*!
     * @brief Get the elapsed time since the start of the currently playing epg event or if
     *        no epg is available since the start of the playback of the current Live TV stream.
     * @return The time in seconds or 0 if no channel is playing.
     */
    int GetElapsedTime(void) const;

    int GetRemainingTime(const CFileItem *item) const;

    /** @name PVRGUIInfo data */
    //@{
    CPVRGUIAnyTimerInfo   m_anyTimersInfo; // tv + radio
    CPVRGUITVTimerInfo    m_tvTimersInfo;
    CPVRGUIRadioTimerInfo m_radioTimersInfo;

    bool                            m_bHasTVRecordings;
    bool                            m_bHasRadioRecordings;
    unsigned int                    m_iCurrentActiveClient;
    std::string                     m_strPlayingClientName;
    std::string                     m_strBackendName;
    std::string                     m_strBackendVersion;
    std::string                     m_strBackendHost;
    std::string                     m_strBackendTimers;
    std::string                     m_strBackendRecordings;
    std::string                     m_strBackendDeletedRecordings;
    std::string                     m_strBackendChannels;
    long long                       m_iBackendDiskTotal;
    long long                       m_iBackendDiskUsed;
    unsigned int                    m_iDuration;
    bool                            m_bIsPlayingTV;
    bool                            m_bIsPlayingRadio;
    bool                            m_bIsPlayingRecording;
    bool                            m_bIsPlayingEpgTag;
    bool                            m_bIsPlayingEncryptedStream;
    bool                            m_bHasTVChannels;
    bool                            m_bHasRadioChannels;
    bool                            m_bCanRecordPlayingChannel;
    bool                            m_bIsRecordingPlayingChannel;
    std::string                     m_strPlayingTVGroup;
    std::string                     m_strPlayingRadioGroup;

    //@}

    PVR_SIGNAL_STATUS               m_qualityInfo;       /*!< stream quality information */
    PVR_DESCRAMBLE_INFO             m_descrambleInfo;    /*!< stream descramble information */
    CPVREpgInfoTagPtr               m_playingEpgTag;
    std::vector<SBackend>           m_backendProperties;

    bool                            m_bHasTimeshiftData;
    bool                            m_bIsTimeshifting;
    time_t                          m_iLastTimeshiftUpdate;
    time_t                          m_iStartTime;
    time_t                          m_iTimeshiftStartTime;
    time_t                          m_iTimeshiftEndTime;
    time_t                          m_iTimeshiftPlayTime;
    unsigned int                    m_iTimeshiftOffset;

    CCriticalSection                m_critSection;

    /**
     * The various backend-related fields will only be updated when this
     * flag is set. This is done to limit the amount of unnecessary
     * backend querying when we're not displaying any of the queried
     * information.
     */
    mutable std::atomic<bool>       m_updateBackendCacheRequested;

    bool m_bRegistered;
  };
}
