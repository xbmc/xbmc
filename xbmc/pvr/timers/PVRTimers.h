/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "XBDateTime.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "utils/Observer.h"

#include "pvr/PVRSettings.h"
#include "pvr/PVRTypes.h"
#include "pvr/timers/PVRTimerInfoTag.h"

class CFileItem;
class CFileItemList;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CPVRRecording;
  class CPVRTimersPath;

  class CPVRTimersContainer
  {
  public:
    /*!
     * @brief Add a timer tag to this container or update the tag if already present in this container.
     * @param The timer tag
     * @return True, if the update was successful. False, otherwise.
     */
    bool UpdateFromClient(const CPVRTimerInfoTagPtr &timer);

    /*!
     * @brief Get the timer tag denoted by given client id and timer id.
     * @param iClientId The client id.
     * @param iClientTimerId The timer id.
     * @return the timer tag if found, null otherwise.
     */
    CPVRTimerInfoTagPtr GetByClient(int iClientId, unsigned int iClientTimerId) const;

    typedef std::vector<CPVRTimerInfoTagPtr> VecTimerInfoTag;
    typedef std::map<CDateTime, VecTimerInfoTag> MapTags;

    /*!
     * @brief Get the timertags map.
     * @return The map.
     */
    const MapTags& GetTags() const { return m_tags; }

  protected:
    void InsertTimer(const CPVRTimerInfoTagPtr &newTimer);

    mutable CCriticalSection m_critSection;
    unsigned int m_iLastId = 0;
    MapTags m_tags;
  };

  class CPVRTimers : public CPVRTimersContainer, public Observer
  {
  public:
    CPVRTimers(void);
    ~CPVRTimers(void) override;

    /**
     * @brief (re)load the timers from the clients.
     * @return True if loaded successfully, false otherwise.
     */
    bool Load(void);

    /**
     * @brief unload all timers.
     */
    void Unload();

    /**
     * @brief refresh the channel list from the clients.
     */
    bool Update(void);

    /*!
     * @return The tv or radio timer that will be active next (state scheduled), or an empty fileitemptr if none.
     */
    CFileItemPtr GetNextActiveTimer(void) const;

    /*!
     * @return The tv timer that will be active next (state scheduled), or an empty fileitemptr if none.
     */
    CFileItemPtr GetNextActiveTVTimer(void) const;

    /*!
     * @return The radio timer that will be active next (state scheduled), or an empty fileitemptr if none.
     */
    CFileItemPtr GetNextActiveRadioTimer(void) const;

    /*!
     * @return All timers that are active (states scheduled or recording)
     */
    std::vector<CFileItemPtr> GetActiveTimers(void) const;

    /*!
     * Get all timers
     * @param items The list to add the timers to
     */
    void GetAll(CFileItemList& items) const;

    /*!
     * @return True when there is at least one timer that is active (states scheduled or recording), false otherwise.
     */
    bool HasActiveTimers(void) const;

    /*!
     * @return The amount of tv and radio timers that are active (states scheduled or recording)
     */
    int AmountActiveTimers(void) const;

    /*!
     * @return The amount of tv timers that are active (states scheduled or recording)
     */
    int AmountActiveTVTimers(void) const;

    /*!
     * @return The amount of radio timers that are active (states scheduled or recording)
     */
    int AmountActiveRadioTimers(void) const;

    /*!
     * @return All tv and radio timers that are recording
     */
    std::vector<CFileItemPtr> GetActiveRecordings(void) const;

    /*!
     * @return All tv timers that are recording
     */
    std::vector<CFileItemPtr> GetActiveTVRecordings(void) const;

    /*!
     * @return All radio timers that are recording
     */
    std::vector<CFileItemPtr> GetActiveRadioRecordings(void) const;

    /*!
     * @return True when recording, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Check if a recording is running on the given channel.
     * @param channel The channel to check.
     * @return True when recording, false otherwise.
     */
    bool IsRecordingOnChannel(const CPVRChannel &channel) const;

    /*!
     * @brief Obtain the active timer for a given channel.
     * @param channel The channel to check.
     * @return the timer, null otherwise.
     */
    CPVRTimerInfoTagPtr GetActiveTimerForChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @return The amount of tv and radio timers that are currently recording
     */
    int AmountActiveRecordings(void) const;

    /*!
     * @return The amount of tv timers that are currently recording
     */
    int AmountActiveTVRecordings(void) const;

    /*!
     * @return The amount of radio timers that are currently recording
     */
    int AmountActiveRadioRecordings(void) const;

    /*!
     * @brief Get all timers for the given path.
     * @param strPath The vfs path to get the timers for.
     * @param items The results.
     * @return True when the path was valid, false otherwise.
     */
    bool GetDirectory(const std::string& strPath, CFileItemList &items) const;

    /*!
     * @brief Delete all timers on a channel.
     * @param channel The channel to delete the timers for.
     * @param bDeleteTimerRules True to delete timer rules too, false otherwise.
     * @param bCurrentlyActiveOnly True to delete timers that are currently running only.
     * @return True if timers any were deleted, false otherwise.
     */
    bool DeleteTimersOnChannel(const CPVRChannelPtr &channel, bool bDeleteTimerRules = true, bool bCurrentlyActiveOnly = false);

    /*!
     * @return Next event time (timer or daily wake up)
     */
    CDateTime GetNextEventTime(void) const;

    /*!
     * @brief Add a timer to the client. Doesn't add the timer to the container. The backend will do this.
     * @param tag The timer to add.
     * @return True if timer add request was sent correctly, false if not.
     */
     static bool AddTimer(const CPVRTimerInfoTagPtr &tag);

    /*!
     * @brief Delete a timer on the client. Doesn't delete the timer from the container. The backend will do this.
     * @param tag The timer to delete.
     * @param bForce Control what to do in case the timer is currently recording.
     *        True to force to delete the timer, false to return TimerDeleteResult::RECORDING.
     * @param bDeleteRule Also delete the timer rule that scheduled the timer instead of single timer only.
     * @return The result.
     */
    static TimerOperationResult DeleteTimer(const CPVRTimerInfoTagPtr &tag, bool bForce = false, bool bDeleteRule = false);

    /*!
     * @brief Rename a timer on the client. Doesn't update the timer in the container. The backend will do this.
     * @param tag The timer to rename.
     * @return True if timer rename request was sent correctly, false if not.
     */
    static bool RenameTimer(const CPVRTimerInfoTagPtr &tag, const std::string &strNewName);

    /*!
     * @brief Update the timer on the client. Doesn't update the timer in the container. The backend will do this.
     * @param tag The timer to update.
     * @return True if timer update request was sent correctly, false if not.
     */
    static bool UpdateTimer(const CPVRTimerInfoTagPtr &tag);

    /*!
     * @brief Get the timer tag that matches the given epg tag.
     * @param epgTag The epg tag.
     * @return The requested timer tag, or an empty fileitemptr if none was found.
     */
    CPVRTimerInfoTagPtr GetTimerForEpgTag(const CPVREpgInfoTagPtr &epgTag) const;

    /*!
     * @brief Check whether there is a timer currently recording the given recording.
     * @param recording The recording to check.
     * @return true if there is a timer currently recording the given recording, false otherwise.
     */
    bool HasRecordingTimerForRecording(const CPVRRecording &recording) const;

    /*!
     * @brief Get the timer currently recording the given recording, if any.
     * @param recording The recording to check.
     * @return The requested timer tag, or an null if none was found.
     */
    CPVRTimerInfoTagPtr GetRecordingTimerForRecording(const CPVRRecording &recording) const;

    /*!
     * Get the timer rule for a given timer tag
     * @param timer The timer to query the timer rule for
     * @return The timer rule, or null if none was found.
     */
    CPVRTimerInfoTagPtr GetTimerRule(const CPVRTimerInfoTagPtr &timer) const;

    /*!
     * Get the timer rule for a given timer tag
     * @param item The timer to query the timer rule for
     * @return The timer rule, or an empty fileitemptr if none was found.
     */
    CFileItemPtr GetTimerRule(const CFileItemPtr &item) const;

    /*!
     * @brief Update the channel pointers.
     */
    void UpdateChannels(void);

    void Notify(const Observable &obs, const ObservableMessage msg) override;

    /*!
     * Get a timer tag given it's unique ID
     * @param iTimerId The ID to find
     * @return The tag, or an empty one when not found
     */
    CPVRTimerInfoTagPtr GetById(unsigned int iTimerId) const;

  private:
    bool UpdateEntries(const CPVRTimersContainer &timers, const std::vector<int> &failedClients);
    bool GetRootDirectory(const CPVRTimersPath &path, CFileItemList &items) const;
    bool GetSubDirectory(const CPVRTimersPath &path, CFileItemList &items) const;
    bool SetEpgTagTimer(const CPVRTimerInfoTagPtr &timer);
    bool ClearEpgTagTimer(const CPVRTimerInfoTagPtr &timer);

    enum TimerKind
    {
      TimerKindAny = 0,
      TimerKindTV,
      TimerKindRadio
    };

    bool KindMatchesTag(const TimerKind &eKind, const CPVRTimerInfoTagPtr &tag) const;

    CFileItemPtr GetNextActiveTimer(const TimerKind &eKind) const;
    int AmountActiveTimers(const TimerKind &eKind) const;
    std::vector<CFileItemPtr> GetActiveRecordings(const TimerKind &eKind) const;
    int AmountActiveRecordings(const TimerKind &eKind) const;

    bool m_bIsUpdating = false;
    CPVRSettings m_settings;
  };

  class CPVRTimersPath
  {
  public:
    static const std::string PATH_ADDTIMER;
    static const std::string PATH_NEW;

    explicit CPVRTimersPath(const std::string &strPath);
    CPVRTimersPath(const std::string &strPath, int iClientId, unsigned int iParentId);
    CPVRTimersPath(bool bRadio, bool bTimerRules);

    bool IsValid() const { return m_bValid; }

    const std::string &GetPath() const     { return m_path; }
    bool              IsTimersRoot() const { return m_bRoot; }
    bool              IsTimerRule() const  { return !IsTimersRoot(); }
    bool              IsRadio() const      { return m_bRadio; }
    bool              IsRules() const      { return m_bTimerRules; }
    int               GetClientId() const  { return m_iClientId; }
    unsigned int      GetParentId() const  { return m_iParentId; }

  private:
    bool Init(const std::string &strPath);

    std::string  m_path;
    bool         m_bValid;
    bool         m_bRoot;
    bool         m_bRadio;
    bool         m_bTimerRules;
    int          m_iClientId;
    unsigned int m_iParentId;
  };
}
