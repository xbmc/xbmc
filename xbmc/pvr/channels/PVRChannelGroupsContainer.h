#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PVRChannelGroups.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

namespace PVR
{
  class CPVRManager;
  class CPVRChannelsUpdateJob;
  class CPVRChannelGroupsUpdateJob;

  class CPVRChannelGroupsContainer
  {
    friend class CPVRManager;
    friend class CPVRChannelsUpdateJob;
    friend class CPVRChannelGroupsUpdateJob;

  public:
    /*!
     * @brief Create a new container for all channel groups
     */
    CPVRChannelGroupsContainer(void);

    /*!
     * @brief Destroy this container.
     */
    virtual ~CPVRChannelGroupsContainer(void);

    /*!
     * @brief Load all channel groups and all channels in those channel groups.
     * @return True if all groups were loaded, false otherwise.
     */
    virtual bool Load(void);

    /*!
     * @brief Unload and destruct all channel groups and all channels in them.
     */
    virtual void Unload(void);

    /*!
     * @brief Get the TV channel groups.
     * @return The TV channel groups.
     */
    virtual CPVRChannelGroups *GetTV(void) const { return Get(false); }

    /*!
     * @brief Get the radio channel groups.
     * @return The radio channel groups.
     */
    virtual CPVRChannelGroups *GetRadio(void) const { return Get(true); }

    /*!
     * @brief Get the radio or TV channel groups.
     * @param bRadio If true, get the radio channel groups. Get the TV channel groups otherwise.
     * @return The requested groups.
     */
    virtual CPVRChannelGroups *Get(bool bRadio) const;

    /*!
     * @brief Get the group containing all TV channels.
     * @return The group containing all TV channels.
     */
    virtual CPVRChannelGroupInternal *GetGroupAllTV(void)  const{ return GetGroupAll(false); }

    /*!
     * @brief Get the group containing all radio channels.
     * @return The group containing all radio channels.
     */
    virtual CPVRChannelGroupInternal *GetGroupAllRadio(void)  const{ return GetGroupAll(true); }

    /*!
     * @brief Get the group containing all TV or radio channels.
     * @param bRadio If true, get the group containing all radio channels. Get the group containing all TV channels otherwise.
     * @return The requested group.
     */
    virtual CPVRChannelGroupInternal *GetGroupAll(bool bRadio) const;

    /*!
     * @brief Get a group given it's ID.
     * @param bRadio If true, search the radio channel container. Search the TV channels otherwise.
     * @param iGroupId The ID of the group.
     * @return The requested group or NULL if it wasn't found.
     */
    virtual const CPVRChannelGroup *GetById(bool bRadio, int iGroupId) const;

    /*!
     * @brief Get a group given it's ID.
     * @param iGroupId The ID of the group.
     * @return The requested group or NULL if it wasn't found.
     */
    virtual const CPVRChannelGroup *GetByIdFromAll(int iGroupId) const;

    /*!
     * @brief Get a channel given it's database ID.
     * @param iChannelId The ID of the channel.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetChannelById(int iChannelId) const;

    /*!
     * @brief Get a channel given it's EPG ID.
     * @param iEpgId The EPG ID of the channel.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetChannelByEpgId(int iEpgId) const;

    /*!
     * @brief Get the groups list for a directory.
     * @param strBase The directory path.
     * @param results The file list to store the results in.
     * @param bRadio Get radio channels or tv channels.
     * @return True if the list was filled succesfully.
     */
    virtual bool GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio);

    /*!
     * @brief Get a channel given it's path.
     * @param strPath The path.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetByPath(const CStdString &strPath);

    /*!
     * @brief Get the directory for a path.
     * @param strPath The path.
     * @param results The file list to store the results in.
     * @return True if the directory was found, false if not.
     */
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &results);

    /*!
     * @brief The total amount of unique channels in all containers.
     * @return The total amount of unique channels in all containers.
     */
    virtual int GetNumChannelsFromAll(void);

    /*!
     * @brief Get the group that is currently selected in the UI.
     * @param bRadio True to get the selected radio group, false to get the selected TV group.
     * @return The selected group.
     */
    virtual CPVRChannelGroup *GetSelectedGroup(bool bRadio) const;

    /*!
     * @brief Get a channel given it's channel ID from all containers.
     * @param iClientChannelNumber The channel number on the client.
     * @param iClientID The ID of the client.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetByUniqueID(int iClientChannelNumber, int iClientID);

    /*!
     * @brief Get a channel given it's channel ID from all containers.
     * @param iChannelID The channel ID.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetByChannelIDFromAll(int iChannelID);

    virtual const CPVRChannel *GetByClientFromAll(unsigned int iClientId, unsigned int iChannelUid);

    /*!
     * @brief Get a channel given it's unique ID.
     * @param iUniqueID The unique ID of the channel.
     * @return The channel or NULL if it wasn't found.
     */
    virtual const CPVRChannel *GetByUniqueIDFromAll(int iUniqueID);

    /*!
     * @brief Try to find missing channel icons automatically
     */
    virtual void SearchMissingChannelIcons(void);

    /*!
     * @brief The channel that was played last that has a valid client or NULL if there was none.
     * @return The requested channel.
     */
    virtual const CPVRChannel *GetLastPlayedChannel(void) const;

  protected:
    /*!
     * @brief Update the contents of all the groups in this container.
     * @param bChannelsOnly Set to true to only update channels, not the groups themselves.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool Update(bool bChannelsOnly = false);

    CPVRChannelGroups *m_groupsRadio; /*!< all radio channel groups */
    CPVRChannelGroups *m_groupsTV;    /*!< all TV channel groups */
    CCriticalSection   m_critSection;
    bool               m_bUpdateChannelsOnly;
    bool               m_bIsUpdating;
  };
}
