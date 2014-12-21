#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "FileItem.h"
#include "PVRChannel.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/JobManager.h"

#include <memory>

namespace EPG
{
  struct EpgSearchFilter;
}

namespace PVR
{
#define PVR_INTERNAL_GROUP_ID_RADIO 1
#define PVR_INTERNAL_GROUP_ID_TV    2

#define PVR_GROUP_TYPE_DEFAULT      0
#define PVR_GROUP_TYPE_INTERNAL     1
#define PVR_GROUP_TYPE_USER_DEFINED 2

  class CPVRChannelGroups;
  class CPVRChannelGroupInternal;
  class CPVRChannelGroupsContainer;

  typedef struct
  {
    CPVRChannelPtr channel;
    unsigned int   iChannelNumber;
    unsigned int   iSubChannelNumber;
  } PVRChannelGroupMember;
  
  enum EpgDateType
  {
    EPG_FIRST_DATE = 0,
    EPG_LAST_DATE = 1
  };
  
  class CPVRChannelGroup;
  typedef std::shared_ptr<PVR::CPVRChannelGroup> CPVRChannelGroupPtr;

  /** A group of channels */
  class CPVRChannelGroup : public Observable,
                           public IJobCallback,
                           public ISettingCallback

  {
    friend class CPVRChannelGroups;
    friend class CPVRChannelGroupInternal;
    friend class CPVRChannelGroupsContainer;
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
    CPVRChannelGroup(const PVR_CHANNEL_GROUP &group);

    /*!
     * @brief Copy constructor
     * @param group Source group
     */
    CPVRChannelGroup(const CPVRChannelGroup &group);

    virtual ~CPVRChannelGroup(void);

    bool operator ==(const CPVRChannelGroup &right) const;
    bool operator !=(const CPVRChannelGroup &right) const;

    /*!
     * @return The amount of group members
     */
    int Size(void) const;

    /*!
     * @brief Refresh the channel list from the clients.
     */
    virtual bool Update(void);

    /*!
     * @brief Change the channelnumber of a group. Used by CGUIDialogPVRChannelManager. Call SortByChannelNumber() and Renumber() after all changes are done.
     * @param channel The channel to change the channel number for.
     * @param iChannelNumber The new channel number.
     * @param iSubChannelNumber The new sub channel number.
     */
    bool SetChannelNumber(const CPVRChannel &channel, unsigned int iChannelNumber, unsigned int iSubChannelNumber = 0);

    /*!
     * @brief Move a channel from position iOldIndex to iNewIndex.
     * @param iOldChannelNumber The channel number of the channel to move.
     * @param iNewChannelNumber The new channel number.
     * @param bSaveInDb If true, save this change in the database.
     * @return True if the channel was moved successfully, false otherwise.
     */
    virtual bool MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb = true);

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
    virtual bool RemoveFromGroup(const CPVRChannel &channel);

    /*!
     * @brief Add a channel to this container.
     * @param channel The channel to add.
     * @param iChannelNumber The channel number of the channel number to add. Use -1 to add it at the end.
     * @return True if the channel was added, false otherwise.
     */
    virtual bool AddToGroup(CPVRChannel &channel, int iChannelNumber = 0);

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
    virtual bool IsGroupMember(const CPVRChannel &channel) const;

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
    virtual bool IsInternalGroup(void) const { return false; }

    /*!
     * @brief True if this group holds radio channels, false if it holds TV channels.
     * @return True if this group holds radio channels, false if it holds TV channels.
     */
    bool IsRadio(void) const { return m_bRadio; }

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

    //@}

    virtual void OnSettingChanged(const CSetting *setting);

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
     * @param iChannelNumber The channel number.
     * * @param iSubChannelNumber The sub channel number.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetByChannelNumber(unsigned int iChannelNumber, unsigned int iSubChannelNumber = 0) const;

    /*!
     * @brief Get the channel number in this group of the given channel.
     * @param channel The channel to get the channel number for.
     * @return The channel number in this group or 0 if the channel isn't a member of this group.
     */
    unsigned int GetChannelNumber(const CPVRChannel &channel) const;

    /*!
     * @brief Get the sub channel number in this group of the given channel.
     * @param channel The channel to get the sub channel number for.
     * @return The sub channel number in this group or 0 if the channel isn't a member of this group.
     */
    unsigned int GetSubChannelNumber(const CPVRChannel &channel) const;

    /*!
     * @brief Get the next channel in this group.
     * @param channel The current channel.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetByChannelUp(const CFileItem &channel) const { return GetByChannelUpDown(channel, true); }

    /*!
     * @brief Get the previous channel in this group.
     * @param channel The current channel.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetByChannelDown(const CFileItem &channel) const { return GetByChannelUpDown(channel, false); }

    /*!
     * @brief Get a channel given it's index in this container.
     * @param index The index in this container.
     * @return The channel or NULL if it wasn't found.
     */
    CFileItemPtr GetByIndex(unsigned int index) const;

    /*!
     * @brief Get the current index in this group of a channel.
     * @param channel The channel to get the index for.
     * @return The index or -1 if it wasn't found.
     */
    int GetIndex(const CPVRChannel &channel) const;

    /*!
     * @brief Get the list of channels in a group.
     * @param results The file list to store the results in.
     * @param bGroupMembers If true, get the channels that are in this group. Get the channels that are not in this group otherwise.
     * @return The amount of channels that were added to the list.
     */
    virtual int GetMembers(CFileItemList &results, bool bGroupMembers = true) const;

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
    virtual int GetNumHiddenChannels(void) const { return 0; }
    
    /*!
     * @brief The amount of channels in this container.
     * @return The amount of channels in this container.
     */
    int GetNumChannels(void) const;

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
     * @brief Reset the channel number cache if this is the selected group in the UI.
     */
    void ResetChannelNumberCache(void);

    void OnJobComplete(unsigned int jobID, bool success, CJob* job) {}

    /*!
     * @brief Get all EPG tables and apply a filter.
     * @param results The fileitem list to store the results in.
     * @param filter The filter to apply.
     * @return The amount of entries that were added.
     */
    int GetEPGSearch(CFileItemList &results, const EPG::EpgSearchFilter &filter);

    /*!
     * @brief Get all EPG tables.
     * @param results The fileitem list to store the results in.
     * @return The amount of entries that were added.
     */
    int GetEPGAll(CFileItemList &results);

    /*!
     * @brief Get all entries that are active now.
     * @param results The fileitem list to store the results in.
     * @return The amount of entries that were added.
     */
    int GetEPGNow(CFileItemList &results);

    /*!
     * @brief Get all entries that will be active next.
     * @param results The fileitem list to store the results in.
     * @return The amount of entries that were added.
     */
    int GetEPGNext(CFileItemList &results);
    
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

    bool UpdateChannel(const CFileItem &channel, bool bHidden, bool bVirtual, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const std::string &strChannelName, const std::string &strIconPath, const std::string &strStreamURL, bool bUserSetIcon = false);

    bool ToggleChannelLocked(const CFileItem &channel);

    virtual bool AddNewChannel(const CPVRChannel &channel, unsigned int iChannelNumber = 0) { return false; }

    /*!
     * @brief Get a channel given the channel number on the client.
     * @param iUniqueChannelId The unique channel id on the client.
     * @param iClientID The ID of the client.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByClient(int iUniqueChannelId, int iClientID) const;

    /*!
     * @brief Get a channel given it's unique ID.
     * @param iUniqueID The unique ID.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByUniqueID(int iUniqueID) const;

    void SetSelectedGroup(bool bSetTo);
    bool IsSelectedGroup(void) const;

  protected:
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

    virtual bool AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers);

    bool RemoveDeletedChannels(const CPVRChannelGroup &channels);

    /*!
     * @brief Create an EPG table for each channel.
     * @brief bForce Create the tables, even if they already have been created before.
     * @return True if all tables were created successfully, false otherwise.
     */
    virtual bool CreateChannelEpgs(bool bForce = false);

    /*!
     * @brief Remove invalid channels from this container.
     */
    void RemoveInvalidChannels(void);

    /*!
     * @brief Load the channels from the database.
     * @return True when loaded successfully, false otherwise.
     */
    virtual bool Load(void);

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
     * @brief Remove invalid channels and updates the channel numbers.
     * @return True if something changed, false otherwise.
     */
    virtual bool Renumber(void);

    /*!
     * @brief Sort the current channel list by client channel number.
     */
    void SortByClientChannelNumber(void);

    /*!
     * @brief Sort the current channel list by channel number.
     */
    void SortByChannelNumber(void);

    /*!
     * @brief Get the previous or next channel in this group.
     * @param channel The current channel.
     * @param bChannelUp True to get the next channel, false to get the previous one.
     * @return The requested channel or NULL if there is none.
     */
    CFileItemPtr GetByChannelUpDown(const CFileItem &channel, bool bChannelUp) const;

    void ResetChannelNumbers(void);

    /*!
     * @brief Get a channel given it's channel ID.
     * @param iChannelID The channel ID.
     * @return The channel or NULL if it wasn't found.
     */
    CPVRChannelPtr GetByChannelID(int iChannelID) const;

    bool             m_bRadio;                      /*!< true if this container holds radio channels, false if it holds TV channels */
    int              m_iGroupType;                  /*!< The type of this group */
    int              m_iGroupId;                    /*!< The ID of this group in the database */
    std::string      m_strGroupName;                /*!< The name of this group */
    bool             m_bLoaded;                     /*!< True if this container is loaded, false otherwise */
    bool             m_bChanged;                    /*!< true if anything changed in this group that hasn't been persisted, false otherwise */
    bool             m_bUsingBackendChannelOrder;   /*!< true to use the channel order from backends, false otherwise */
    bool             m_bUsingBackendChannelNumbers; /*!< true to use the channel numbers from 1 backend, false otherwise */
    bool             m_bSelectedGroup;              /*!< true when this is the selected group, false otherwise */
    bool             m_bPreventSortAndRenumber;     /*!< true when sorting and renumbering should not be done after adding/updating channels to the group */
    time_t           m_iLastWatched;                /*!< last time group has been watched */
    std::vector<PVRChannelGroupMember> m_members;
    CCriticalSection m_critSection;
    
  private:
    CDateTime GetEPGDate(EpgDateType epgDateType) const;
    
  };

  class CPVRPersistGroupJob : public CJob
  {
  public:
    CPVRPersistGroupJob(CPVRChannelGroupPtr group) { m_group = group; }
    virtual ~CPVRPersistGroupJob() {}
    const char *GetType() const { return "pvr-channelgroup-persist"; }

    virtual bool DoWork();

  private:
    CPVRChannelGroupPtr m_group;
  };
}
