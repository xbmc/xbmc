/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"

#include <memory>
#include <vector>

namespace PVR
{
  enum class PVREvent;

  class CPVRChannel;
  class CPVRChannelNumber;

  class CPVRChannelGroupInternal : public CPVRChannelGroup
  {
  public:
    /*!
     * @brief Create a new internal channel group.
     * @param bRadio True if this group holds radio channels.
     */
    explicit CPVRChannelGroupInternal(bool bRadio);

    ~CPVRChannelGroupInternal() override;

    /**
     * @brief The amount of channels in this container.
     * @return The amount of channels in this container.
     */
    size_t GetNumHiddenChannels() const override { return m_iHiddenChannels; }

    /*!
     * @brief Callback for add-ons to update a channel.
     * @param channel The updated channel.
     * @param channelNumber A new channel number for the channel.
     * @param iOrder The value denoting the order of this member in the group, 0 if unknown and needs to be generated
     * @param clientChannelNumber The client channel number of the channel to add. (optional)
     * @return The new/updated channel.
     */
    std::shared_ptr<CPVRChannel> UpdateFromClient(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, const CPVRChannelNumber& clientChannelNumber = {});

    /*!
     * @see CPVRChannelGroup::IsGroupMember
     */
    bool IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const override;

    /*!
     * @see CPVRChannelGroup::AddToGroup
     */
    bool AddToGroup(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, bool bUseBackendChannelNumbers, const CPVRChannelNumber& clientChannelNumber = {}) override;

    /*!
     * @see CPVRChannelGroup::AppendToGroup
     */
    bool AppendToGroup(const std::shared_ptr<CPVRChannel>& channel) override;

    /*!
     * @see CPVRChannelGroup::RemoveFromGroup
     */
    bool RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel) override;

    /*!
     * @brief Check whether the group name is still correct after the language setting changed.
     */
    void CheckGroupName();

    /*!
     * @brief Create an EPG table for each channel.
     * @brief bForce Create the tables, even if they already have been created before.
     * @return True if all tables were created successfully, false otherwise.
     */
    bool CreateChannelEpgs(bool bForce = false) override;

  protected:
    /*!
     * @brief Load all channels from the database.
     * @param bCompress Compress the database after changing anything.
     * @return The amount of channels that were loaded.
     */
    int LoadFromDb(bool bCompress = false) override;

    /*!
     * @brief Load all channels from the clients.
     * @return True when updated successfully, false otherwise.
     */
    bool LoadFromClients() override;

    /*!
     * @brief Check if this group is the internal group containing all channels.
     * @return True if it's the internal group, false otherwise.
     */
    bool IsInternalGroup() const override { return true; }

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
    bool UpdateGroupEntries(const CPVRChannelGroup& channels, std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove) override;

    /*!
     * @brief Add new channels to this group; updtae data.
     * @param channels The new channels to use for this group.
     * @param bUseBackendChannelNumbers True, if channel numbers from backends shall be used.
     * @return True if everything went well, false otherwise.
     */
    bool AddAndUpdateChannels(const CPVRChannelGroup& channels, bool bUseBackendChannelNumbers) override;

    /*!
     * @brief Remove deleted channels from this group.
     * @param channels The new channels to use for this group.
     * @return The removed channels.
     */
    std::vector<std::shared_ptr<CPVRChannel>> RemoveDeletedChannels(const CPVRChannelGroup& channels) override;

    /*!
     * @brief Refresh the channel list from the clients.
     * @param channelsToRemove Returns the channels to be removed from all groups, if any
     */
    bool Update(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove) override;

    /*!
     * @brief Load the channels from the database.
     *
     * Load the channels from the database.
     * If no channels are stored in the database, then the channels will be loaded from the clients.
     *
     * @param channelsToRemove Returns the channels to be removed from all groups, if any
     * @return True when loaded successfully, false otherwise.
     */
    bool Load(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove) override;

    /*!
     * @brief Update the vfs paths of all channels.
     */
    void UpdateChannelPaths();

    void CreateChannelEpg(const std::shared_ptr<CPVRChannel>& channel);

    size_t m_iHiddenChannels; /*!< the amount of hidden channels in this container */

  private:
    void OnPVRManagerEvent(const PVREvent& event);
  };
}
