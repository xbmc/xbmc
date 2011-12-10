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

#include "XBDateTime.h"
#include "FileItem.h"
#include "addons/include/xbmc_pvr_types.h"
#include "utils/Observer.h"
#include "threads/CriticalSection.h"

namespace EPG
{
  class CEpg;
}

namespace PVR
{
  class CPVRChannelGroup;
  class CPVRChannelGroupInternal;
  class CPVRDatabase;
  class CPVREpgContainer;
  class CPVRChannelIconCacheJob;

  /** PVR Channel class */

  class CPVRChannel : public Observable
  {
    friend class CPVRChannelGroup;
    friend class CPVRChannelGroupInternal;
    friend class CPVRDatabase;
    friend class CPVREpgContainer;
    friend class EPG::CEpg;
    friend class CPVRChannelIconCacheJob;

  private:
    /*! @name XBMC related channel data
     */
    //@{
    int              m_iChannelId;              /*!< the identifier given to this channel by the TV database */
    bool             m_bIsRadio;                /*!< true if this channel is a radio channel, false if not */
    bool             m_bIsHidden;               /*!< true if this channel is hidden, false if not */
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

  public:
    /*! @brief Create a new channel */
    CPVRChannel(bool bRadio = false);
    CPVRChannel(const PVR_CHANNEL &channel, unsigned int iClientId);
    CPVRChannel(const CPVRChannel &channel);

    bool operator ==(const CPVRChannel &right) const;
    bool operator !=(const CPVRChannel &right) const;
    CPVRChannel &operator=(const CPVRChannel &channel);

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
     * @brief The identifier given to this channel by the TV database.
     * @return The identifier given to this channel by the TV database.
     */
    int ChannelID(void) const { return m_iChannelId; }

    /*!
     * @brief Set the identifier for this channel.
     * @param iDatabaseId The new channel ID
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelID(int iDatabaseId, bool bSaveInDb = false);

    /*!
     * @brief The channel number used by XBMC by the currently active group.
     * @return The channel number used by XBMC.
     */
    int ChannelNumber(void) const;

    /*!
     * @brief True if this channel is a radio channel, false if not.
     * @return True if this channel is a radio channel, false if not.
     */
    bool IsRadio(void) const { return m_bIsRadio; }

    /*!
     * @brief True if this channel is hidden. False if not.
     * @return True if this channel is hidden. False if not.
     */
    bool IsHidden(void) const { return m_bIsHidden; }

    /*!
     * @brief Set to true to hide this channel. Set to false to unhide it.
     *
     * Set to true to hide this channel. Set to false to unhide it.
     * The EPG of hidden channels won't be updated.
     * @param bIsHidden The new setting.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetHidden(bool bIsHidden, bool bSaveInDb = false);

    /*!
     * @brief True if a recording is currently running on this channel. False if not.
     * @return True if a recording is currently running on this channel. False if not.
     */
    bool IsRecording(void) const;

    /*!
     * @brief The path to the icon for this channel.
     * @return The path to the icon for this channel.
     */
    const CStdString &IconPath(void) const { return m_strIconPath; }

    /*!
     * @brief Set the path to the icon for this channel.
     * @param strIconPath The new path.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetIconPath(const CStdString &strIconPath, bool bSaveInDb = false);

    /*!
     * @brief The name for this channel used by XBMC.
     * @return The name for this channel used by XBMC.
     */
    const CStdString &ChannelName(void) const { return m_strChannelName; }

    /*!
     * @brief Set the name for this channel used by XBMC.
     * @param strChannelName The new channel name.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelName(const CStdString &strChannelName, bool bSaveInDb = false);

    /*!
     * @brief True if this channel is marked as virtual. False if not.
     * @return True if this channel is marked as virtual. False if not.
     */
    bool IsVirtual() const { return m_bIsVirtual; }

    /*!
     * @brief True if this channel is marked as virtual. False if not.
     * @param bIsVirtual The new value.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetVirtual(bool bIsVirtual, bool bSaveInDb = false);

    /*!
     * @brief Last time channel has been watched.
     * @return Time channel has been watched last.
     */
    time_t LastWatched() const { return m_iLastWatched; }

    /*!
     * @brief Last time channel has been watched
     * @param iLastWatched The new value.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetLastWatched(time_t iLastWatched, bool bSaveInDb = false);

    /*!
     * @brief True if this channel has no file or stream name
     * @return True if this channel has no file or stream name
     */
    bool IsEmpty() const;

    bool IsChanged() const { return m_bChanged; }
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
    int UniqueID(void) const { return m_iUniqueId; }

    /*!
     * @brief Change the unique identifier for this channel.
     * @param iUniqueId The new unique ID.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetUniqueID(int iUniqueId, bool bSaveInDb = false);

    /*!
     * @brief The identifier of the client that serves this channel.
     * @return The identifier of the client that serves this channel.
     */
    int ClientID(void) const { return m_iClientId; }

    /*!
     * @brief Set the identifier of the client that serves this channel.
     * @param iClientId The new ID.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetClientID(int iClientId, bool bSaveInDb = false);

    /*!
     * @brief The channel number on the client.
     * @return The channel number on the client.
     */
    int ClientChannelNumber(void) const { return m_iClientChannelNumber; }

    /*!
     * @brief Set the channel number on the client.
     *
     * Set the channel number on the client.
     * It will only be changed in this tag and won't change anything on the client.
     *
     * @param iClientChannelNumber The new channel number
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetClientChannelNumber(int iClientChannelNumber, bool bSaveInDb = false);

    /*!
     * @brief The name of this channel on the client.
     * @return The name of this channel on the client.
     */
    const CStdString &ClientChannelName(void) const { return m_strClientChannelName; }

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
    const CStdString &InputFormat(void) const { return m_strInputFormat; }

    /*!
     * @brief Set the stream input type
     * @param strInputFormat The new input format.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetInputFormat(const CStdString &strInputFormat, bool bSaveInDb = false);

    /*!
     * @brief The stream URL to access this channel.
     *
     * The stream URL to access this channel.
     * If this is empty, then the client should be used to read from the channel.
     *
     * @return The stream URL to access this channel.
     */
    const CStdString &StreamURL(void) const { return m_strStreamURL; }

    /*!
     * @brief Set the stream URL to access this channel.
     *
     * Set the stream URL to access this channel.
     * If this is empty, then the client should be used to read from the channel.
     *
     * @param strStreamURL The new stream URL.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetStreamURL(const CStdString &strStreamURL, bool bSaveInDb = false);

    /*!
     * @brief The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     * @return The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     */
    const CStdString &Path(void) const { return m_strFileNameAndPath; }

  private:
    /*!
     * @brief Update the path after the channel number in the internal group changed.
     */
    void UpdatePath(unsigned int iNewChannelNumber);

    /*!
     * @brief Update the encryption name after SetEncryptionSystem() has been called.
     */
    void UpdateEncryptionName(void);

    void SetCachedChannelNumber(unsigned int iChannelNumber);

  public:
    /*!
     * @brief Return true if this channel is encrypted.
     *
     * Return true if this channel is encrypted. Does not inform whether XBMC can play the file.
     * Decryption should be done by the client.
     *
     * @return Return true if this channel is encrypted.
     */
    bool IsEncrypted(void) const { return m_iClientEncryptionSystem > 0; }


    /*!
     * @brief Return the encryption system ID for this channel. 0 for FTA.
     *
     * Return the encryption system ID for this channel. 0 for FTA.
     * The values are documented on: http://www.dvb.org/index.php?id=174.
     *
     * @return Return the encryption system ID for this channel.
     */
    int EncryptionSystem(void) const { return m_iClientEncryptionSystem; }

    /*!
     * @brief Set the encryption ID (CAID) for this channel.
     * @param iClientEncryptionSystem The new CAID.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetEncryptionSystem(int iClientEncryptionSystem, bool bSaveInDb = false);

    /*!
     * @return A friendly name for the used encryption system.
     */
    const CStdString &EncryptionName() const { return m_strClientEncryptionName; }
    //@}

    /*! @name EPG methods
     */
    //@{

    /*!
     * @return The ID of the EPG table to use for this channel or -1 if it isn't set.
     */
    int EpgID() const { return m_iEpgId; };

    /*!
     * @brief Get the EPG table for this channel.
     * @return The EPG for this channel.
     */
    EPG::CEpg *GetEPG() const;

    /*!
     * @brief Create the EPG table for this channel.
     * @brief bForce Create a table, even if it already has been created before.
     * @return True if the table was created successfully, false otherwise.
     */
    bool CreateEPG(bool bForce = false);

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
    bool ClearEPG() const;

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
     * @brief Don't use an EPG for this channel if set to false.
     * @return Don't use an EPG for this channel if set to false.
     */
    bool EPGEnabled() const { return m_bEPGEnabled; }

    /*!
     * @brief Set to true if an EPG should be used for this channel. Set to false otherwise.
     * @param bEPGEnabled The new value.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetEPGEnabled(bool bEPGEnabled = true, bool bSaveInDb = false);

    /*!
     * @brief Get the name of the scraper to be used for this channel.
     *
     * Get the name of the scraper to be used for this channel.
     * The default is 'client', which means the EPG should be loaded from the backend.
     *
     * @return The name of the scraper to be used for this channel.
     */
    const CStdString &EPGScraper(void) const { return m_strEPGScraper; }

    /*!
     * @brief Set the name of the scraper to be used for this channel.
     *
     * Set the name of the scraper to be used for this channel.
     * Set to "client" to load the EPG from the backend
     *
     * @param strScraper The new scraper name.
     * @param bSaveInDb Save in the database or not.
     * @return True if the something changed, false otherwise.
     */
    bool SetEPGScraper(const CStdString &strScraper, bool bSaveInDb = false);

    //@}
  };
}
