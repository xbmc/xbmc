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

#include "FileItem.h"
#include "PVRDatabase.h"
#include "threads/Thread.h"
#include "utils/Observer.h"
#include "windows/GUIWindowPVRCommon.h"

class CPVRClients;
class CPVRChannelGroupsContainer;
class CPVRChannelGroup;
class CPVRRecordings;
class CPVRTimers;
class CPVREpgContainer;

#define INFO_TOGGLE_TIME 1500

class CPVRManager : public Observer, private CThread
{
  friend class CPVRClients;

private:
  /*!
   * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
   */
  CPVRManager(void);

public:
  void Notify(const Observable &obs, const CStdString& msg);

  /*!
   * @brief Stop the PVRManager and destroy all objects it created.
   */
  virtual ~CPVRManager(void);

  /*!
   * @brief Get the instance of the PVRManager.
   * @return The PVRManager instance.
   */
  static CPVRManager *Get(void);

  /*!
   * @brief Get the channel groups container.
   * @return The groups container.
   */
  static CPVRChannelGroupsContainer *GetChannelGroups(void);

  /*!
   * @brief Get the EPG container.
   * @return The EPG container.
   */
  static CPVREpgContainer *GetEpg(void);

  /*!
   * @brief Get the recordings container.
   * @return The recordings container.
   */
  static CPVRRecordings *GetRecordings(void);

  /*!
   * @brief Get the timers container.
   * @return The timers container.
   */
  static CPVRTimers *GetTimers(void);

  /*!
   * @brief Get the timers container.
   * @return The timers container.
   */
  static CPVRClients *GetClients(void);

  /*!
   * @brief Clean up and destroy the PVRManager.
   */
  static void Destroy(void);

  /*!
   * @brief Start the PVRManager
   */
  void Start(void);

  /*!
   * @brief Stop the PVRManager and destroy all objects it created.
   */
  void Stop(void);

public:

  /*!
   * @brief Get the TV database.
   * @return The TV database.
   */
  CPVRDatabase *GetTVDatabase(void) { return &m_database; }

  /*!
   * @brief Updates the recordings and the "now" and "next" timers.
   */
  void UpdateRecordingsCache(void);

  /*!
   * @brief Get a GUIInfoManager character string.
   * @param dwInfo The string to get.
   * @return The requested string or an empty one if it wasn't found.
   */
  const char* TranslateCharInfo(DWORD dwInfo);

  /*!
   * @brief Get a GUIInfoManager integer.
   * @param dwInfo The integer to get.
   * @return The requested integer or 0 if it wasn't found.
   */
  int TranslateIntInfo(DWORD dwInfo);

  /*!
   * @brief Get a GUIInfoManager boolean.
   * @param dwInfo The boolean to get.
   * @return The requested boolean or false if it wasn't found.
   */
  bool TranslateBoolInfo(DWORD dwInfo);

  /*!
   * @brief Reset the TV database to it's initial state and delete all the data inside.
   * @param bShowProgress True to show a progress bar, false otherwise.
   */
  void ResetDatabase(bool bShowProgress = true);

  /*!
   * @brief Delete all EPG data from the database and reload it from the clients.
   */
  void ResetEPG(void);

  /*!
   * @brief Check if a TV channel, radio channel or recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlaying(void);

  /*!
   * @return True if the thread is stopped, false otherwise.
   */
  bool IsRunning(void) { return !m_bStop; }

  /*!
   * @brief Return the channel that is currently playing.
   * @param channel The channel or NULL if none is playing.
   * @return True if a channel is playing, false otherwise.
   */
  bool GetCurrentChannel(CPVRChannel *channel);

  /*!
   * @brief Return the EPG for the channel that is currently playing.
   * @param channel The EPG or NULL if no channel is playing.
   * @return The amount of results that was added or -1 if none.
   */
  int GetCurrentEpg(CFileItemList *results);

  /*!
   * @brief Check whether the PVRManager has fully started.
   * @return True if started, false otherwise.
   */
  bool IsStarted(void) const { return m_bLoaded; }

  bool PerformChannelSwitch(const CPVRChannel &channel, bool bPreview);

  bool IsRunningChannelScan(void);
  void CloseStream();
  bool OpenLiveStream(const CPVRChannel &tag);
  bool OpenRecordedStream(const CPVRRecording &tag);
  bool StartRecordingOnPlayingChannel(bool bOnOff);

  /*!
   * @brief Get the channel number of the previously selected channel.
   * @return The requested channel number or -1 if it wasn't found.
   */
  int GetPreviousChannel();

  /*!
   * @brief Check whether there are active timers.
   * @return True if there are active timers, false otherwise.
   */
  bool HasTimer() { return m_bHasTimers; }

  /*!
   * @brief Check whether there are active recordings.
   * @return True if there are active recordings, false otherwise.
   */
  bool IsRecording() { return m_bIsRecording; }

  /*!
   * @brief Set the current playing group, used to load the right channel.
   * @param group The new group.
   */
  void SetPlayingGroup(CPVRChannelGroup *group);

  /*!
   * @brief Get the current playing group, used to load the right channel.
   * @param bRadio True to get the current radio group, false to get the current TV group.
   * @return The current group or the group containing all channels if it's not set.
   */
  const CPVRChannelGroup *GetPlayingGroup(bool bRadio = false);

  /*!
   * @brief Let the background thread update the recordings list.
   */
  void TriggerRecordingsUpdate(void);

  /*!
   * @brief Let the background thread update the timer list.
   */
  void TriggerTimersUpdate(void);

  /*!
   * @brief Let the background thread update the channel list.
   */
  void TriggerChannelsUpdate(void);

  /*!
   * @brief Let the background thread update the channel groups list.
   */
  void TriggerChannelGroupsUpdate(void);

  /*!
   * @brief Update the channel that is currently active.
   * @param item The new channel.
   * @return True if it was updated correctly, false otherwise.
   */
  bool UpdateItem(CFileItem& item);

  /*!
   * @brief Switch to a channel given it's channel number.
   * @param channel The channel number to switch to.
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelSwitch(unsigned int channel);

  /*!
   * @brief Switch to the next channel in this group.
   * @param newchannel The new channel number after the switch.
   * @param preview If true, don't do the actual switch but just update channel pointers.
   *                Used to display event info while doing "fast channel switching"
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelUp(unsigned int *newchannel, bool preview = false);

  /*!
   * @brief Switch to the previous channel in this group.
   * @param newchannel The new channel number after the switch.
   * @param preview If true, don't do the actual switch but just update channel pointers.
   *                Used to display event info while doing "fast channel switching"
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelDown(unsigned int *newchannel, bool preview = false);

  /*!
   * @brief Get the total duration of the currently playing LiveTV item.
   * @return The total duration in milliseconds or NULL if no channel is playing.
   */
  int GetTotalTime();

  /*!
   * @brief Get the current position in milliseconds since the start of a LiveTV item.
   * @return The position in milliseconds or NULL if no channel is playing.
   */
  int GetStartTime();

  /*!
   * @brief Start playback on a channel.
   * @param channel The channel to start to play.
   * @param bPreview If true, open minimised.
   * @return True if playback was started, false otherwise.
   */
  bool StartPlayback(const CPVRChannel *channel, bool bPreview = false);

  /*!
   * @brief Get the currently playing EPG tag and update it if needed.
   * @return The currently playing EPG tag or NULL if there is none.
   */
  const CPVREpgInfoTag *GetPlayingTag(void);

protected:
  /*!
   * @brief PVR update and control thread.
   */
  virtual void Process();

  bool DisableIfNoClients(void);

  void UpdateWindow(PVRWindow window);

private:

  const char *CharInfoNowRecordingTitle(void);
  const char *CharInfoNowRecordingChannel(void);
  const char *CharInfoNowRecordingDateTime(void);
  const char *CharInfoNextRecordingTitle(void);
  const char *CharInfoNextRecordingChannel(void);
  const char *CharInfoNextRecordingDateTime(void);
  const char *CharInfoNextTimer(void);
  const char *CharInfoPlayingDuration(void);
  const char *CharInfoPlayingTime(void);

  /*!
   * @brief Reset all properties.
   */
  void ResetProperties(void);

  /*!
   * @brief Switch to the given channel.
   * @param channel The new channel.
   * @param bPreview Don't reset quality data if true.
   * @return True if the switch was successful, false otherwise.
   */
  bool PerformChannelSwitch(const CPVRChannel *channel, bool bPreview);

  /*!
   * @brief Called by ChannelUp() and ChannelDown() to perform a channel switch.
   * @param iNewChannelNumber The new channel number after the switch.
   * @param bPreview Preview window if true.
   * @param bUp Go one channel up if true, one channel down if false.
   * @return True if the switch was successful, false otherwise.
   */
  bool ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp);

  /*!
   * @brief Stop the EPG and PVR threads but do not remove their data.
   */
  void StopThreads(void);

  /*!
   * @brief Restart the EPG and PVR threads after they've been stopped by StopThreads()
   */
  void StartThreads(void);

  /*!
   * @brief Persist the current channel settings in the database.
   */
  void SaveCurrentChannelSettings(void);

  /*!
   * @brief Load the settings for the current channel from the database.
   */
  void LoadCurrentChannelSettings(void);

  /*!
   * @brief Continue playback on the last channel if it was stored in the database.
   * @return True if playback was continued, false otherwise.
   */
  bool ContinueLastChannel(void);

  /*!
   * @brief Clean up all data that was created by the PVRManager.
   */
  void Cleanup(void);

  /*!
   * @brief Update all channels.
   */
  void UpdateChannels(void);

  /*!
   * @brief Update all recordings.
   */
  void UpdateRecordings(void);

  /*!
   * @brief Update all timers.
   */
  void UpdateTimers(void);

  void UpdateTimersCache(void);
  void UpdateRecordingToggle(void);

  /** @name General PVRManager data */
  //@{
  static CPVRManager *            m_instance;                    /*!< singleton instance */
  CFileItem *                     m_currentFile;
  CPVRChannelGroupsContainer *    m_channelGroups;               /*!< pointer to the channel groups container */
  CPVREpgContainer *              m_epg;                         /*!< pointer to the EPG container */
  CPVRRecordings *                m_recordings;                  /*!< pointer to the recordings container */
  CPVRTimers *                    m_timers;                      /*!< pointer to the timers container */
  CPVRClients *                   m_addons;                      /*!< pointer to the pvr addon container */
  CPVRDatabase                    m_database;                    /*!< the database for all PVR related data */
  CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class */
  CCriticalSection                m_critSectionTriggers;         /*!< critical section for triggers */
  CCriticalSection                m_critSectionAddons;           /*!< critical section for addons */
  bool                            m_bFirstStart;                 /*!< true when the PVR manager was started first, false otherwise */
  bool                            m_bLoaded;

  bool                            m_bTriggerChannelsUpdate;      /*!< set to true to let the background thread update the channels list */
  bool                            m_bTriggerRecordingsUpdate;    /*!< set to true to let the background thread update the recordings list */
  bool                            m_bTriggerTimersUpdate;        /*!< set to true to let the background thread update the timer list */
  bool                            m_bTriggerChannelGroupsUpdate; /*!< set to true to let the background thread update the channel groups list */
  //@}

  /** @name GUIInfoManager data */
  //@{
  DWORD                           m_recordingToggleStart;        /*!< time to toggle currently running pvr recordings */
  unsigned int                    m_recordingToggleCurrent;      /*!< the current item showed by the GUIInfoManager */

  std::vector<CPVRTimerInfoTag *> m_NowRecording;
  const CPVRTimerInfoTag *        m_NextRecording;

  CStdString                      m_strPlayingDuration;
  CStdString                      m_strPlayingTime;
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
  //@}

  /*--- Previous Channel data ---*/
  int                             m_PreviousChannel[2];
  int                             m_PreviousChannelIndex;
  int                             m_LastChannel;
  unsigned int                    m_LastChannelChanged;

  /*--- Stream playback data ---*/
  CPVRChannelGroup *              m_currentRadioGroup;        /* The current selected radio channel group list */
  CPVRChannelGroup *              m_currentTVGroup;           /* The current selected TV channel group list */
};
