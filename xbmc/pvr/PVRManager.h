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
#include <memory>
#include <string>
#include <vector>

#include "FileItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "interfaces/IAnnouncer.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/EventStream.h"
#include "utils/JobManager.h"
#include "utils/Observer.h"

#include "pvr/PVRActionListener.h"
#include "pvr/PVREvent.h"
#include "pvr/PVRSettings.h"
#include "pvr/PVRTypes.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/recordings/PVRRecording.h"

class CStopWatch;
class CVariant;

namespace PVR
{
  class CPVRClient;
  class CPVRGUIInfo;
  class CPVRGUIProgressHandler;

  class CPVRManagerJobQueue
  {
  public:
    CPVRManagerJobQueue();

    void Start();
    void Stop();
    void Clear();

    void AppendJob(CJob * job);
    void ExecutePendingJobs();
    bool WaitForJobs(unsigned int milliSeconds);

  private:
    CCriticalSection m_critSection;
    CEvent m_triggerEvent;
    std::vector<CJob *> m_pendingUpdates;
    bool m_bStopped = true;
  };

  class CPVRManager : private CThread, public Observable, public ANNOUNCEMENT::IAnnouncer
  {
  public:
    /*!
     * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
     */
    CPVRManager(void);

    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    ~CPVRManager(void) override;

    void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

    /*!
     * @brief Get the channel groups container.
     * @return The groups container.
     */
    CPVRChannelGroupsContainerPtr ChannelGroups(void) const;

    /*!
     * @brief Get the recordings container.
     * @return The recordings container.
     */
    CPVRRecordingsPtr Recordings(void) const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRTimersPtr Timers(void) const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRClientsPtr Clients(void) const;

    /*!
     * @brief Get the instance of a client that matches the given item.
     * @param item The item containing a PVR recording, a PVR channel, a PVR timer or a PVR EPG event.
     * @return the requested client on success, nullptr otherwise.
     */
    std::shared_ptr<CPVRClient> GetClient(const CFileItem &item) const;

    /*!
     * @brief Get the instance of a client that matches the given id.
     * @param iClientId The id of a PVR client.
     * @return the requested client on success, nullptr otherwise.
     */
    std::shared_ptr<CPVRClient> GetClient(int iClientId) const;

    /*!
     * @brief Get access to the pvr gui actions.
     * @return The gui actions.
     */
    CPVRGUIActionsPtr GUIActions(void) const;

    /*!
     * @brief Get access to the epg container.
     * @return The epg container.
     */
    CPVREpgContainer& EpgContainer();

    /*!
     * @brief Init PVRManager.
     */
    void Init(void);

    /*!
     * @brief Start the PVRManager, which loads all PVR data and starts some threads to update the PVR data.
     */
    void Start();

    /*!
     * @brief Stop PVRManager.
     */
    void Stop(void);

    /*!
     * @brief Stop PVRManager, unload data.
     */
    void Unload();

    /*!
     * @brief Deinit PVRManager, unload data, unload addons.
     */
    void Deinit();

    /*!
     * @brief Propagate event on system sleep
     */
    void OnSleep();

    /*!
     * @brief Propagate event on system wake
     */
    void OnWake();

    /*!
     * @brief Get the TV database.
     * @return The TV database.
     */
    CPVRDatabasePtr GetTVDatabase(void) const;

    /*!
     * @brief Check if a TV channel, radio channel or recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @brief Check if the given channel is playing.
     * @param channel The channel to check.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Check if the given recording is playing.
     * @param recording The recording to check.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(const CPVRRecordingPtr &recording) const;

    /*!
     * @brief Check if the given epg tag is playing.
     * @param epgTag The tag to check.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingEpgTag(const CPVREpgInfoTagPtr &epgTag) const;

    /*!
     * @brief Check whether the currently playing livetv stream is timeshifted.
     * @return True if there is a playing stream and if it is timeshifted, false otherwise.
     */
    bool IsTimeshifting() const;

    /*!
     * @return True while the PVRManager is initialising.
     */
    inline bool IsInitialising(void) const
    {
      return GetState() == ManagerStateStarting;
    }

    /*!
     * @brief Check whether the PVRManager has fully started.
     * @return True if started, false otherwise.
     */
    inline bool IsStarted(void) const
    {
      return GetState() == ManagerStateStarted;
    }

    /*!
     * @brief Check whether the PVRManager is stopping
     * @return True while the PVRManager is stopping.
     */
    inline bool IsStopping(void) const
    {
      return GetState() == ManagerStateStopping;
    }

    /*!
     * @brief Check whether the PVRManager has been stopped.
     * @return True if stopped, false otherwise.
     */
    inline bool IsStopped(void) const
    {
      return GetState() == ManagerStateStopped;
    }

    /*!
     * @brief Return the channel that is currently playing.
     * @return The channel or NULL if none is playing.
     */
    CPVRChannelPtr GetPlayingChannel(void) const;

    /*!
     * @brief Return the recording that is currently playing.
     * @return The recording or NULL if none is playing.
     */
    CPVRRecordingPtr GetPlayingRecording(void) const;

    /*!
     * @brief Return the epg tag that is currently playing.
     * @return The tag or NULL if none is playing.
     */
    CPVREpgInfoTagPtr GetPlayingEpgTag(void) const;

    /*!
     * @brief Get the name of the playing client, if there is one.
     * @return The name of the client or an empty string if nothing is playing.
     */
    std::string GetPlayingClientName(void) const;

    /*!
     * @brief Get the ID of the playing client, if there is one.
     * @return The ID or -1 if no client is playing.
     */
    int GetPlayingClientID(void) const;

    /*!
     * @brief Check whether there is an active recording on the currenlyt playing channel.
     * @return True if there is a playing channel and there is an active recording on that channel, false otherwise.
     */
    bool IsRecordingOnPlayingChannel(void) const;

    /*!
     * @brief Check whether the currently playing channel can be recorded.
     * @return True if there is a playing channel that can be recorded, false otherwise.
     */
    bool CanRecordOnPlayingChannel(void) const;

    /*!
     * @brief Check whether EPG tags for channels have been created.
     * @return True if EPG tags have been created, false otherwise.
     */
    bool EpgsCreated(void) const;

    /*!
     * @brief Reset the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Inform PVR manager that playback of an item just started.
     * @param item The item that started to play.
     */
    void OnPlaybackStarted(const CFileItemPtr item);

    /*!
     * @brief Inform PVR manager that playback of an item was stopped due to user interaction.
     * @param item The item that stopped to play.
     */
    void OnPlaybackStopped(const CFileItemPtr item);

    /*!
     * @brief Inform PVR manager that playback of an item has stopped without user interaction.
     * @param item The item that ended to play.
     */
    void OnPlaybackEnded(const CFileItemPtr item);

    /*!
     * @brief Check whether there are active recordings.
     * @return True if there are active recordings, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Set the current playing group, used to load the right channel.
     * @param group The new group.
     */
    void SetPlayingGroup(const CPVRChannelGroupPtr &group);

    /*!
     * @brief Get the current playing group, used to load the right channel.
     * @param bRadio True to get the current radio group, false to get the current TV group.
     * @return The current group or the group containing all channels if it's not set.
     */
    CPVRChannelGroupPtr GetPlayingGroup(bool bRadio = false);

    /*!
     * @brief Fill the file item for a recording, a channel or an epg tag with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled. Item must contain either a pvr recording, a pvr channel or an epg tag.
     * @return True if the stream properties have been set, false otherwiese.
     */
    bool FillStreamFileItem(CFileItem &fileItem);

    /*!
     * @brief Let the background thread create epg tags for all channels.
     */
    void TriggerEpgsCreate(void);

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
     * @brief Let the background thread search for missing channel icons.
     */
    void TriggerSearchMissingChannelIcons(void);

    /*!
     * @brief Check whether names are still correct after the language settings changed.
     */
    void LocalizationChanged(void);

    /*!
     * @brief Check if a TV channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @brief Check if a radio channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @brief Check if a an encrypted TV or radio channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingEncryptedChannel(void) const;

    /*!
     * @brief Check if a recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @brief Check if an epg tag is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingEpgTag(void) const;

    /*!
     * @brief Try to find missing channel icons automatically
     */
    void SearchMissingChannelIcons(void);

    /*!
     * @brief Check if parental lock is overridden at the given moment.
     * @param channel The channel to open.
     * @return True if parental lock is overridden, false otherwise.
     */
    bool IsParentalLocked(const CPVRChannelPtr &channel);

    /*!
     * @brief Restart the parental timer.
     */
    void RestartParentalTimer();

    /*!
     * @brief Create EPG tags for all channels in internal channel groups
     * @return True if EPG tags where created successfully, false otherwise
     */
    bool CreateChannelEpgs(void);

    /*!
     * @brief Signal a connection change of a client
     */
    void ConnectionStateChange(CPVRClient *client, std::string connectString, PVR_CONNECTION_STATE state, std::string message);

    /*!
     * @brief Query the events available for CEventStream
     */
    CEventStream<PVREvent>& Events() { return m_events; }

    /*!
     * @brief Publish an event
     * @param state the event
     */
    void PublishEvent(PVREvent state);

  protected:
    /*!
     * @brief PVR update and control thread.
     */
    void Process(void) override;

  private:
    /*!
     * @brief Updates the last watched timestamps of the channel and group which are currently playing.
     * @param channel The channel which is updated
     */
    void UpdateLastWatched(const CPVRChannelPtr &channel);

    /*!
     * @brief Set the playing group to the first group the channel is in if the given channel is not part of the current playing group
     * @param channel The channel
     */
    void SetPlayingGroup(const CPVRChannelPtr &channel);

    /*!
     * @brief Executes "pvrpowermanagement.setwakeupcmd"
     */
    bool SetWakeupCommand(void);

    /*!
     * @brief Load at least one client and load all other PVR data (channelgroups, timers, recordings) after loading the client.
     * @param progressHandler The progress handler to use for showing the different load stages.
     * @return If at least one client and all pvr data was loaded, false otherwise.
     */
    bool LoadComponents(CPVRGUIProgressHandler* progressHandler);

    /*!
     * @brief Unload all PVR data (recordings, timers, channelgroups).
     */
    void UnloadComponents();

    /*!
     * @brief Reset all properties.
     */
    void ResetProperties(void);

    /*!
     * @brief Destroy PVRManager's objects.
     */
    void Clear(void);

    /*!
     * @brief Continue playback on the last played channel.
     */
    void TriggerPlayChannelOnStartup(void);

    enum ManagerState
    {
      ManagerStateError = 0,
      ManagerStateStopped,
      ManagerStateStarting,
      ManagerStateStopping,
      ManagerStateInterrupted,
      ManagerStateStarted
    };

    /*!
     * @brief Get the current state of the PVR manager.
     * @return the state.
     */
    ManagerState GetState(void) const;

    /*!
     * @brief Set the current state of the PVR manager.
     * @param state the new state.
     */
    void SetState(ManagerState state);

    /** @name containers */
    //@{
    CPVRChannelGroupsContainerPtr  m_channelGroups;               /*!< pointer to the channel groups container */
    CPVRRecordingsPtr              m_recordings;                  /*!< pointer to the recordings container */
    CPVRTimersPtr                  m_timers;                      /*!< pointer to the timers container */
    CPVRClientsPtr                 m_addons;                      /*!< pointer to the pvr addon container */
    std::unique_ptr<CPVRGUIInfo>   m_guiInfo;                     /*!< pointer to the guiinfo data */
    CPVRGUIActionsPtr              m_guiActions;                  /*!< pointer to the pvr gui actions */
    CPVREpgContainer               m_epgContainer;                /*!< the epg container */
    //@}

    CPVRManagerJobQueue             m_pendingUpdates;              /*!< vector of pending pvr updates */

    CPVRDatabasePtr                 m_database;                    /*!< the database for all PVR related data */
    CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
    bool                            m_bFirstStart = true;                 /*!< true when the PVR manager was started first, false otherwise */
    bool                            m_bEpgsCreated = false;                /*!< true if epg data for channels has been created */

    CCriticalSection                m_managerStateMutex;
    ManagerState                    m_managerState = ManagerStateStopped;
    std::unique_ptr<CStopWatch>     m_parentalTimer;

    CCriticalSection                m_startStopMutex; // mutex for protecting pvr manager's start/restart/stop sequence */

    CEventSource<PVREvent> m_events;

    CPVRActionListener m_actionListener;
    CPVRSettings m_settings;

    CPVRChannelPtr m_playingChannel;
    CPVRRecordingPtr m_playingRecording;
    CPVREpgInfoTagPtr m_playingEpgTag;
    std::string m_strPlayingClientName;
    int m_playingClientId = -1;
  };
}
