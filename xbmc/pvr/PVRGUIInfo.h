#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "threads/Thread.h"
#include "addons/include/xbmc_pvr_types.h"

namespace EPG
{
  class CEpgInfoTag;
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
    bool TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const;
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

    bool GetPlayingTag(EPG::CEpgInfoTag &tag) const;

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

    bool AddonInfoToggle(void);
    bool TimerInfoToggle(void);
    void UpdateTimersToggle(void);
    void ToggleShowInfo(void);

    void CharInfoActiveTimerTitle(CStdString &strValue) const;
    void CharInfoActiveTimerChannelName(CStdString &strValue) const;
    void CharInfoActiveTimerChannelIcon(CStdString &strValue) const;
    void CharInfoActiveTimerDateTime(CStdString &strValue) const;
    void CharInfoNextTimerTitle(CStdString &strValue) const;
    void CharInfoNextTimerChannelName(CStdString &strValue) const;
    void CharInfoNextTimerChannelIcon(CStdString &strValue) const;
    void CharInfoNextTimerDateTime(CStdString &strValue) const;
    void CharInfoPlayingDuration(CStdString &strValue) const;
    void CharInfoPlayingTime(CStdString &strValue) const;
    void CharInfoNextTimer(CStdString &strValue) const;
    void CharInfoBackendNumber(CStdString &strValue) const;
    void CharInfoTotalDiskSpace(CStdString &strValue) const;
    void CharInfoVideoBR(CStdString &strValue) const;
    void CharInfoAudioBR(CStdString &strValue) const;
    void CharInfoDolbyBR(CStdString &strValue) const;
    void CharInfoSignal(CStdString &strValue) const;
    void CharInfoSNR(CStdString &strValue) const;
    void CharInfoBER(CStdString &strValue) const;
    void CharInfoUNC(CStdString &strValue) const;
    void CharInfoFrontendName(CStdString &strValue) const;
    void CharInfoFrontendStatus(CStdString &strValue) const;
    void CharInfoBackendName(CStdString &strValue) const;
    void CharInfoBackendVersion(CStdString &strValue) const;
    void CharInfoBackendHost(CStdString &strValue) const;
    void CharInfoBackendDiskspace(CStdString &strValue) const;
    void CharInfoBackendChannels(CStdString &strValue) const;
    void CharInfoBackendTimers(CStdString &strValue) const;
    void CharInfoBackendRecordings(CStdString &strValue) const;
    void CharInfoPlayingClientName(CStdString &strValue) const;
    void CharInfoEncryption(CStdString &strValue) const;

    /** @name GUIInfoManager data */
    //@{
    CStdString                      m_strActiveTimerTitle;
    CStdString                      m_strActiveTimerChannelName;
    CStdString                      m_strActiveTimerChannelIcon;
    CStdString                      m_strActiveTimerTime;
    CStdString                      m_strNextTimerInfo;
    CStdString                      m_strNextRecordingTitle;
    CStdString                      m_strNextRecordingChannelName;
    CStdString                      m_strNextRecordingChannelIcon;
    CStdString                      m_strNextRecordingTime;
    bool                            m_bHasRecordings;
    unsigned int                    m_iTimerAmount;
    unsigned int                    m_iRecordingTimerAmount;
    int                             m_iActiveClients;
    CStdString                      m_strPlayingClientName;
    CStdString                      m_strBackendName;
    CStdString                      m_strBackendVersion;
    CStdString                      m_strBackendHost;
    CStdString                      m_strBackendDiskspace;
    CStdString                      m_strBackendTimers;
    CStdString                      m_strBackendRecordings;
    CStdString                      m_strBackendChannels;
    CStdString                      m_strTotalDiskspace;
    unsigned int                    m_iDuration;

    bool                            m_bHasNonRecordingTimers;
    bool                            m_bIsPlayingTV;
    bool                            m_bIsPlayingRadio;
    bool                            m_bIsPlayingRecording;
    bool                            m_bIsPlayingEncryptedStream;
    //@}

    PVR_SIGNAL_STATUS               m_qualityInfo;       /*!< stream quality information */
    unsigned int                    m_iAddonInfoToggleStart;
    unsigned int                    m_iAddonInfoToggleCurrent;
    unsigned int                    m_iTimerInfoToggleStart;
    unsigned int                    m_iTimerInfoToggleCurrent;
    unsigned int                    m_iToggleShowInfo;
    EPG::CEpgInfoTag *              m_playingEpgTag;

    CCriticalSection                m_critSection;
  };
}
