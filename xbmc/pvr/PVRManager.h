/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "interfaces/IAnnouncer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/guilib/PVRGUIActionListener.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/EventStream.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CStopWatch;

namespace PVR
{
  class CPVRChannel;
  class CPVRChannelGroup;
  class CPVRChannelGroupsContainer;
  class CPVRProviders;
  class CPVRClient;
  class CPVRClients;
  class CPVRDatabase;
  class CPVRGUIActions;
  class CPVRGUIInfo;
  class CPVRGUIProgressHandler;
  class CPVRManagerJobQueue;
  class CPVRPlaybackState;
  class CPVRRecording;
  class CPVRRecordings;
  class CPVRTimers;

  enum class PVREvent
  {
    // PVR Manager states
    ManagerError = 0,
    ManagerStopped,
    ManagerStarting,
    ManagerStopping,
    ManagerInterrupted,
    ManagerStarted,

    // Channel events
    ChannelPlaybackStopped,

    // Channel group events
    ChannelGroup,
    ChannelGroupInvalidated,
    ChannelGroupsInvalidated,
    ChannelGroupsLoaded,

    // Recording events
    RecordingsInvalidated,

    // Timer events
    AnnounceReminder,
    Timers,
    TimersInvalidated,

    // EPG events
    Epg,
    EpgActiveItem,
    EpgContainer,
    EpgItemUpdate,
    EpgUpdatePending,
    EpgDeleted,

    // Item events
    CurrentItem,

    // Syetem events
    SystemSleep,
    SystemWake,
  };

  class CPVRManager : private CThread, public ANNOUNCEMENT::IAnnouncer
  {
  public:
    /*!
     * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
     */
    CPVRManager();

    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    ~CPVRManager() override;

    void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data) override;

    /*!
     * @brief Get the providers container.
     * @return The providers container.
     */
    std::shared_ptr<CPVRProviders> Providers() const;

    /*!
     * @brief Get the channel groups container.
     * @return The groups container.
     */
    std::shared_ptr<CPVRChannelGroupsContainer> ChannelGroups() const;

    /*!
     * @brief Get the recordings container.
     * @return The recordings container.
     */
    std::shared_ptr<CPVRRecordings> Recordings() const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    std::shared_ptr<CPVRTimers> Timers() const;

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    std::shared_ptr<CPVRClients> Clients() const;

    /*!
     * @brief Get the instance of a client that matches the given item.
     * @param item The item containing a PVR recording, a PVR channel, a PVR timer or a PVR EPG event.
     * @return the requested client on success, nullptr otherwise.
     */
    std::shared_ptr<CPVRClient> GetClient(const CFileItem& item) const;

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
    std::shared_ptr<CPVRGUIActions> GUIActions() const;

    /*!
     * @brief Get access to the pvr playback state.
     * @return The playback state.
     */
    std::shared_ptr<CPVRPlaybackState> PlaybackState() const;

    /*!
     * @brief Get access to the epg container.
     * @return The epg container.
     */
    CPVREpgContainer& EpgContainer();

    /*!
     * @brief Init PVRManager.
     */
    void Init();

    /*!
     * @brief Start the PVRManager, which loads all PVR data and starts some threads to update the PVR data.
     */
    void Start();

    /*!
     * @brief Stop PVRManager.
     */
    void Stop(bool bRestart = false);

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
    std::shared_ptr<CPVRDatabase> GetTVDatabase() const;

    /*!
     * @return True while the PVRManager is initialising.
     */
    inline bool IsInitialising() const
    {
      return GetState() == ManagerStateStarting;
    }

    /*!
     * @brief Check whether the PVRManager has fully started.
     * @return True if started, false otherwise.
     */
    inline bool IsStarted() const
    {
      return GetState() == ManagerStateStarted;
    }

    /*!
     * @brief Check whether the PVRManager is stopping
     * @return True while the PVRManager is stopping.
     */
    inline bool IsStopping() const
    {
      return GetState() == ManagerStateStopping;
    }

    /*!
     * @brief Check whether the PVRManager has been stopped.
     * @return True if stopped, false otherwise.
     */
    inline bool IsStopped() const
    {
      return GetState() == ManagerStateStopped;
    }

    /*!
     * @brief Check whether EPG tags for channels have been created.
     * @return True if EPG tags have been created, false otherwise.
     */
    bool EpgsCreated() const;

    /*!
     * @brief Inform PVR manager that playback of an item just started.
     * @param item The item that started to play.
     */
    void OnPlaybackStarted(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Inform PVR manager that playback of an item was stopped due to user interaction.
     * @param item The item that stopped to play.
     */
    void OnPlaybackStopped(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Inform PVR manager that playback of an item has stopped without user interaction.
     * @param item The item that ended to play.
     */
    void OnPlaybackEnded(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Let the background thread create epg tags for all channels.
     */
    void TriggerEpgsCreate();

    /*!
     * @brief Let the background thread update the recordings list.
     */
    void TriggerRecordingsUpdate();

    /*!
     * @brief Let the background thread update the size for any in progress recordings.
     */
    void TriggerRecordingsSizeInProgressUpdate();

    /*!
     * @brief Let the background thread update the timer list.
     */
    void TriggerTimersUpdate();

    /*!
     * @brief Let the background thread update the channel list.
     */
    void TriggerChannelsUpdate();

    /*!
     * @brief Let the background thread update the provider list.
     */
    void TriggerProvidersUpdate();

    /*!
     * @brief Let the background thread update the channel groups list.
     */
    void TriggerChannelGroupsUpdate();

    /*!
     * @brief Let the background thread search for all missing channel icons.
     */
    void TriggerSearchMissingChannelIcons();

    /*!
     * @brief Let the background thread erase stale texture db entries and image files.
     */
    void TriggerCleanupCachedImages();

    /*!
     * @brief Let the background thread search for missing channel icons for channels contained in the given group.
     * @param group The channel group.
     */
    void TriggerSearchMissingChannelIcons(const std::shared_ptr<CPVRChannelGroup>& group);

    /*!
     * @brief Check whether names are still correct after the language settings changed.
     */
    void LocalizationChanged();

    /*!
     * @brief Check if parental lock is overridden at the given moment.
     * @param channel The channel to check.
     * @return True if parental lock is overridden, false otherwise.
     */
    bool IsParentalLocked(const std::shared_ptr<CPVRChannel>& channel) const;

    /*!
     * @brief Check if parental lock is overridden at the given moment.
     * @param epgTag The epg tag to check.
     * @return True if parental lock is overridden, false otherwise.
     */
    bool IsParentalLocked(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const;

    /*!
     * @brief Restart the parental timer.
     */
    void RestartParentalTimer();

    /*!
     * @brief Create EPG tags for all channels in internal channel groups
     * @return True if EPG tags where created successfully, false otherwise
     */
    bool CreateChannelEpgs();

    /*!
     * @brief Signal a connection change of a client
     */
    void ConnectionStateChange(CPVRClient* client,
                               const std::string& connectString,
                               PVR_CONNECTION_STATE state,
                               const std::string& message);

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
    void Process() override;

  private:
    /*!
     * @brief Executes "pvrpowermanagement.setwakeupcmd"
     */
    bool SetWakeupCommand();

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
    void ResetProperties();

    /*!
     * @brief Destroy PVRManager's objects.
     */
    void Clear();

    /*!
     * @brief Continue playback on the last played channel.
     */
    void TriggerPlayChannelOnStartup();

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
    ManagerState GetState() const;

    /*!
     * @brief Set the current state of the PVR manager.
     * @param state the new state.
     */
    void SetState(ManagerState state);

    bool IsCurrentlyParentalLocked(const std::shared_ptr<CPVRChannel>& channel, bool bGenerallyLocked) const;

    /** @name containers */
    //@{
    std::shared_ptr<CPVRProviders> m_providers; /*!< pointer to the providers container */
    std::shared_ptr<CPVRChannelGroupsContainer> m_channelGroups; /*!< pointer to the channel groups container */
    std::shared_ptr<CPVRRecordings> m_recordings; /*!< pointer to the recordings container */
    std::shared_ptr<CPVRTimers> m_timers; /*!< pointer to the timers container */
    std::shared_ptr<CPVRClients> m_addons; /*!< pointer to the pvr addon container */
    std::unique_ptr<CPVRGUIInfo> m_guiInfo; /*!< pointer to the guiinfo data */
    std::shared_ptr<CPVRGUIActions> m_guiActions; /*!< pointer to the pvr gui actions */
    CPVREpgContainer m_epgContainer; /*!< the epg container */
    //@}

    std::unique_ptr<CPVRManagerJobQueue> m_pendingUpdates; /*!< vector of pending pvr updates */
    std::shared_ptr<CPVRDatabase> m_database; /*!< the database for all PVR related data */
    mutable CCriticalSection m_critSection; /*!< critical section for all changes to this class, except for changes to triggers */
    bool m_bFirstStart = true; /*!< true when the PVR manager was started first, false otherwise */
    bool m_bEpgsCreated = false; /*!< true if epg data for channels has been created */

    mutable CCriticalSection m_managerStateMutex;
    ManagerState m_managerState = ManagerStateStopped;
    std::unique_ptr<CStopWatch> m_parentalTimer;

    CCriticalSection m_startStopMutex; // mutex for protecting pvr manager's start/restart/stop sequence */

    CEventSource<PVREvent> m_events;

    const std::shared_ptr<CPVRPlaybackState> m_playbackState;

    CPVRGUIActionListener m_actionListener;
    CPVRSettings m_settings;
  };
}
