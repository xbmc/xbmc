#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "threads/Thread.h"
#include "addons/include/xbmc_pvr_types.h"

class CPVREpgInfoTag;
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

  void Notify(const Observable &obs, const CStdString& msg);

  bool TranslateBoolInfo(DWORD dwInfo) const;
  const char* TranslateCharInfo(DWORD dwInfo) const;
  int TranslateIntInfo(DWORD dwInfo) const;

  bool IsRecording(void) const { return m_bIsRecording; };
  bool HasTimers(void) const { return m_bHasTimers; };
  const CPVREpgInfoTag *GetPlayingTag(void) const { return m_playingEpgTag; }

  /*!
   * @brief Get the total duration of the currently playing LiveTV item.
   * @return The total duration in milliseconds or NULL if no channel is playing.
   */
  int GetTotalTime(void) const;

  /*!
   * @brief Get the current position in milliseconds since the start of a LiveTV item.
   * @return The position in milliseconds or NULL if no channel is playing.
   */
  int GetStartTime(void) const;

private:
  void ResetProperties(void);
  void Process(void);

  void UpdatePlayingTag(void);
  void UpdateTimersCache(void);
  void UpdateBackendCache(void);

  bool AddonInfoToggle(void);
  bool TimerInfoToggle(void);
  void UpdateTimersToggle(void);

  const char *CharInfoActiveTimerTitle(void) const;
  const char *CharInfoActiveTimerChannelName(void) const;
  const char *CharInfoActiveTimerDateTime(void) const;
  const char *CharInfoNextTimerTitle(void) const;
  const char *CharInfoNextTimerChannelName(void) const;
  const char *CharInfoNextTimerDateTime(void) const;
  const char *CharInfoPlayingDuration(void) const;
  const char *CharInfoPlayingTime(void) const;
  const char *CharInfoNextTimer(void) const;
  const char *CharInfoBackendNumber(void) const;
  const char *CharInfoTotalDiskSpace(void) const;
  const char *CharInfoVideoBR(void) const;
  const char *CharInfoAudioBR(void) const;
  const char *CharInfoDolbyBR(void) const;
  const char *CharInfoSignal(void) const;
  const char *CharInfoSNR(void) const;
  const char *CharInfoBER(void) const;
  const char *CharInfoUNC(void) const;
  const char *CharInfoFrontendName(void) const;
  const char *CharInfoFrontendStatus(void) const;
  const char *CharInfoBackendName(void) const;
  const char *CharInfoBackendVersion(void) const;
  const char *CharInfoBackendHost(void) const;
  const char *CharInfoBackendDiskspace(void) const;
  const char *CharInfoBackendChannels(void) const;
  const char *CharInfoBackendTimers(void) const;
  const char *CharInfoBackendRecordings(void) const;
  const char *CharInfoPlayingClientName(void) const;
  const char *CharInfoEncryption(void) const;

  /** @name GUIInfoManager data */
  //@{
  std::vector<CPVRTimerInfoTag *> m_NowRecording;
  const CPVRTimerInfoTag *        m_NextRecording;

  CStdString                      m_strActiveTimerTitle;
  CStdString                      m_strActiveTimerChannelName;
  CStdString                      m_strActiveTimerTime;
  CStdString                      m_strNextTimerInfo;
  CStdString                      m_strNextRecordingTitle;
  CStdString                      m_strNextRecordingChannelName;
  CStdString                      m_strNextRecordingTime;
  bool                            m_bIsRecording;
  bool                            m_bHasRecordings;
  bool                            m_bHasTimers;
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
  //@}

  PVR_SIGNAL_STATUS               m_qualityInfo;       /*!< stream quality information */
  unsigned int                    m_iAddonInfoToggleStart;
  unsigned int                    m_iAddonInfoToggleCurrent;
  unsigned int                    m_iTimerInfoToggleStart;
  unsigned int                    m_iTimerInfoToggleCurrent;
  mutable const CPVREpgInfoTag *  m_playingEpgTag;

  CCriticalSection                m_critSection;
};
