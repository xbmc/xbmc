/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/settings/PVRSettings.h"
#include "threads/Thread.h"

#include <map>
#include <memory>
#include <queue>
#include <vector>

class CDateTime;

namespace PVR
{
enum class TimerOperationResult;
enum class PVREvent;

class CPVRChannel;
class CPVRClient;
class CPVREpgInfoTag;
class CPVRTimerInfoTag;
class CPVRTimersPath;

class CPVRTimersContainer
{
public:
  /*!
   * @brief Add a timer tag to this container or update the tag if already present in this
   * container.
   * @param The timer tag
   * @return True, if the update was successful. False, otherwise.
   */
  bool UpdateFromClient(const std::shared_ptr<CPVRTimerInfoTag>& timer);

  /*!
   * @brief Get the timer tag denoted by given client id and timer id.
   * @param iClientId The client id.
   * @param iClientIndex The timer id.
   * @return the timer tag if found, null otherwise.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetByClient(int iClientId, int iClientIndex) const;

  typedef std::vector<std::shared_ptr<CPVRTimerInfoTag>> VecTimerInfoTag;
  typedef std::map<CDateTime, VecTimerInfoTag> MapTags;

  /*!
   * @brief Get the timertags map.
   * @return The map.
   */
  const MapTags& GetTags() const { return m_tags; }

protected:
  void InsertEntry(const std::shared_ptr<CPVRTimerInfoTag>& newTimer);

  mutable CCriticalSection m_critSection;
  unsigned int m_iLastId = 0;
  MapTags m_tags;
};

class CPVRTimers : public CPVRTimersContainer, private CThread
{
public:
  CPVRTimers();
  ~CPVRTimers() override = default;

  /*!
     * @brief start the timer update thread.
     */
  void Start();

  /*!
     * @brief stop the timer update thread.
     */
  void Stop();

  /*!
   * @brief Update all timers from PVR database and from given clients.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool Update(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief unload all timers.
   */
  void Unload();

  /*!
   * @brief Update data with recordings from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created
   * clients.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @param bIgnoreReminders include or ignore reminders
   * @return The tv or radio timer that will be active next (state scheduled), or nullptr if none.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer(bool bIgnoreReminders = true) const;

  /*!
   * @return The tv timer that will be active next (state scheduled), or nullptr if none.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTVTimer() const;

  /*!
   * @return The radio timer that will be active next (state scheduled), or nullptr if none.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetNextActiveRadioTimer() const;

  /*!
   * @return All timers that are active (states scheduled or recording)
   */
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveTimers() const;

  /*!
   * @return Next due reminder, if any. Removes it from the queue of due reminders.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetNextReminderToAnnnounce();

  /*!
   * Get all timers
   * @return The list of all timers
   */
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetAll() const;

  /*!
   * @return The amount of tv and radio timers that are active (states scheduled or recording)
   */
  int AmountActiveTimers() const;

  /*!
   * @return The amount of tv timers that are active (states scheduled or recording)
   */
  int AmountActiveTVTimers() const;

  /*!
   * @return The amount of radio timers that are active (states scheduled or recording)
   */
  int AmountActiveRadioTimers() const;

  /*!
   * @return All tv and radio timers that are recording
   */
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings() const;

  /*!
   * @return All tv timers that are recording
   */
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveTVRecordings() const;

  /*!
   * @return All radio timers that are recording
   */
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRadioRecordings() const;

  /*!
   * @return True when recording, false otherwise.
   */
  bool IsRecording() const;

  /*!
   * @brief Check if a recording is running on the given channel.
   * @param channel The channel to check.
   * @return True when recording, false otherwise.
   */
  bool IsRecordingOnChannel(const CPVRChannel& channel) const;

  /*!
   * @brief Obtain the active timer for a given channel.
   * @param channel The channel to check.
   * @return the timer, null otherwise.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetActiveTimerForChannel(
      const std::shared_ptr<const CPVRChannel>& channel) const;

  /*!
   * @return The amount of tv and radio timers that are currently recording
   */
  int AmountActiveRecordings() const;

  /*!
   * @return The amount of tv timers that are currently recording
   */
  int AmountActiveTVRecordings() const;

  /*!
   * @return The amount of radio timers that are currently recording
   */
  int AmountActiveRadioRecordings() const;

  /*!
   * @brief Delete all timers on a channel.
   * @param channel The channel to delete the timers for.
   * @param bDeleteTimerRules True to delete timer rules too, false otherwise.
   * @param bCurrentlyActiveOnly True to delete timers that are currently running only.
   * @return True if timers any were deleted, false otherwise.
   */
  bool DeleteTimersOnChannel(const std::shared_ptr<CPVRChannel>& channel,
                             bool bDeleteTimerRules = true,
                             bool bCurrentlyActiveOnly = false);

  /*!
   * @return Next event time (timer or daily wake up)
   */
  CDateTime GetNextEventTime() const;

  /*!
   * @brief Add a timer to the client. Doesn't add the timer to the container. The backend will do
   * this.
   * @param tag The timer to add.
   * @return True if timer add request was sent correctly, false if not.
   */
  bool AddTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag);

  /*!
   * @brief Delete a timer on the client. Doesn't delete the timer from the container. The backend
   * will do this.
   * @param tag The timer to delete.
   * @param bForce Control what to do in case the timer is currently recording.
   *        True to force to delete the timer, false to return TimerDeleteResult::RECORDING.
   * @param bDeleteRule Also delete the timer rule that scheduled the timer instead of single timer
   * only.
   * @return The result.
   */
  TimerOperationResult DeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag,
                                   bool bForce = false,
                                   bool bDeleteRule = false);

  /*!
   * @brief Update the timer on the client. Doesn't update the timer in the container. The backend
   * will do this.
   * @param tag The timer to update.
   * @return True if timer update request was sent correctly, false if not.
   */
  bool UpdateTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag);

  /*!
   * @brief Get the timer tag that matches the given epg tag.
   * @param epgTag The epg tag.
   * @return The requested timer tag, or nullptr if none was found.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetTimerForEpgTag(
      const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

  /*!
   * @brief Get the timer rule for a given timer tag
   * @param timer The timer to query the timer rule for
   * @return The timer rule, or nullptr if none was found.
   */
  std::shared_ptr<CPVRTimerInfoTag> GetTimerRule(
      const std::shared_ptr<const CPVRTimerInfoTag>& timer) const;

  /*!
     * @brief Update the channel pointers.
     */
  void UpdateChannels();

  /*!
     * @brief CEventStream callback for PVR events.
     * @param event The event.
     */
  void Notify(const PVREvent& event);

  /*!
   * @brief Get a timer tag given it's unique ID
   * @param iTimerId The ID to find
   * @return The tag, or an empty one when not found
   */
  std::shared_ptr<CPVRTimerInfoTag> GetById(unsigned int iTimerId) const;

private:
  void Process() override;

  /*!
   * @brief Load all timers from PVR database.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  void RemoveEntry(const std::shared_ptr<const CPVRTimerInfoTag>& tag);
  bool UpdateEntries(const CPVRTimersContainer& timers, const std::vector<int>& failedClients);
  bool UpdateEntries(int iMaxNotificationDelay);
  std::shared_ptr<CPVRTimerInfoTag> UpdateEntry(
      const std::shared_ptr<const CPVRTimerInfoTag>& timer);

  bool AddLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag, bool bNotify);
  bool DeleteLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag, bool bNotify);
  bool UpdateLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag);
  std::shared_ptr<CPVRTimerInfoTag> PersistAndUpdateLocalTimer(
      const std::shared_ptr<CPVRTimerInfoTag>& timer,
      const std::shared_ptr<CPVRTimerInfoTag>& parentTimer);
  void NotifyTimersEvent(bool bAddedOrDeleted = true);

  enum TimerKind
  {
    TimerKindAny = 0,
    TimerKindTV,
    TimerKindRadio
  };

  bool KindMatchesTag(const TimerKind& eKind,
                      const std::shared_ptr<const CPVRTimerInfoTag>& tag) const;

  std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer(const TimerKind& eKind,
                                                       bool bIgnoreReminders) const;
  int AmountActiveTimers(const TimerKind& eKind) const;
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings(const TimerKind& eKind) const;
  int AmountActiveRecordings(const TimerKind& eKind) const;

  bool CheckAndAppendTimerNotification(std::vector<std::pair<int, std::string>>& timerNotifications,
                                       const std::shared_ptr<const CPVRTimerInfoTag>& tag,
                                       bool bDeleted) const;

  bool m_bIsUpdating = false;
  CPVRSettings m_settings;
  std::queue<std::shared_ptr<CPVRTimerInfoTag>> m_remindersToAnnounce;
  bool m_bReminderRulesUpdatePending = false;

  bool m_bFirstUpdate = true;
  std::vector<int> m_failedClients;
};
} // namespace PVR
