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
#include <utility>
#include <vector>

#include "XBDateTime.h"
#include "settings/lib/ISettingCallback.h"

#include "pvr/PVRTypes.h"
#include "pvr/channels/PVRChannel.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
#define PVR_GROUP_TYPE_DEFAULT      0
#define PVR_GROUP_TYPE_INTERNAL     1
#define PVR_GROUP_TYPE_USER_DEFINED 2

  struct PVRChannelGroupMember
  {
    PVRChannelGroupMember() = default;

    PVRChannelGroupMember(const CPVRChannelPtr _channel, const CPVRChannelNumber &_channelNumber, int _iClientPriority)
    : channel(_channel), channelNumber(_channelNumber), iClientPriority(_iClientPriority) {}

    CPVRChannelPtr channel;
    CPVRChannelNumber channelNumber; // the number this channel has in the group
    int iClientPriority = 0;
  };

  typedef std::vector<PVRChannelGroupMember> PVR_CHANNEL_GROUP_SORTED_MEMBERS;
  typedef std::map<std::pair<int, int>, PVRChannelGroupMember> PVR_CHANNEL_GROUP_MEMBERS;

  enum EpgDateType
  {
    EPG_FIRST_DATE = 0,
    EPG_LAST_DATE = 1
  };

  /** A group of channels */
  class CPVRChannelGroup : public Observable,
                           public ISettingCallback
  {
    friend class CPVRChannelGroupInternal;
    friend class CPVRDatabase;

  public:
    CPVRChannelGroup(void);

    /*!
     * @brief Create a new channel group instance.
     * @param bRadio True if this group holds radio channels.
     * @param iGroupId The database ID of this group.
     * @param strGroupName The name of this group.
     */
    CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const std::string &strGroupName);

    /*!
     * @brief Create a new channel group instance from a channel group provided by an add-on.
     * @param group The channel group provided by the add-on.
     */
    explicit CPVRChannelGroup(const PVR_CHANNEL_GROUP &group);

    /*!
     * @brief Copy constructor
     * @param group Source group
     */
    CPVRChannelGroup(const CPVRChannelGroup &group);

    ~CPVRChannelGroup(void) override;

    bool operator ==(const CPVRChannelGroup &right) const;
    bool operator !=(const CPVRChannelGroup &right) const;

    /**
     * Empty group member
     */
    static PVRChannelGroupMember EmptyMember;

    /*!
     * @brief Load the channels from the database.
     * @return True when loaded successfully, false otherwise.
     */
    virtual bool Load(void);

    /*!
     * @return The amount of group members
     */
    size_t Size(void) const;

    /*!
     * @brief Refresh the channel list from the clients.
     */
    virtual bool Update(void);

    /*!
     * @brief Get the path of this group.
     * @return the path.
     */
    std::string GetPath() const;

    /*!
     * @brief Change the channelnumber of a group. Used by CGUIDialogPVRChannelManager. Call SortByChannelNumber() and Renumber() after all changes are done.
     * @param channel The channel to change the channel number for.
     * @param channelNumber The new channel number.
     */
    bool SetChannelNumber(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber);

    /*!
     * @brief Search missing channel icons for all known channels.
     * @param bUpdateDb If true, update the changed values in the database.
     */
    void SearchAndSetChannelIcons(bool bUpdateDb = false);

    /*!
     * @brief Remove a channel from this container.
     * @param channel The channel to remove.
     * @return True if the channel was found and removed, false otherwise.
     */
    virtual bool RemoveFromGroup(const CPVRChannelPtr &channel);

    /*!
     * @brief Add a channel to this container.
     * @param channel The channel to add.
     * @param channelNumber The channel number of the channel to add. Use empty channel number to add it at the end.
     * @param bUseBackendChannelNumbers True, if channelNumber contains a backend channel number.
     * @return True if the channel was added, false otherwise.
     */
    virtual bool AddToGroup(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber, bool bUseBackendChannelNumbers);

    /*!
     * @brief Change the name of this group.
     * @param strGroupName The new group name.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetGroupName(const std::string &strGroupName, bool bSaveInDb = false);

    /*!
     * @brief Persist changed or new data.
     * @return True if the channel was persisted, false otherwise.
     */
    bool Persist(void);

    /*!
     * @brief Check whether a channel is in this container.
     * @param channel The channel to find.
     * @return True if the channel was found, false otherwise.
     */
    virtual bool IsGroupMember(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Check whether a channel is in this container.
     * @param iChannelId The db id of the channel to find.
     * @return True if the channel was found, false otherwise.
     */
    virtual bool IsGroupMember(int iChannelId) const;

    /*!
     * @brief Check if this group is the internal group containing all channels.
     * @return True if it's the internal group, false otherwise.
     */
    virtual bool IsInternalGroup(void) const { return m_iGroupType == PVR_GROUP_TYPE_INTERNAL; }

    /*!
     * @brief True if this group holds radio channels, false if it holds TV channels.
     * @return True if this group holds radio channels, false if it holds TV channels.
     */
    bool IsRadio(void) const { return m_bRadio; }

    /*!
     * @brief Set 'radio' property of this group.
     * @param bIsRadio The new value for the 'radio' property.
     */
    void SetRadio(bool bIsRadio) { m_bRadio = bIsRadio; }

    /*!
     * @brief True if sorting should be prevented when adding/updating channels to the group.
     * @return True if sorting should be prevented when adding/updating channels to the group.
     */
    bool PreventSortAndRenumber(void) const;

    /*!
     * @brief The database ID of this group.
     * @return The database ID of this group.
     */
    int GroupID(void) const;

    /*!
     * @brief Set the database ID of this group.
     * @param iGroupId The new database ID.
     */
    void SetGroupID(int iGroupId);

    /*!
     * @brief Set the type of this group.
     * @param the new type for this group.
     */
    void SetGroupType(int iGroupType);

    /*!
     * @brief Return the type of this group.
     */
    int GroupType(void) const;

    /*!
     * @return Time group has been watched last.
     */
    time_t LastWatched() const;

    /*!
     * @brief Last time group has been watched
     * @param iLastWatched The new value.
     * @return True if something changed, false otherwise.
     */
    bool SetLastWatched(time_t iLastWatched);

    /*!
     * @brief Set if sorting and renumbering should happen after adding/updating channels to group.
     * @param bPreventSortAndRenumber The new sorting and renumbering prevention value for this group.
     */
    void SetPreventSortAndRenumber(bool bPreventSortAndRenumber = true);

    /*!
     * @brief The name of this group.
     * @return The name of this group.
     */
    std::string GroupName(void) const;

    /*! @name Sort methods
     */
    //@{

    /*!
     * @brief Sort the group and fix up channel numbers.
     * @return True when numbering changed, false otherwise
     */
    bool SortAndRenumber(void);

    /*!
     * @brief Remove invalid channels and updates the channel numbers.
     * @return True if something changed, false otherwise.
     */
    bool Renumber(void);

    //@}

    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

    /*!
     * @brief Get a channel given it's EPG ID.
     * @param iEpgID The channel EPG ID.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByChannelEpgID(int iEpgID) const;

    /*!
     * @brief The channel that was played last that has a valid client or NULL if there was none.
     * @param iCurrentChannel The channelid of the current channel that is playing, or -1 if none
     * @return The requested channel.
     */
    CFileItemPtr GetLastPlayedChannel(int iCurrentChannel = -1) const;

    /*!
     * @brief Get a channel given it's channel number.
     * @param channelNumber The channel number.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetByChannelNumber(const CPVRChannelNumber &channelNumber) const;

    /*!
     * @brief Get the channel number in this group of the given channel.
     * @param channel The channel to get the channel number for.
     * @return The channel number in this group.
     */
    CPVRChannelNumber GetChannelNumber(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Get the next channel in this group.
     * @param channel The current channel.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetNextChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Get the previous channel in this group.
     * @param channel The current channel.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetPreviousChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Get a channel given it's channel ID.
     * @param iChannelID The channel ID.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByChannelID(int iChannelID) const;

    /*!
     * Get the current members of this group
     * @return The group members
     */
    PVR_CHANNEL_GROUP_SORTED_MEMBERS GetMembers(void) const;

    enum class Include
    {
      ALL,
      ONLY_HIDDEN,
      ONLY_VISIBLE
    };

    /*!
     * @brief Get a filtered list of channels in this group.
     * @param results The file list to store the results in.
     * @param eFilter A filter to apply to the list.
     * @return The amount of channels that were added to the list.
     */
    int GetMembers(CFileItemList &results, Include eFilter = Include::ONLY_VISIBLE) const;

    /*!
     * @brief Get the list of channel numbers in a group.
     * @param channelNumbers The list to store the numbers in.
     */
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) const;

    /*!
     * @return The next channel group.
     */
    CPVRChannelGroupPtr GetNextGroup(void) const;

    /*!
     * @return The previous channel group.
     */
    CPVRChannelGroupPtr GetPreviousGroup(void) const;

    /*!
     * @brief The amount of hidden channels in this container.
     * @return The amount of hidden channels in this container.
     */
    virtual size_t GetNumHiddenChannels(void) const { return 0; }

    /*!
     * @brief Does this container holds channels.
     * @return True if there is at least one channel in this container, otherwise false.
     */
    bool HasChannels(void) const;

    /*!
     * @return True if there is at least one channel in this group with changes that haven't been persisted, false otherwise.
     */
    bool HasChangedChannels(void) const;

    /*!
     * @return True if there is at least one new channel in this group that hasn't been persisted, false otherwise.
     */
    bool HasNewChannels(void) const;

    /*!
     * @return True if anything changed in this group that hasn't been persisted, false otherwise.
     */
    bool HasChanges(void) const;

    /*!
     * @brief Create an EPG table for each channel.
     * @brief bForce Create the tables, even if they already have been created before.
     * @return True if all tables were created successfully, false otherwise.
     */
    virtual bool CreateChannelEpgs(bool bForce = false);

    /*!
     * @brief Get all EPG tables.
     * @param results The fileitem list to store the results in.
     * @param bIncludeChannelsWithoutEPG, for channels without EPG data, put an empty EPG tag associated with the channel into results
     * @return The amount of entries that were added.
     */
    int GetEPGAll(CFileItemList &results, bool bIncludeChannelsWithoutEPG = false) const;

    /*!
     * @brief Get the start time of the first entry.
     * @return The start time.
     */
    CDateTime GetFirstEPGDate(void) const;

    /*!
     * @brief Get the end time of the last entry.
     * @return The end time.
     */
    CDateTime GetLastEPGDate(void) const;

    bool UpdateChannel(const CFileItem &channel, bool bHidden, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const std::string &strChannelName, const std::string &strIconPath, bool bUserSetIcon = false);

    /*!
     * @brief Get a channel given the channel number on the client.
     * @param iUniqueChannelId The unique channel id on the client.
     * @param iClientID The ID of the client.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByUniqueID(int iUniqueChannelId, int iClientID) const;

    /*!
     * @brief Get a channel group member given its storage id.
     * @param id The storage id (a pair of client id and unique channel id).
     * @return A reference to the group member or an empty group member if it wasn't found.
     */
    PVRChannelGroupMember& GetByUniqueID(const std::pair<int, int>& id);
    const PVRChannelGroupMember& GetByUniqueID(const std::pair<int, int>& id) const;

    void SetHidden(bool bHidden);
    bool IsHidden(void) const;

    int GetPosition(void) const;
    void SetPosition(int iPosition);

    /*!
     * @brief Check, whether channel group member data for a given pvr client are currently missing, for instance, because the client was offline when data was last queried.
     * @param iClientId The id of the client.
     * @return True, if data is currently missing, false otherwise.
     */
    bool IsMissingChannelGroupMembersFromClient(int iClientId) const;

    /*!
     * @brief Check, whether channel data for a given pvr client are currently missing, for instance, because the client was offline when data was last queried.
     * @param iClientId The id of the client.
     * @return True, if data is currently missing, false otherwise.
     */
    bool IsMissingChannelsFromClient(int iClientId) const;

  protected:
    /*!
     * @brief Init class
     */
    virtual void OnInit(void);

    /*!
     * @brief Load the channels stored in the database.
     * @param bCompress If true, compress the database after storing the channels.
     * @return The amount of channels that were added.
     */
    virtual int LoadFromDb(bool bCompress = false);

    /*!
     * @brief Update the current channel list with the given list.
     *
     * Update the current channel list with the given list.
     * Only the new channels will be present in the passed list after this call.
     *
     * @param channels The channels to use to update this list.
     * @return True if everything went well, false otherwise.
     */
    virtual bool UpdateGroupEntries(const CPVRChannelGroup &channels);

    /*!
     * @brief Add new channels to this group; updtae data.
     * @param channels The new channels to use for this group.
     * @param bUseBackendChannelNumbers True, if channel numbers from backends shall be used.
     * @return True if everything went well, false otherwise.
     */
    virtual bool AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers);

    /*!
     * @brief Remove deleted channels from this group.
     * @param channels The new channels to use for this group.
     * @return The removed channels.
     */
    virtual std::vector<CPVRChannelPtr> RemoveDeletedChannels(const CPVRChannelGroup &channels);

    /*!
     * @brief Clear this channel list.
     */
    virtual void Unload(void);

    /*!
     * @brief Load the channels from the clients.
     * @return True when loaded successfully, false otherwise.
     */
    virtual bool LoadFromClients(void);

    /*!
     * @brief Sort the current channel list by client channel number.
     */
    void SortByClientChannelNumber(void);

    /*!
     * @brief Sort the current channel list by channel number.
     */
    void SortByChannelNumber(void);

    /*!
     * @brief Update the priority for all members of all channel groups.
     */
    bool UpdateClientPriorities();

    bool             m_bRadio = false;                      /*!< true if this container holds radio channels, false if it holds TV channels */
    int              m_iGroupType = PVR_GROUP_TYPE_DEFAULT;                  /*!< The type of this group */
    int              m_iGroupId = -1;                    /*!< The ID of this group in the database */
    std::string      m_strGroupName;                /*!< The name of this group */
    bool             m_bLoaded = false;                     /*!< True if this container is loaded, false otherwise */
    bool             m_bChanged = false;                    /*!< true if anything changed in this group that hasn't been persisted, false otherwise */
    bool             m_bUsingBackendChannelOrder = false;   /*!< true to use the channel order from backends, false otherwise */
    bool             m_bUsingBackendChannelNumbers = false; /*!< true to use the channel numbers from 1 backend, false otherwise */
    bool             m_bPreventSortAndRenumber = false;     /*!< true when sorting and renumbering should not be done after adding/updating channels to the group */
    time_t           m_iLastWatched = 0;                /*!< last time group has been watched */
    bool             m_bHidden = false;                     /*!< true if this group is hidden, false otherwise */
    int              m_iPosition = 0;                   /*!< the position of this group within the group list */
    PVR_CHANNEL_GROUP_SORTED_MEMBERS m_sortedMembers; /*!< members sorted by channel number */
    PVR_CHANNEL_GROUP_MEMBERS        m_members;       /*!< members with key clientid+uniqueid */
    mutable CCriticalSection m_critSection;
    std::vector<int> m_failedClientsForChannels;
    std::vector<int> m_failedClientsForChannelGroupMembers;

  private:
    CDateTime GetEPGDate(EpgDateType epgDateType) const;
  };
}
