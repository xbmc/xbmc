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

#include "XBDateTime.h"
#include "FileItem.h"
#include "addons/include/xbmc_pvr_types.h"
#include "utils/Observer.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"

#include <boost/shared_ptr.hpp>

namespace EPG
{
  class CEpg;
}

namespace PVR
{
  class CPVRDatabase;
  class CPVRChannelGroupInternal;

  class CPVRChannel;
  typedef boost::shared_ptr<PVR::CPVRChannel> CPVRChannelPtr;

  /** PVR Channel class */
  class CPVRChannel : public Observable, public ISerializable
  {
    friend class CPVRDatabase;
    friend class CPVRChannelGroupInternal;

  public:
    /*! @brief Create a new channel */
    CPVRChannel(bool bRadio = false);
    CPVRChannel(const PVR_CHANNEL &channel, unsigned int iClientId);
    CPVRChannel(const CPVRChannel &channel);

    bool operator ==(const CPVRChannel &right) const;
    bool operator !=(const CPVRChannel &right) const;
    CPVRChannel &operator=(const CPVRChannel &channel);

    virtual void Serialize(CVariant& value) const;

    /*! @name XBMC related channel methods
     */
    //@{

    /*!
     * @brief Delete this channel from the database and delete the corresponding EPG table if it exists.
     * @return True if it was deleted successfully, false otherwise.
     */
    bool Delete(void);

    /*!
     * @brief Update this channel tag with the data of the given channel tag.
     * @param channel The new channel data.
     * @return True if something changed, false otherwise.
     */
    bool UpdateFromClient(const CPVRChannel &channel);

    /*!
     * @brief Persists the changes in the database.
     * @param bQueueWrite Queue the change and write changes later.
     * @return True if the changes were saved succesfully, false otherwise.
     */
    bool Persist(bool bQueueWrite = false);

    /*!
     * @return The identifier given to this channel by the TV database.
     */
    int ChannelID(void) const;

    /*!
     * @return True when not persisted yet, false otherwise.
     */
    bool IsNew(void) const;

    /*!
     * @brief Set the identifier for this channel.
     * @param iDatabaseId The new channel ID
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelID(int iDatabaseId);

    /*!
     * @return The channel number used by XBMC by the currently active group.
     */
    int ChannelNumber(void) const;

    /*!
     * @return True if this channel is a radio channel, false if not.
     */
    bool IsRadio(void) const { return m_bIsRadio; }

    /*!
     * @return True if this channel is hidden. False if not.
     */
    bool IsHidden(void) const;

    /*!
     * @brief Set to true to hide this channel. Set to false to unhide it.
     *
     * Set to true to hide this channel. Set to false to unhide it.
     * The EPG of hidden channels won't be updated.
     * @param bIsHidden The new setting.
     * @return True if the something changed, false otherwise.
     */
    bool SetHidden(bool bIsHidden);

    /*!
     * @return True if this channel is locked. False if not.
     */
    bool IsLocked(void) const;

    /*!
     * @brief Set to true to lock this channel. Set to false to unlock it.
     *
     * Set to true to lock this channel. Set to false to unlock it.
     * Locked channels need can only be viewed if parental PIN entered.
     * @param bIsLocked The new setting.
     * @return True if the something changed, false otherwise.
     */
    bool SetLocked(bool bIsLocked);

    /*!
     * @return True if a recording is currently running on this channel. False if not.
     */
    bool IsRecording(void) const;

    /*!
     * @return The path to the icon for this channel.
     */
    CStdString IconPath(void) const;

    /*!
     * @return True if this user changed icon via GUI. False if not.
     */
    bool IsUserSetIcon(void) const;
	  
    /*!
     * @brief Set the path to the icon for this channel.
     * @param strIconPath The new path.
     * @param bIsUserSetIcon true if user changed the icon via GUI, false otherwise.
     * @return True if the something changed, false otherwise.
     */
    bool SetIconPath(const CStdString &strIconPath, bool bIsUserSetIcon = false);

    /*!
     * @return The name for this channel used by XBMC.
     */
    CStdString ChannelName(void) const;

    /*!
     * @brief Set the name for this channel used by XBMC.
     * @param strChannelName The new channel name.
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelName(const CStdString &strChannelName);

    /*!
     * @return True if this channel is marked as virtual. False if not.
     */
    bool IsVirtual(void) const;

    /*!
     * @brief True if this channel is marked as virtual. False if not.
     * @param bIsVirtual The new value.
     * @return True if the something changed, false otherwise.
     */
    bool SetVirtual(bool bIsVirtual);

    /*!
     * @return Time channel has been watched last.
     */
    time_t LastWatched() const;

    /*!
     * @brief Last time channel has been watched
     * @param iLastWatched The new value.
     * @return True if the something changed, false otherwise.
     */
    bool SetLastWatched(time_t iLastWatched);

    /*!
     * @brief True if this channel has no file or stream name
     * @return True if this channel has no file or stream name
     */
    bool IsEmpty() const;

    bool IsChanged() const;
    //@}

    /*! @name Client related channel methods
     */
    //@{

    /*!
     * @brief A unique identifier for this channel.
     *
     * A unique identifier for this channel.
     * It can be used to find the same channel on different providers
     *
     * @return The Unique ID.
     */
    int UniqueID(void) const;

    /*!
     * @brief Change the unique identifier for this channel.
     * @param iUniqueId The new unique ID.
     * @return True if the something changed, false otherwise.
     */
    bool SetUniqueID(int iUniqueId);

    /*!
     * @return The identifier of the client that serves this channel.
     */
    int ClientID(void) const;

    /*!
     * @brief Set the identifier of the client that serves this channel.
     * @param iClientId The new ID.
     * @return True if the something changed, false otherwise.
     */
    bool SetClientID(int iClientId);

    /*!
     * @return The channel number on the client.
     */
    int ClientChannelNumber(void) const;

    /*!
     * @brief Set the channel number on the client.
     *
     * Set the channel number on the client.
     * It will only be changed in this tag and won't change anything on the client.
     *
     * @param iClientChannelNumber The new channel number
     * @return True if the something changed, false otherwise.
     */
    bool SetClientChannelNumber(int iClientChannelNumber);

    /*!
     * @return The name of this channel on the client.
     */
    CStdString ClientChannelName(void) const;

    /*!
     * @brief Set the name of this channel on the client.
     *
     * Set the name of this channel on the client.
     * It will only be changed in this tag and won't change anything on the client.
     *
     * @param strClientChannelName The new channel name
     * @return True if the something changed, false otherwise.
     */
    bool SetClientChannelName(const CStdString &strClientChannelName);

    /*!
     * @brief The stream input type
     *
     * The stream input type
     * If it is empty, ffmpeg will try to scan the stream to find the right input format.
     * See "xbmc/cores/dvdplayer/Codecs/ffmpeg/libavformat/allformats.c" for a
     * list of the input formats.
     *
     * @return The stream input type
     */
    CStdString InputFormat(void) const;

    /*!
     * @brief Set the stream input type
     * @param strInputFormat The new input format.
     * @return True if the something changed, false otherwise.
     */
    bool SetInputFormat(const CStdString &strInputFormat);

    /*!
     * @brief The stream URL to access this channel.
     *
     * The stream URL to access this channel.
     * If this is empty, then the client should be used to read from the channel.
     *
     * @return The stream URL to access this channel.
     */
    CStdString StreamURL(void) const;

    /*!
     * @brief Set the stream URL to access this channel.
     *
     * Set the stream URL to access this channel.
     * If this is empty, then the client should be used to read from the channel.
     *
     * @param strStreamURL The new stream URL.
     * @return True if the something changed, false otherwise.
     */
    bool SetStreamURL(const CStdString &strStreamURL);

    /*!
     * @brief The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     * @return The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     */
    CStdString Path(void) const;

    void ToSortable(SortItem& sortable) const;

    /*!
     * @brief Update the path after the channel number in the internal group changed.
     * @param group The internal group that contains this channel
     * @param iNewChannelGroupPosition The new channel number in the group
     */
    void UpdatePath(CPVRChannelGroupInternal* group, unsigned int iNewChannelGroupPosition);

    /*!
     * @brief Return true if this channel is encrypted.
     *
     * Return true if this channel is encrypted. Does not inform whether XBMC can play the file.
     * Decryption should be done by the client.
     *
     * @return Return true if this channel is encrypted.
     */
    bool IsEncrypted(void) const;


    /*!
     * @brief Return the encryption system ID for this channel. 0 for FTA.
     *
     * Return the encryption system ID for this channel. 0 for FTA.
     * The values are documented on: http://www.dvb.org/index.php?id=174.
     *
     * @return Return the encryption system ID for this channel.
     */
    int EncryptionSystem(void) const;

    /*!
     * @brief Set the encryption ID (CAID) for this channel.
     * @param iClientEncryptionSystem The new CAID.
     * @return True if the something changed, false otherwise.
     */
    bool SetEncryptionSystem(int iClientEncryptionSystem);

    /*!
     * @return A friendly name for the used encryption system.
     */
    CStdString EncryptionName(void) const;
    //@}

    /*! @name EPG methods
     */
    //@{

    /*!
     * @return The ID of the EPG table to use for this channel or -1 if it isn't set.
     */
    int EpgID(void) const;

    /*!
     * @brief Change the id of the epg that is linked to this channel
     * @param iEpgId The new epg id
     */
    void SetEpgID(int iEpgId);

    /*!
     * @brief Get the EPG table for this channel.
     * @return The EPG for this channel.
     */
    EPG::CEpg *GetEPG(void) const;

    /*!
     * @brief Get the EPG table for this channel.
     * @param results The file list to store the results in.
     * @return The number of tables that were added.
     */
    int GetEPG(CFileItemList &results) const;

    /*!
     * @brief Clear the EPG for this channel.
     * @return True if it was cleared, false if not.
     */
    bool ClearEPG(void) const;

    /*!
     * @brief Get the EPG tag that is active on this channel now.
     *
     * Get the EPG tag that is active on this channel now.
     * Will return an empty tag if there is none.
     *
     * @return The EPG tag that is active on this channel now.
     */
    bool GetEPGNow(EPG::CEpgInfoTag &tag) const;

    /*!
     * @brief Get the EPG tag that is active on this channel next.
     *
     * Get the EPG tag that is active on this channel next.
     * Will return an empty tag if there is none.
     *
     * @return The EPG tag that is active on this channel next.
     */
    bool GetEPGNext(EPG::CEpgInfoTag &tag) const;

    /*!
     * @return Don't use an EPG for this channel if set to false.
     */
    bool EPGEnabled(void) const;

    /*!
     * @brief Set to true if an EPG should be used for this channel. Set to false otherwise.
     * @param bEPGEnabled The new value.
     * @return True if the something changed, false otherwise.
     */
    bool SetEPGEnabled(bool bEPGEnabled);

    /*!
     * @brief Get the name of the scraper to be used for this channel.
     *
     * Get the name of the scraper to be used for this channel.
     * The default is 'client', which means the EPG should be loaded from the backend.
     *
     * @return The name of the scraper to be used for this channel.
     */
    CStdString EPGScraper(void) const;

    /*!
     * @brief Set the name of the scraper to be used for this channel.
     *
     * Set the name of the scraper to be used for this channel.
     * Set to "client" to load the EPG from the backend
     *
     * @param strScraper The new scraper name.
     * @return True if the something changed, false otherwise.
     */
    bool SetEPGScraper(const CStdString &strScraper);

    void SetCachedChannelNumber(unsigned int iChannelNumber);

    bool CanRecord(void) const;
    //@}
  private:
    /*!
     * @brief Update the encryption name after SetEncryptionSystem() has been called.
     */
    void UpdateEncryptionName(void);

    /*! @name XBMC related channel data
     */
    //@{
    int              m_iChannelId;              /*!< the identifier given to this channel by the TV database */
    bool             m_bIsRadio;                /*!< true if this channel is a radio channel, false if not */
    bool             m_bIsHidden;               /*!< true if this channel is hidden, false if not */
    bool             m_bIsUserSetIcon;          /*!< true if user set the icon via GUI, false if not */
    bool             m_bIsLocked;               /*!< true if channel is locked, false if not */
    CStdString       m_strIconPath;             /*!< the path to the icon for this channel */
    CStdString       m_strChannelName;          /*!< the name for this channel used by XBMC */
    bool             m_bIsVirtual;              /*!< true if this channel is marked as virtual, false if not */
    time_t           m_iLastWatched;            /*!< last time channel has been watched */
    bool             m_bChanged;                /*!< true if anything in this entry was changed that needs to be persisted */
    unsigned int     m_iCachedChannelNumber;    /*!< the cached channel number in the selected group */
    //@}

    /*! @name EPG related channel data
     */
    //@{
    int              m_iEpgId;                  /*!< the id of the EPG for this channel */
    bool             m_bEPGCreated;             /*!< true if an EPG has been created for this channel */
    bool             m_bEPGEnabled;             /*!< don't use an EPG for this channel if set to false */
    CStdString       m_strEPGScraper;           /*!< the name of the scraper to be used for this channel */
    //@}

    /*! @name Client related channel data
     */
    //@{
    int              m_iUniqueId;               /*!< the unique identifier for this channel */
    int              m_iClientId;               /*!< the identifier of the client that serves this channel */
    int              m_iClientChannelNumber;    /*!< the channel number on the client */
    CStdString       m_strClientChannelName;    /*!< the name of this channel on the client */
    CStdString       m_strInputFormat;          /*!< the stream input type based on ffmpeg/libavformat/allformats.c */
    CStdString       m_strStreamURL;            /*!< URL of the stream. Use the client to read stream if this is empty */
    CStdString       m_strFileNameAndPath;      /*!< the filename to be used by PVRManager to open and read the stream */
    int              m_iClientEncryptionSystem; /*!< the encryption system used by this channel. 0 for FreeToAir, -1 for unknown */
    CStdString       m_strClientEncryptionName; /*!< the name of the encryption system used by this channel */
    //@}

    CCriticalSection m_critSection;
  };
}
