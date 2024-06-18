/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroupSettings.h"
#include "pvr/channels/PVRChannelNumber.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "utils/EventStream.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct PVR_CHANNEL_GROUP;

namespace PVR
{
static constexpr int PVR_GROUP_TYPE_CLIENT = 0;
static constexpr int PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS = 1;
static constexpr int PVR_GROUP_TYPE_USER = 2;

static constexpr int PVR_GROUP_CLIENT_ID_UNKNOWN = -2;
static constexpr int PVR_GROUP_CLIENT_ID_LOCAL = -1;

static constexpr int PVR_GROUP_ID_UNNKOWN{-1};

enum class PVREvent;

class CPVRChannel;
class CPVRChannelGroupMember;
class CPVRClient;
class CPVREpgInfoTag;

enum RenumberMode
{
  NORMAL = 0,
  IGNORE_NUMBERING_FROM_ONE = 1
};

using GroupMemberPair =
    std::pair<std::shared_ptr<CPVRChannelGroupMember>, std::shared_ptr<CPVRChannelGroupMember>>;

class CPVRChannelGroup : public IChannelGroupSettingsCallback
{
  friend class CPVRChannelGroups; // for GroupType()
  friend class CPVRDatabase; // for GroupType()

public:
  static const int INVALID_GROUP_ID = -1;

  /*!
   * @brief Create a new channel group instance.
   * @param path The channel group path.
   * @param allChannelsGroup The channel group containing all TV or radio channels.
   */
  CPVRChannelGroup(const CPVRChannelsPath& path,
                   const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup);

  ~CPVRChannelGroup() override;

  bool operator==(const CPVRChannelGroup& right) const;
  bool operator!=(const CPVRChannelGroup& right) const;

  /*!
   * @brief Copy over data to the given PVR_CHANNEL_GROUP instance.
   * @param group The group instance to fill.
   */
  void FillAddonData(PVR_CHANNEL_GROUP& group) const;

  /*!
   * @brief Query the events available for CEventStream
   */
  CEventStream<PVREvent>& Events() { return m_events; }

  /*!
   * @brief Load the channels from the database.
   * @param channels All available channels.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True when loaded successfully, false otherwise.
   */
  bool LoadFromDatabase(const std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& channels,
                        const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief Clear all data.
   */
  void Unload();

  /*!
   * @return The amount of group members
   */
  size_t Size() const;

  /*!
   * @brief Update data with channel group members from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @return True on success, false otherwise.
   */
  virtual bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) = 0;

  /*!
   * @brief Get the identifier of the client that serves this channel group.
   * @return The identifier or PVR_GROUP_CLIENT_ID_LOCAL for local groups.
   */
  int GetClientID() const;

  /*!
   * @brief Set the identifier of the client that serves this channel group.
   * @param clientID The identifier; must be PVR_GROUP_CLIENT_ID_LOCAL for local groups.
   */
  void SetClientID(int clientID);

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
   * @brief Remove a channel group member from this container.
   * @param groupMember The channel group member to remove.
   * @return True if the channel group member was removed, false otherwise.
   */
  virtual bool RemoveFromGroup(const std::shared_ptr<const CPVRChannelGroupMember>& groupMember);

  /*!
   * @brief Append a channel group member to this container.
   * @param groupMember The channel group member to append.
   * @return True if the channel group member was appended, false otherwise.
   */
  virtual bool AppendToGroup(const std::shared_ptr<const CPVRChannelGroupMember>& groupMember);

  /*!
   * @brief Check whether a channel group member is in this container.
   * @param groupMember The channel group member to check.
   * @return True if the channel group member was found, false otherwise.
   */
  virtual bool IsGroupMember(
      const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const;

  /*!
   * @brief Change the name of this group.
   * @param strGroupName The new group name.
   * @param isUserSetName Whether the name was set by the user.
   */
  void SetGroupName(const std::string& strGroupName, bool isUserSetName = false);

  /*!
   * @brief Set the name this group has on the client.
   * @param groupName The client group name.
   */
  void SetClientGroupName(const std::string& groupName);

  /*!
   * @brief Check whether the group name was set by the user.
   * @return True if set by user, false otherwise;
   */
  bool IsUserSetName() const { return m_isUserSetName; }

  /*!
   * @brief Persist changed or new data.
   * @return True if the channel was persisted, false otherwise.
   */
  bool Persist();

  /*!
   * @brief True if this group holds radio channels, false if it holds TV channels.
   * @return True if this group holds radio channels, false if it holds TV channels.
   */
  bool IsRadio() const;

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
   * @return Time group has been watched last.
   */
  time_t LastWatched() const;

  /*!
   * @brief Last time group has been watched
   * @param iLastWatched The new value.
   */
  void SetLastWatched(time_t iLastWatched);

  /*!
   * @return Time in milliseconds from epoch this group was last opened.
   */
  uint64_t LastOpened() const;

  /*!
   * @brief Set the time in milliseconds from epoch this group was last opened.
   * @param iLastOpened The new value.
   */
  void SetLastOpened(uint64_t iLastOpened);

  /*!
   * @brief The name of this group.
   * @return The name of this group.
   */
  std::string GroupName() const;

  /*!
   * @brief Get the name of this group on the client.
   * @return The group's name.
   */
  std::string ClientGroupName() const;

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

  // IChannelGroupSettingsCallback implementation
  void UseBackendChannelOrderChanged() override;
  void UseBackendChannelNumbersChanged() override;
  void StartGroupChannelNumbersFromOneChanged() override;

  /*!
   * @brief Get the channel group member that was played last.
   * @param iCurrentChannel The channelid of the current channel that is playing, or -1 if none
   * @return The requested channel group member or nullptr.
     */
  std::shared_ptr<CPVRChannelGroupMember> GetLastPlayedChannelGroupMember(
      int iCurrentChannel = -1) const;

  /*!
   * @brief Get the last and previous to last played channel group members.
   * @return The members. pair.first contains the last, pair.second the previous to last member.
   */
  GroupMemberPair GetLastAndPreviousToLastPlayedChannelGroupMember() const;

  /*!
   * @brief Get a channel group member given it's active channel number
   * @param channelNumber The channel number.
   * @return The channel group member or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetByChannelNumber(
      const CPVRChannelNumber& channelNumber) const;

  /*!
   * @brief Get the channel number in this group of the given channel.
   * @param channel The channel to get the channel number for.
   * @return The channel number in this group.
   */
  CPVRChannelNumber GetChannelNumber(const std::shared_ptr<const CPVRChannel>& channel) const;

  /*!
   * @brief Get the client channel number in this group of the given channel.
   * @param channel The channel to get the channel number for.
   * @return The client channel number in this group.
   */
  CPVRChannelNumber GetClientChannelNumber(const std::shared_ptr<const CPVRChannel>& channel) const;

  /*!
   * @brief Get the next channel group member in this group.
   * @param groupMember The current channel group member.
   * @return The channel group member or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetNextChannelGroupMember(
      const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const;

  /*!
   * @brief Get the previous channel group member in this group.
   * @param groupMember The current channel group member.
   * @return The channel group member or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetPreviousChannelGroupMember(
      const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const;

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
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> GetMembers(
      Include eFilter = Include::ALL) const;

  /*!
   * @brief Get the list of active channel numbers in a group.
   * @param channelNumbers The list to store the numbers in.
   */
  void GetChannelNumbers(std::vector<std::string>& channelNumbers) const;

  /*!
   * @brief Check whether this container has any channels.
   * @return True if there is at least one channel in this container, otherwise false.
   */
  bool HasChannels() const;

  /*!
   * @brief Check whether this container has any new channels.
   * @return True if there is at least one new channel in this group that hasn't been persisted, false otherwise.
   */
  bool HasNewChannels() const;

  /*!
  * @brief Check whether this container has any hidden channels.
  * @return True if at least one hidden channel is present, false otherwise.
  */
  bool HasHiddenChannels() const;

  /*!
   * @return True if anything changed in this group that hasn't been persisted, false otherwise.
   */
  bool HasChanges() const;

  /*!
   * @return True if the group was never persisted, false otherwise.
   */
  bool IsNew() const;

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
   * @return The channel group member or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetByUniqueID(const std::pair<int, int>& id) const;

  bool SetHidden(bool bHidden);
  bool IsHidden() const;

  /*!
   * @brief Get the local position of this group.
   * @return The local group position.
   */
  int GetPosition() const;

  /*!
   * @brief Set the local position of this group.
   * @param iPosition The new local group position.
   */
  void SetPosition(int iPosition);

  /*!
   * @brief Get the position of this group as supplied by the PVR client.
   * @return The client-supplied group position.
   */
  int GetClientPosition() const;

  /*!
   * @brief Set the client-supplied position of this group.
   * @param iPosition The new client-supplied group position.
   */
  void SetClientPosition(int iPosition);

  /*!
   * @brief Check, whether data for a given pvr client are currently valid. For instance, data
   * can be invalid because the client's backend was offline when data was last queried.
   * @param iClientId The id of the client.
   * @return True, if data is currently valid, false otherwise.
   */
  bool HasValidDataForClient(int iClientId) const;

  /*!
   * @brief Check, whether data for given pvr clients are currently valid. For instance, data
   * can be invalid because the client's backend was offline when data was last queried.
   * @param clients The clients to check. Check all active clients if vector is empty.
   * @return True, if data is currently valid, false otherwise.
   */
  bool HasValidDataForClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

  /*!
   * @brief Update the channel numbers according to the all channels group and publish event.
   * @return True, if a channel number was changed, false otherwise.
   */
  bool UpdateChannelNumbersFromAllChannelsGroup();

  /*!
   * @brief Remove this group from database.
   */
  void Delete();

  /*!
   * @brief Whether this group is deleted.
   * @return True, if deleted, false otherwise.
   */
  bool IsDeleted() const { return m_bDeleted; }

  /*!
   * @brief Erase stale texture db entries and image files.
   * @return number of cleaned up images.
   */
  int CleanupCachedImages();

  /*!
   * @brief Get the priority of the client that provides this group.
   * @return the priority, as set by the user or 0. Always 0 for non-backend-supplied groups.
   */
  int GetClientPriority() const;

  /*!
   * @brief Update the client priority for this group and all members of this group.
   */
  void UpdateClientPriorities();

  enum class Origin
  {
    USER, // created and managed by the user
    SYSTEM, // created and managed by Kodi
    CLIENT, // created and managed by PVR client add-on
  };
  /*!
   * @brief Get the group's origin.
   * @return The origin.
   */
  virtual Origin GetOrigin() const = 0;

  /*!
   * @brief Check whether this group could be deleted by the user.
   * @return True if the group could be deleted, false otherwise.
   */
  virtual bool SupportsDelete() const = 0;

  /*!
   * @brief Check whether members could be added to this group by the user.
   * @return True if members could be added, false otherwise.
   */
  virtual bool SupportsMemberAdd() const = 0;

  /*!
   * @brief Check whether members could be removed from this group by the user.
   * @return True if members could be removed, false otherwise.
   */
  virtual bool SupportsMemberRemove() const = 0;

  /*!
   * @brief Check whether this group is owner of the channel instances it contains.
   * @return True if owner, false otherwise.
   */
  virtual bool IsChannelsOwner() const = 0;

protected:
  /*!
   * @brief Remove deleted group members from this group.
   * @param groupMembers The group members to use to update this list.
   * @return The removed members .
   */
  virtual std::vector<std::shared_ptr<CPVRChannelGroupMember>> RemoveDeletedGroupMembers(
      const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers);

  /*!
   * @brief Update the current channel group members with the given list.
   * @param groupMembers The group members to use to update this list.
   * @return True if everything went well, false otherwise.
   */
  bool UpdateGroupEntries(const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers);

  /*!
   * @brief Sort the current channel list by client channel number.
   */
  void SortByClientChannelNumber();

  /*!
   * @brief Sort the current channel list by channel number.
   */
  void SortByChannelNumber();

  /*!
   * @brief Update the client priority for all members of this group.
   */
  bool UpdateMembersClientPriority();

  std::shared_ptr<CPVRChannelGroupSettings> GetSettings() const;

  int m_iGroupId = INVALID_GROUP_ID; /*!< The ID of this group in the database */
  bool m_bLoaded = false; /*!< True if this container is loaded, false otherwise */
  bool m_bChanged =
      false; /*!< true if anything changed in this group that hasn't been persisted, false otherwise */
  time_t m_iLastWatched = 0; /*!< last time group has been watched */
  uint64_t m_iLastOpened = 0; /*!< time in milliseconds from epoch this group was last opened */
  bool m_bHidden = false; /*!< true if this group is hidden, false otherwise */
  int m_iPosition = 0; /*!< the local position of this group within the group list */
  std::vector<std::shared_ptr<CPVRChannelGroupMember>>
      m_sortedMembers; /*!< members sorted by channel number */
  std::map<std::pair<int, int>, std::shared_ptr<CPVRChannelGroupMember>>
      m_members; /*!< members with key clientid+uniqueid */
  mutable CCriticalSection m_critSection;
  std::vector<int> m_failedClients;
  CEventSource<PVREvent> m_events;
  mutable std::shared_ptr<CPVRChannelGroupSettings> m_settings;

  // the settings singleton shared between all group instances
  static CCriticalSection m_settingsSingletonCritSection;
  static std::weak_ptr<CPVRChannelGroupSettings> m_settingsSingleton;

private:
  /*!
   * @brief Return the type of this group.
   */
  virtual int GroupType() const = 0;

  /*!
   * @brief Load the channel group members stored in the database.
   * @param clients The PVR clients to load data for. Leave empty for all clients.
   * @return The amount of channel group members that were added.
   */
  int LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief Delete channel group members from database.
   * @param membersToDelete The channel group members.
   */
  void DeleteGroupMembersFromDb(
      const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& membersToDelete);

  /*!
   * @brief Update this group's data with a channel group member provided by a client.
   * @param groupMember The updated group member.
   * @return True if group member data were changed, false otherwise.
   */
  bool UpdateFromClient(const std::shared_ptr<CPVRChannelGroupMember>& groupMember);

  /*!
   * @brief Add new channel group members to this group; update data.
   * @param groupMembers The group members to use to update this list.
   * @return True if group member data were changed, false otherwise.
   */
  bool AddAndUpdateGroupMembers(
      const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers);

  void OnSettingChanged();

  std::shared_ptr<const CPVRChannelGroup> m_allChannelsGroup;
  CPVRChannelsPath m_path;
  bool m_bDeleted = false;
  mutable std::optional<int> m_clientPriority;
  bool m_isUserSetName{false};
  std::string m_clientGroupName;
  int m_iClientPosition{0};
};
} // namespace PVR
