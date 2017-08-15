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

#include <atomic>
#include <string>
#include <vector>

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "pvr/PVRTypes.h"
#include "pvr/addons/PVRClients.h"

namespace PVR
{
  class CPVRGUIInfo : private CThread,
                      private Observer
  {
  public:
    CPVRGUIInfo(void);
    ~CPVRGUIInfo(void) override;

    void Start(void);
    void Stop(void);

    void Notify(const Observable &obs, const ObservableMessage msg) override;

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
     * @brief Clear the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Get the currently playing EPG tag.
     * @return The currently playing EPG tag or NULL if no EPG tag is playing.
     */
    CPVREpgInfoTagPtr GetPlayingTag() const;

    /*!
     * @brief Get playing TV group.
     * @return The currently playing TV group or NULL if no TV group is playing.
     */
    std::string GetPlayingTVGroup();

  private:
    class TimerInfo
    {
    public:
      TimerInfo();
      virtual ~TimerInfo() = default;

      void ResetProperties();

      void UpdateTimersCache();
      void UpdateTimersToggle();
      void UpdateNextTimer();

      void CharInfoActiveTimerTitle(std::string &strValue) const { strValue = m_strActiveTimerTitle; }
      void CharInfoActiveTimerChannelName(std::string &strValue) const { strValue = m_strActiveTimerChannelName; }
      void CharInfoActiveTimerChannelIcon(std::string &strValue) const { strValue = m_strActiveTimerChannelIcon; }
      void CharInfoActiveTimerDateTime(std::string &strValue) const { strValue = m_strActiveTimerTime; }
      void CharInfoNextTimerTitle(std::string &strValue) const { strValue = m_strNextRecordingTitle; }
      void CharInfoNextTimerChannelName(std::string &strValue) const { strValue = m_strNextRecordingChannelName; }
      void CharInfoNextTimerChannelIcon(std::string &strValue) const { strValue = m_strNextRecordingChannelIcon; }
      void CharInfoNextTimerDateTime(std::string &strValue) const { strValue = m_strNextRecordingTime; }
      void CharInfoNextTimer(std::string &strValue) const { strValue = m_strNextTimerInfo; }

      bool HasTimers() const { return m_iTimerAmount > 0; }
      bool HasRecordingTimers() const { return m_iRecordingTimerAmount > 0; }
      bool HasNonRecordingTimers() const { return m_iTimerAmount - m_iRecordingTimerAmount > 0; }

    private:
      bool TimerInfoToggle();

      virtual int AmountActiveTimers() = 0;
      virtual int AmountActiveRecordings() = 0;
      virtual std::vector<CFileItemPtr> GetActiveRecordings() = 0;
      virtual CFileItemPtr GetNextActiveTimer() = 0;

      unsigned int m_iTimerAmount;
      unsigned int m_iRecordingTimerAmount;

      std::string m_strActiveTimerTitle;
      std::string m_strActiveTimerChannelName;
      std::string m_strActiveTimerChannelIcon;
      std::string m_strActiveTimerTime;
      std::string m_strNextRecordingTitle;
      std::string m_strNextRecordingChannelName;
      std::string m_strNextRecordingChannelIcon;
      std::string m_strNextRecordingTime;
      std::string m_strNextTimerInfo;

      unsigned int m_iTimerInfoToggleStart;
      unsigned int m_iTimerInfoToggleCurrent;

      CCriticalSection m_critSection;
    };

    class AnyTimerInfo : public TimerInfo
    {
    public:
      AnyTimerInfo() = default;

    private:
      int AmountActiveTimers() override;
      int AmountActiveRecordings() override;
      std::vector<CFileItemPtr> GetActiveRecordings() override;
      CFileItemPtr GetNextActiveTimer() override;
    };

    class TVTimerInfo : public TimerInfo
    {
    public:
      TVTimerInfo() = default;

    private:
      int AmountActiveTimers() override;
      int AmountActiveRecordings() override;
      std::vector<CFileItemPtr> GetActiveRecordings() override;
      CFileItemPtr GetNextActiveTimer() override;
    };

    class RadioTimerInfo : public TimerInfo
    {
    public:
      RadioTimerInfo() = default;

    private:
      int AmountActiveTimers() override;
      int AmountActiveRecordings() override;
      std::vector<CFileItemPtr> GetActiveRecordings() override;
      CFileItemPtr GetNextActiveTimer() override;
    };

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

    void CharInfoPlayingDuration(std::string &strValue) const;
    void CharInfoPlayingTime(std::string &strValue) const;
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
    AnyTimerInfo   m_anyTimersInfo; // tv + radio
    TVTimerInfo    m_tvTimersInfo;
    RadioTimerInfo m_radioTimersInfo;

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
    bool                            m_bIsPlayingEncryptedStream;
    bool                            m_bHasTVChannels;
    bool                            m_bHasRadioChannels;
    std::string                     m_strPlayingTVGroup;
    //@}

    PVR_SIGNAL_STATUS               m_qualityInfo;       /*!< stream quality information */
    PVR_DESCRAMBLE_INFO             m_descrambleInfo;    /*!< stream descramble information */
    CPVREpgInfoTagPtr               m_playingEpgTag;
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
