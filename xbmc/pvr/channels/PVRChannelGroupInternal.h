#pragma once

/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "PVRChannelGroup.h"

namespace PVR
{
  class CPVRChannelGroups;
  class CPVRDatabase;

  /** XBMC's internal group, the group containing all channels */

  class CPVRChannelGroupInternal : public CPVRChannelGroup
  {
    friend class CPVRChannelGroups;
    friend class CPVRDatabase;

  public:
    /*!
     * @brief Create a new internal channel group.
     * @param bRadio True if this group holds radio channels.
     */
    CPVRChannelGroupInternal(bool bRadio);

    CPVRChannelGroupInternal(const CPVRChannelGroup &group);

    virtual ~CPVRChannelGroupInternal(void);

    /**
     * @brief The amount of channels in this container.
     * @return The amount of channels in this container.
     */
    int GetNumHiddenChannels() const { return m_iHiddenChannels; }

    /*!
     * @brief Add or update a channel in this table.
     * @param channel The channel to update.
     * @return True if the channel was updated and persisted.
     */
    bool UpdateChannel(const CPVRChannel &channel);

    /*!
     * @brief Add a channel to this internal group.
     * @param iChannelNumber The channel number to use for this channel or 0 to add it to the back.
     * @param bSortAndRenumber Set to false to not to sort the group after adding a channel
     */
    bool InsertInGroup(CPVRChannel &channel, int iChannelNumber = 0, bool bSortAndRenumber = true);

    /*!
     * @brief Callback for add-ons to update a channel.
     * @param channel The updated channel.
     * @return True if the channel has been updated succesfully, false otherwise.
     */
    void UpdateFromClient(const CPVRChannel &channel, unsigned int iChannelNumber = 0);

    /*!
     * @see CPVRChannelGroup::IsGroupMember
     */
    bool IsGroupMember(const CPVRChannel &channel) const;

    /*!
     * @see CPVRChannelGroup::AddToGroup
     */
    bool AddToGroup(CPVRChannel &channel, int iChannelNumber = 0, bool bSortAndRenumber = true);

    /*!
     * @see CPVRChannelGroup::RemoveFromGroup
     */
    bool RemoveFromGroup(const CPVRChannel &channel);

    /*!
     * @see CPVRChannelGroup::MoveChannel
     */
    bool MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb = true);

    /*!
     * @see CPVRChannelGroup::GetMembers
     */
    int GetMembers(CFileItemList &results, bool bGroupMembers = true) const;

    /*!
     * @brief Check whether the group name is still correct after the language setting changed.
     */
    void CheckGroupName(void);

    /*!
     * @brief Create an EPG table for each channel.
     * @brief bForce Create the tables, even if they already have been created before.
     * @return True if all tables were created successfully, false otherwise.
     */
    bool CreateChannelEpgs(bool bForce = false);

    bool AddNewChannel(const CPVRChannel &channel, unsigned int iChannelNumber = 0) { UpdateFromClient(channel, iChannelNumber); return true; }

  protected:
    /*!
     * @brief Load all channels from the database.
     * @param bCompress Compress the database after changing anything.
     * @return The amount of channels that were loaded.
     */
    int LoadFromDb(bool bCompress = false);

    /*!
     * @brief Load all channels from the clients.
     * @return The amount of channels that were loaded.
     */
    int LoadFromClients(void);

    /*!
     * @brief Check if this group is the internal group containing all channels.
     * @return True if it's the internal group, false otherwise.
     */
    bool IsInternalGroup(void) const { return true; }

    /*!
     * @brief Update the current channel list with the given list.
     *
     * Update the current channel list with the given list.
     * Only the new channels will be present in the passed list after this call.
     *
     * @param channels The channels to use to update this list.
     * @return True if everything went well, false otherwise.
     */
    bool UpdateGroupEntries(const CPVRChannelGroup &channels);

    bool AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers);

    /*!
     * @brief Refresh the channel list from the clients.
     */
    bool Update(void);

    /*!
     * @brief Remove invalid channels and updates the channel numbers.
     */
    bool Renumber(void);

    /*!
     * @brief Load the channels from the database.
     *
     * Load the channels from the database.
     * If no channels are stored in the database, then the channels will be loaded from the clients.
     *
     * @return The amount of channels that were added.
     */
    int Load(void);

    /*!
     * @brief Update the vfs paths of all channels.
     */
    void UpdateChannelPaths(void);

    void CreateChannelEpg(CPVRChannelPtr channel, bool bForce = false);

    int m_iHiddenChannels; /*!< the amount of hidden channels in this container */
  };
}
