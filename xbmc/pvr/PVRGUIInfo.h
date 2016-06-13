#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "pvr/addons/PVRClients.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <atomic>
#include <memory>
#include <vector>

namespace EPG
{
  class CEpgInfoTag;
  typedef std::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;
}

namespace PVR
{
  class CPVRTimerInfoTag;
  class CPVRRecording;

  class CPVRGUIInfo : private CThread,
                      private Observer
  {
  public:
    CPVRGUIInfo(void);
    virtual ~CPVRGUIInfo(void);

    void Start(void);
    void Stop(void);

    void Notify(const Observable &obs, const ObservableMessage msg);

    bool TranslateBoolInfo(DWORD dwInfo) const;
    bool TranslateCharInfo(DWORD dwInfo, std::string &strValue) const;
    int TranslateIntInfo(DWORD dwInfo) const;

    /*!
     * @brief Get the total duration of the currently playing LiveTV item.
     * @return The total duration in milliseconds or NULL if no channel is playing.
     */
    int GetDuration(void) const;

    /*!
     * @brief Get the current position in milliseconds since the start of a LiveTV item.
     * @return The position in milliseconds or NULL if no channel is playing.
     */
    int GetStartTime(void) const;

    /*!
     * @brief Show the player info.
     * @param iTimeout Hide the player info after iTimeout seconds.
     * @todo not really the right place for this :-)
     */
    void ShowPlayerInfo(int iTimeout);

    /*!
     * @brief Clear the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Get the currently playing EPG tag.
     * @return The currently playing EPG tag or NULL if no EPG tag is playing.
     */
    EPG::CEpgInfoTagPtr GetPlayingTag() const;

    /*!
     * @brief Get playing TV group.
     * @return The currently playing TV group or NULL if no TV group is playing.
     */
    std::string GetPlayingTVGroup();

  private:
    void ResetProperties(void);
    void ClearQualityInfo(PVR_SIGNAL_STATUS &qualityInfo);
    void Process(void);

    void UpdatePlayingTag(void);
    void UpdateTimersCache(void);
    void UpdateBackendCache(void);
    void UpdateQualityData(void);
    void UpdateMisc(void);
    void UpdateNextTimer(void);
    void UpdateTimeshift(void);

    bool TimerInfoToggle(void);
    void UpdateTimersToggle(void);
    void ToggleShowInfo(void);

    void CharInfoActiveTimerTitle(std::string &strValue) const;
    void CharInfoActiveTimerChannelName(std::string &strValue) const;
    void CharInfoActiveTimerChannelIcon(std::string &strValue) const;
    void CharInfoActiveTimerDateTime(std::string &strValue) const;
    void CharInfoNextTimerTitle(std::string &strValue) const;
    void CharInfoNextTimerChannelName(std::string &strValue) const;
    void CharInfoNextTimerChannelIcon(std::string &strValue) const;
    void CharInfoNextTimerDateTime(std::string &strValue) const;
    void CharInfoPlayingDuration(std::string &strValue) const;
    void CharInfoPlayingTime(std::string &strValue) const;
    void CharInfoNextTimer(std::string &strValue) const;
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
    void CharInfoTimeshiftStartTime(std::string &strValue) const;
    void CharInfoTimeshiftEndTime(std::string &strValue) const;
    void CharInfoTimeshiftPlayTime(std::string &strValue) const;

    /** @name GUIInfoManager data */
    //@{
    std::string                     m_strActiveTimerTitle;
    std::string                     m_strActiveTimerChannelName;
    std::string                     m_strActiveTimerChannelIcon;
    std::string                     m_strActiveTimerTime;
    std::string                     m_strNextTimerInfo;
    std::string                     m_strNextRecordingTitle;
    std::string                     m_strNextRecordingChannelName;
    std::string                     m_strNextRecordingChannelIcon;
    std::string                     m_strNextRecordingTime;
    bool                            m_bHasTVRecordings;
    bool                            m_bHasRadioRecordings;
    unsigned int                    m_iTimerAmount;
    unsigned int                    m_iRecordingTimerAmount;
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

    bool                            m_bHasNonRecordingTimers;
    bool                            m_bIsPlayingTV;
    bool                            m_bIsPlayingRadio;
    bool                            m_bIsPlayingRecording;
    bool                            m_bIsPlayingEncryptedStream;
    bool                            m_bHasTVChannels;
    bool                            m_bHasRadioChannels;
    std::string                     m_strPlayingTVGroup;
    //@}

    PVR_SIGNAL_STATUS               m_qualityInfo;       /*!< stream quality information */
    unsigned int                    m_iTimerInfoToggleStart;
    unsigned int                    m_iTimerInfoToggleCurrent;
    XbmcThreads::EndTime            m_ToggleShowInfo;
    EPG::CEpgInfoTagPtr             m_playingEpgTag;
    std::vector<SBackend>           m_backendProperties;

    bool                            m_bIsTimeshifting;
    time_t                          m_iTimeshiftStartTime;
    time_t                          m_iTimeshiftEndTime;
    time_t                          m_iTimeshiftPlayTime;
    std::string                     m_strTimeshiftStartTime;
    std::string                     m_strTimeshiftEndTime;
    std::string                     m_strTimeshiftPlayTime;

    CCriticalSection                m_critSection;

    /**
     * The various backend-related fields will only be updated when this
     * flag is set. This is done to limit the amount of unnecessary
     * backend querying when we're not displaying any of the queried
     * information.
     */
    mutable std::atomic<bool>       m_updateBackendCacheRequested;
  };
}
