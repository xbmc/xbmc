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
    CPVRChannelGroupInternal() = delete;

    /*!
     * @brief Create a new internal channel group.
     * @param bRadio True if this group holds radio channels.
     */
    explicit CPVRChannelGroupInternal(bool bRadio);

    /*!
     * @brief Create a new internal channel group.
     * @param path The path for the new group.
     */
    explicit CPVRChannelGroupInternal(const CPVRChannelsPath& path);

    ~CPVRChannelGroupInternal() override;

    /**
     * @brief The amount of channels in this container.
     * @return The amount of channels in this container.
     */
    size_t GetNumHiddenChannels() const override { return m_iHiddenChannels; }

    /*!
     * @see CPVRChannelGroup::IsGroupMember
     */
    bool IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const override;

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
     * @brief Remove deleted group members from this group. Delete stale channels.
     * @param groupMembers The group members to use to update this list.
     * @return The removed members .
     */
    std::vector<std::shared_ptr<CPVRChannelGroupMember>> RemoveDeletedGroupMembers(
        const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers) override;

    /*!
     * @brief Update data with 'all channels' group members from the given clients, sync with local data.
     * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
     * @return True on success, false otherwise.
     */
    bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) override;

    /*!
     * @brief Load the channels from the database.
     * @param channels All available channels.
     * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
     * @return True when loaded successfully, false otherwise.
     */
    bool LoadFromDatabase(
        const std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& channels,
        const std::vector<std::shared_ptr<CPVRClient>>& clients) override;

    /*!
     * @brief Clear all data.
     */
    void Unload() override;

    /*!
     * @brief Update the vfs paths of all channels.
     */
    void UpdateChannelPaths();

    size_t m_iHiddenChannels; /*!< the amount of hidden channels in this container */

  private:
    void OnPVRManagerEvent(const PVREvent& event);
  };
}
