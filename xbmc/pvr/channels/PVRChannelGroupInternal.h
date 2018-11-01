/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"

namespace PVR
{
  enum class PVREvent;

  class CPVRChannelGroupInternal : public CPVRChannelGroup
  {
  public:
    /*!
     * @brief Create a new internal channel group.
     * @param bRadio True if this group holds radio channels.
     */
    explicit CPVRChannelGroupInternal(bool bRadio);

    explicit CPVRChannelGroupInternal(const CPVRChannelGroup &group);

    ~CPVRChannelGroupInternal(void) override;

    /**
     * @brief The amount of channels in this container.
     * @return The amount of channels in this container.
     */
    size_t GetNumHiddenChannels() const override { return m_iHiddenChannels; }

    /*!
     * @brief Callback for add-ons to update a channel.
     * @param channel The updated channel.
     * @param channelNumber A new channel number for the channel.
     * @return The new/updated channel.
     */
    CPVRChannelPtr UpdateFromClient(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber);

    /*!
     * @see CPVRChannelGroup::IsGroupMember
     */
    bool IsGroupMember(const CPVRChannelPtr &channel) const override;

    /*!
     * @see CPVRChannelGroup::AddToGroup
     */
    bool AddToGroup(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber, bool bUseBackendChannelNumbers) override;

    /*!
     * @see CPVRChannelGroup::RemoveFromGroup
     */
    bool RemoveFromGroup(const CPVRChannelPtr &channel) override;

    /*!
     * @brief Check whether the group name is still correct after the language setting changed.
     */
    void CheckGroupName(void);

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
    bool LoadFromClients(void) override;

    /*!
     * @brief Check if this group is the internal group containing all channels.
     * @return True if it's the internal group, false otherwise.
     */
    bool IsInternalGroup(void) const override { return true; }

    /*!
     * @brief Update the current channel list with the given list.
     *
     * Update the current channel list with the given list.
     * Only the new channels will be present in the passed list after this call.
     *
     * @param channels The channels to use to update this list.
     * @return True if everything went well, false otherwise.
     */
    bool UpdateGroupEntries(const CPVRChannelGroup &channels) override;

    /*!
     * @brief Add new channels to this group; updtae data.
     * @param channels The new channels to use for this group.
     * @param bUseBackendChannelNumbers True, if channel numbers from backends shall be used.
     * @return True if everything went well, false otherwise.
     */
    bool AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers) override;

    /*!
     * @brief Remove deleted channels from this group.
     * @param channels The new channels to use for this group.
     * @return The removed channels.
     */
    std::vector<CPVRChannelPtr> RemoveDeletedChannels(const CPVRChannelGroup &channels) override;

    /*!
     * @brief Refresh the channel list from the clients.
     */
    bool Update(void) override;

    /*!
     * @brief Load the channels from the database.
     *
     * Load the channels from the database.
     * If no channels are stored in the database, then the channels will be loaded from the clients.
     *
     * @return True when loaded successfully, false otherwise.
     */
    bool Load(void) override;

    /*!
     * @brief Update the vfs paths of all channels.
     */
    void UpdateChannelPaths(void);

    void CreateChannelEpg(const CPVRChannelPtr &channel, bool bForce = false);

    size_t m_iHiddenChannels; /*!< the amount of hidden channels in this container */

  private:
    void OnPVRManagerEvent(const PVREvent& event);
  };
}
