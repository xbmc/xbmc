/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "pvr/channels/PVRChannelNumber.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/EventStream.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct PVR_CHANNEL_GROUP;

namespace PVR
{
#define PVR_GROUP_TYPE_DEFAULT      0
#define PVR_GROUP_TYPE_INTERNAL     1
#define PVR_GROUP_TYPE_USER_DEFINED 2

  enum class PVREvent;

  class CPVRChannel;
  class CPVREpgInfoTag;

  struct PVRChannelGroupMember
  {
    PVRChannelGroupMember() = default;

    PVRChannelGroupMember(const std::shared_ptr<CPVRChannel> _channel, const CPVRChannelNumber& _channelNumber, int _iClientPriority, int _iOrder, const CPVRChannelNumber& _clientChannelNumber)
      : channel(_channel)
      , channelNumber(_channelNumber)
      , clientChannelNumber(_clientChannelNumber)
      , iClientPriority(_iClientPriority)
      , iOrder(_iOrder) {}

    std::shared_ptr<CPVRChannel> channel;
    CPVRChannelNumber channelNumber; // the channel number this channel has in the group
    CPVRChannelNumber clientChannelNumber; // the client channel number this channel has in the group
    int iClientPriority = 0;
    int iOrder = 0; // The value denoting the order of this member in the group
  };

  enum EpgDateType
  {
    EPG_FIRST_DATE = 0,
    EPG_LAST_DATE = 1
  };

  enum RenumberMode
  {
    NORMAL = 0,
    IGNORE_NUMBERING_FROM_ONE = 1
  };

  class CPVRChannelGroup : public ISettingCallback
  {
    friend class CPVRChannelGroupInternal;
    friend class CPVRDatabase;

  public:
    static const int INVALID_GROUP_ID = -1;

    /*!
     * @brief Create a new channel group instance.
     * @param path The channel group path.
     * @param iGroupId The database ID of this group or INVALID_GROUP_ID if the group was not yet stored in the database.
     * @param allChannelsGroup The channel group containing all TV or radio channels.
     */
    CPVRChannelGroup(const CPVRChannelsPath& path,
                     int iGroupId = INVALID_GROUP_ID,
                     const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup = {});

    /*!
     * @brief Create a new channel group instance from a channel group provided by an add-on.
     * @param group The channel group provided by the add-on.
     * @param allChannelsGroup The channel group containing all TV or radio channels.
     */
    CPVRChannelGroup(const PVR_CHANNEL_GROUP& group, const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup);

    ~CPVRChannelGroup() override;

    bool operator ==(const CPVRChannelGroup& right) const;
    bool operator !=(const CPVRChannelGroup& right) const;

    /**
     * Empty group member
     */
    static std::shared_ptr<PVRChannelGroupMember> EmptyMember;

    /*!
     * @brief Query the events available for CEventStream
     */
    CEventStream<PVREvent>& Events() { return m_events; }

    /*!
     * @brief Load the channels from the database.
     * @param channelsToRemove Returns the channels to be removed from all groups, if any
     * @return True when loaded successfully, false otherwise.
     */
    virtual bool Load(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove);

    /*!
     * @return The amount of group members
     */
    size_t Size() const;

    /*!
     * @brief Refresh the channel list from the clients.
     * @param channelsToRemove Returns the channels to be removed from all groups, if any
     */
    virtual bool Update(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove);

    /*!
     * @brief Get the path of this group.
     * @return the path.
     */
    const CPVRChannelsPath& GetPath() const;

    /*!
     * @brief Set the path of this group.
     * @param the path.
     */
    void SetPath(const CPVRChannelsPath& path);

    /*!
     * @brief Change the channelnumber of a group. Used by CGUIDialogPVRChannelManager. Call SortByChannelNumber() and Renumber() after all changes are done.
     * @param channel The channel to change the channel number for.
     * @param channelNumber The new channel number.
     */
    bool SetChannelNumber(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber);

    /*!
     * @brief Remove a channel from this container.
     * @param channel The channel to remove.
     * @return True if the channel was found and removed, false otherwise.
     */
    virtual bool RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel);

    /*!
     * @brief Add a channel to this container.
     * @param channel The channel to add.
     * @param channelNumber The channel number of the channel to add. Use empty channel number if it's to be generated.
     * @param iOrder The value denoting the order of this member in the group, 0 if unknown and needs to be generated
     * @param bUseBackendChannelNumbers True, if channelNumber contains a backend channel number.
     * @param clientChannelNumber The client channel number of the channel to add. (optional)
     * @return True if the channel was added, false otherwise.
     */
    virtual bool AddToGroup(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, bool bUseBackendChannelNumbers, const CPVRChannelNumber& clientChannelNumber = {});

    /*!
     * @brief Append a channel to this container.
     * @param channel The channel to append.
     * @return True if the channel was appended, false otherwise.
     */
    virtual bool AppendToGroup(const std::shared_ptr<CPVRChannel>& channel);

    /*!
     * @brief Change the name of this group.
     * @param strGroupName The new group name.
     */
    void SetGroupName(const std::string& strGroupName);

    /*!
     * @brief Persist changed or new data.
     * @return True if the channel was persisted, false otherwise.
     */
    bool Persist();

    /*!
     * @brief Check whether a channel is in this container.
     * @param channel The channel to find.
     * @return True if the channel was found, false otherwise.
     */
    virtual bool IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const;

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
    virtual bool IsInternalGroup() const { return m_iGroupType == PVR_GROUP_TYPE_INTERNAL; }

    /*!
     * @brief True if this group holds radio channels, false if it holds TV channels.
     * @return True if this group holds radio channels, false if it holds TV channels.
     */
    bool IsRadio() const;

    /*!
     * @brief True if sorting should be prevented when adding/updating channels to the group.
     * @return True if sorting should be prevented when adding/updating channels to the group.
     */
    bool PreventSortAndRenumber() const;

    /*!
     * @brief The database ID of this group.
     * @return The database ID of this group.
     */
    int GroupID() const;

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
    int GroupType() const;

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
    std::string GroupName() const;

    /*! @name Sort methods
     */
    //@{

    /*!
     * @brief Sort the group.
     */
    void Sort();

    /*!
     * @brief Sort the group and fix up channel numbers.
     * @return True when numbering changed, false otherwise
     */
    bool SortAndRenumber();

    /*!
     * @brief Remove invalid channels and updates the channel numbers.
     * @param mode the numbering mode to use
     * @return True if something changed, false otherwise.
     */
    bool Renumber(RenumberMode mode = NORMAL);

    //@}

    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

    /*!
     * @brief Get a channel given it's EPG ID.
     * @param iEpgID The channel EPG ID.
     * @return The channel or NULL if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetByChannelEpgID(int iEpgID) const;

    /*!
     * @brief Get the channel that was played last.
     * @param iCurrentChannel The channelid of the current channel that is playing, or -1 if none
     * @return The requested channel or nullptr.
     */
    std::shared_ptr<CPVRChannel> GetLastPlayedChannel(int iCurrentChannel = -1) const;

    /*!
     * @brief Get a channel given it's active channel number
     * @param channelNumber The channel number.
     * @return The channel or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetByChannelNumber(const CPVRChannelNumber& channelNumber) const;

    /*!
     * @brief Get the channel number in this group of the given channel.
     * @param channel The channel to get the channel number for.
     * @return The channel number in this group.
     */
    CPVRChannelNumber GetChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const;

    /*!
     * @brief Get the client channel number in this group of the given channel.
     * @param channel The channel to get the channel number for.
     * @return The client channel number in this group.
     */
    CPVRChannelNumber GetClientChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const;

    /*!
     * @brief Get the next channel in this group.
     * @param channel The current channel.
     * @return The channel or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetNextChannel(const std::shared_ptr<CPVRChannel>& channel) const;

    /*!
     * @brief Get the previous channel in this group.
     * @param channel The current channel.
     * @return The channel or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetPreviousChannel(const std::shared_ptr<CPVRChannel>& channel) const;

    /*!
     * @brief Get a channel given it's channel ID.
     * @param iChannelID The channel ID.
     * @return The channel or NULL if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetByChannelID(int iChannelID) const;

    enum class Include
    {
      ALL,
      ONLY_HIDDEN,
      ONLY_VISIBLE
    };

    /*!
     * @brief Get the current members of this group
     * @param eFilter A filter to apply.
     * @return The group members
     */
    std::vector<std::shared_ptr<PVRChannelGroupMember>> GetMembers(Include eFilter = Include::ALL) const;

    /*!
     * @brief Get the list of active channel numbers in a group.
     * @param channelNumbers The list to store the numbers in.
     */
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) const;

    /*!
     * @brief The amount of hidden channels in this container.
     * @return The amount of hidden channels in this container.
     */
    virtual size_t GetNumHiddenChannels() const { return 0; }

    /*!
     * @brief Does this container holds channels.
     * @return True if there is at least one channel in this container, otherwise false.
     */
    bool HasChannels() const;

    /*!
     * @return True if there is at least one channel in this group with changes that haven't been persisted, false otherwise.
     */
    bool HasChangedChannels() const;

    /*!
     * @return True if there is at least one new channel in this group that hasn't been persisted, false otherwise.
     */
    bool HasNewChannels() const;

    /*!
     * @return True if anything changed in this group that hasn't been persisted, false otherwise.
     */
    bool HasChanges() const;

    /*!
     * @brief Create an EPG table for each channel.
     * @brief bForce Create the tables, even if they already have been created before.
     * @return True if all tables were created successfully, false otherwise.
     */
    virtual bool CreateChannelEpgs(bool bForce = false);

    /*!
     * @brief Get the start time of the first entry.
     * @return The start time.
     */
    CDateTime GetFirstEPGDate() const;

    /*!
     * @brief Get the end time of the last entry.
     * @return The end time.
     */
    CDateTime GetLastEPGDate() const;

    /*!
     * @brief Update a channel group member with given data.
     * @param storageId The storage id of the channel.
     * @param strChannelName The channel name to set.
     * @param strIconPath The icon path to set.
     * @param iEPGSource The EPG id.
     * @param iChannelNumber The channel number to set.
     * @param bHidden Set/Remove hidden flag for the channel group member identified by storage id.
     * @param bEPGEnabled Set/Remove EPG enabled flag for the channel group member identified by storage id.
     * @param bParentalLocked Set/Remove parental locked flag for the channel group member identified by storage id.
     * @param bUserSetIcon Set/Remove user set icon flag for the channel group member identified by storage id.
     * @return True on success, false otherwise.
     */
    bool UpdateChannel(const std::pair<int, int>& storageId,
                       const std::string& strChannelName,
                       const std::string& strIconPath,
                       int iEPGSource,
                       int iChannelNumber,
                       bool bHidden,
                       bool bEPGEnabled,
                       bool bParentalLocked,
                       bool bUserSetIcon);

    /*!
     * @brief Get a channel given the channel number on the client.
     * @param iUniqueChannelId The unique channel id on the client.
     * @param iClientID The ID of the client.
     * @return The channel or NULL if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetByUniqueID(int iUniqueChannelId, int iClientID) const;

    /*!
     * @brief Get a channel group member given its storage id.
     * @param id The storage id (a pair of client id and unique channel id).
     * @return A reference to the group member or an empty group member if it wasn't found.
     */
    std::shared_ptr<PVRChannelGroupMember>& GetByUniqueID(const std::pair<int, int>& id);
    const std::shared_ptr<PVRChannelGroupMember>& GetByUniqueID(const std::pair<int, int>& id) const;

    void SetHidden(bool bHidden);
    bool IsHidden() const;

    int GetPosition() const;
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

    /*!
     * @brief For each channel and its corresponding epg channel data update the order from the group members
     */
    void UpdateClientOrder();

    /*!
     * @brief For each channel and its corresponding epg channel data update the channel number from the group members
     */
    void UpdateChannelNumbers();

    /*!
     * @brief Update whether or not this group is currently selected
     * @param isSelectedGroup whether or not this group is the currently selected group.
     */
    void SetSelectedGroup(bool isSelectedGroup) { m_bIsSelectedGroup = isSelectedGroup; }

    /*!
     * @brief Update the channel numbers according to the all channels group and publish event.
     * @return True, if a channel number was changed, false otherwise.
     */
    bool UpdateChannelNumbersFromAllChannelsGroup();

  protected:
    /*!
     * @brief Init class
     */
    virtual void OnInit();

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
     * @param channelsToRemove Returns the channels to be removed from all groups, if any
     * @return True if everything went well, false otherwise.
     */
    virtual bool UpdateGroupEntries(const CPVRChannelGroup& channels, std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove);

    /*!
     * @brief Add new channels to this group; updtae data.
     * @param channels The new channels to use for this group.
     * @param bUseBackendChannelNumbers True, if channel numbers from backends shall be used.
     * @return True if everything went well, false otherwise.
     */
    virtual bool AddAndUpdateChannels(const CPVRChannelGroup& channels, bool bUseBackendChannelNumbers);

    /*!
     * @brief Remove deleted channels from this group.
     * @param channels The new channels to use for this group.
     * @return The removed channels.
     */
    virtual std::vector<std::shared_ptr<CPVRChannel>> RemoveDeletedChannels(const CPVRChannelGroup& channels);

    /*!
     * @brief Clear this channel list.
     */
    virtual void Unload();

    /*!
     * @brief Load the channels from the clients.
     * @return True when loaded successfully, false otherwise.
     */
    virtual bool LoadFromClients();

    /*!
     * @brief Sort the current channel list by client channel number.
     */
    void SortByClientChannelNumber();

    /*!
     * @brief Sort the current channel list by channel number.
     */
    void SortByChannelNumber();

    /*!
     * @brief Update the priority for all members of all channel groups.
     */
    bool UpdateClientPriorities();

    int m_iGroupType = PVR_GROUP_TYPE_DEFAULT; /*!< The type of this group */
    int m_iGroupId = INVALID_GROUP_ID; /*!< The ID of this group in the database */
    bool m_bLoaded = false; /*!< True if this container is loaded, false otherwise */
    bool m_bChanged = false; /*!< true if anything changed in this group that hasn't been persisted, false otherwise */
    bool m_bUsingBackendChannelOrder = false; /*!< true to use the channel order from backends, false otherwise */
    bool m_bUsingBackendChannelNumbers = false; /*!< true to use the channel numbers from 1 backend, false otherwise */
    bool m_bPreventSortAndRenumber = false; /*!< true when sorting and renumbering should not be done after adding/updating channels to the group */
    time_t m_iLastWatched = 0; /*!< last time group has been watched */
    bool m_bHidden = false; /*!< true if this group is hidden, false otherwise */
    int m_iPosition = 0; /*!< the position of this group within the group list */
    std::vector<std::shared_ptr<PVRChannelGroupMember>> m_sortedMembers; /*!< members sorted by channel number */
    std::map<std::pair<int, int>, std::shared_ptr<PVRChannelGroupMember>> m_members; /*!< members with key clientid+uniqueid */
    mutable CCriticalSection m_critSection;
    std::vector<int> m_failedClientsForChannels;
    std::vector<int> m_failedClientsForChannelGroupMembers;
    CEventSource<PVREvent> m_events;
    bool m_bIsSelectedGroup = false; /*!< Whether or not this group is currently selected */
    bool m_bStartGroupChannelNumbersFromOne = false; /*!< true if we start group channel numbers from one when not using backend channel numbers, false otherwise */
    bool m_bSyncChannelGroups = false; /*!< true if channel groups should be synced with the backend, false otherwise */

  private:
    CDateTime GetEPGDate(EpgDateType epgDateType) const;

    std::shared_ptr<CPVRChannelGroup> m_allChannelsGroup;
    CPVRChannelsPath m_path;
  };
}
