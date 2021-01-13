/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h"
#include "pvr/channels/PVRChannelNumber.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CDateTime;

namespace PVR
{
  enum class PVREvent;

  class CPVREpg;
  class CPVREpgInfoTag;
  class CPVRRadioRDSInfoTag;

  class CPVRChannel : public ISerializable, public ISortable
  {
    friend class CPVRDatabase;

  public:
    explicit CPVRChannel(bool bRadio);
    CPVRChannel(const PVR_CHANNEL& channel, unsigned int iClientId);

    virtual ~CPVRChannel();

    bool operator ==(const CPVRChannel& right) const;
    bool operator !=(const CPVRChannel& right) const;

    void Serialize(CVariant& value) const override;

    /*! @name XBMC related channel methods
     */
    //@{

    /*!
     * @brief Delete this channel from the database and delete the corresponding EPG table if it exists.
     * @return True if it was deleted successfully, false otherwise.
     */
    bool QueueDelete();

    /*!
     * @brief Update this channel tag with the data of the given channel tag.
     * @param channel The new channel data.
     * @return True if something changed, false otherwise.
     */
    bool UpdateFromClient(const std::shared_ptr<CPVRChannel>& channel);

    /*!
     * @brief Persists the changes in the database.
     * @return True if the changes were saved successfully, false otherwise.
     */
    bool Persist();

    /*!
     * @return The identifier given to this channel by the TV database.
     */
    int ChannelID() const;

    /*!
     * @return True when not persisted yet, false otherwise.
     */
    bool IsNew() const;

    /*!
     * @brief Set the identifier for this channel.
     * @param iDatabaseId The new channel ID
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelID(int iDatabaseId);

    /*!
     * @brief Set the channel number for this channel.
     * @param channelNumber The new channel number
     */
    void SetChannelNumber(const CPVRChannelNumber& channelNumber);

    /*!
     * @brief Set the client channel number for this channel.
     * @param clientChannelNumber The new client channel number
     */
    void SetClientChannelNumber(const CPVRChannelNumber& clientChannelNumber);

    /*!
     * @brief Get the channel number for this channel.
     * @return The channel number.
     */
    const CPVRChannelNumber& ChannelNumber() const;

    /*!
     * @return True if this channel is a radio channel, false if not.
     */
    bool IsRadio() const { return m_bIsRadio; }

    /*!
     * @return True if this channel is hidden. False if not.
     */
    bool IsHidden() const;

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
    bool IsLocked() const;

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
     * @brief Obtain the Radio RDS data for this channel, if available.
     * @return The Radio RDS data or nullptr.
     */
    std::shared_ptr<CPVRRadioRDSInfoTag> GetRadioRDSInfoTag() const;

    /*!
     * @brief Set the Radio RDS data for the channel.
     * @param tag The RDS data.
     */
    void SetRadioRDSInfoTag(const std::shared_ptr<CPVRRadioRDSInfoTag>& tag);

    /*!
     * @return True if this channel has archive support, false otherwise
     */
    bool HasArchive() const;

    /*!
     * @return The path to the icon for this channel.
     */
    std::string IconPath() const;

    /*!
     * @return True if this user changed icon via GUI. False if not.
     */
    bool IsUserSetIcon() const;

    /*!
     * @return whether the user has changed the channel name through the GUI
     */
    bool IsUserSetName() const;

    /*!
     * @brief Set the path to the icon for this channel.
     * @param strIconPath The new path.
     * @param bIsUserSetIcon true if user changed the icon via GUI, false otherwise.
     * @return True if the something changed, false otherwise.
     */
    bool SetIconPath(const std::string& strIconPath, bool bIsUserSetIcon = false);

    /*!
     * @return The name for this channel used by XBMC.
     */
    std::string ChannelName() const;

    /*!
     * @brief Set the name for this channel used by XBMC.
     * @param strChannelName The new channel name.
     * @param bIsUserSetName whether the change was triggered by the user directly
     * @return True if the something changed, false otherwise.
     */
    bool SetChannelName(const std::string& strChannelName, bool bIsUserSetName = false);

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

    /*!
     * @brief reset changed flag after persist
     */
    void Persisted();
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
    int UniqueID() const;

    /*!
     * @return The identifier of the client that serves this channel.
     */
    int ClientID() const;

    /*!
     * @brief Set the identifier of the client that serves this channel.
     * @param iClientId The new ID.
     * @return True if the something changed, false otherwise.
     */
    bool SetClientID(int iClientId);

    /*!
     * Get the channel number on the client.
     * @return The channel number on the client.
     */
    const CPVRChannelNumber& ClientChannelNumber() const;

    /*!
     * @return The name of this channel on the client.
     */
    std::string ClientChannelName() const;

    /*!
     * @brief The stream input mime type
     *
     * The stream input type
     * If it is empty, ffmpeg will try to scan the stream to find the right input format.
     * See https://www.iana.org/assignments/media-types/media-types.xhtml for a
     * list of the input formats.
     *
     * @return The stream input type
     */
    std::string MimeType() const;

    /*!
     * @brief The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     * @return The path in the XBMC VFS to be used by PVRManager to open and read the stream.
     */
    std::string Path() const;

    // ISortable implementation
    void ToSortable(SortItem& sortable, Field field) const override;

    /*!
     * @brief Update the channel path
     * @param channelGroup The (new) name of the group this channel belongs to
     */
    void UpdatePath(const std::string& channelGroup);

    /*!
     * @return Storage id for this channel in CPVRChannelGroup
     */
    std::pair<int, int> StorageId() const { return std::make_pair(m_iClientId, m_iUniqueId); }

    /*!
     * @brief Return true if this channel is encrypted.
     *
     * Return true if this channel is encrypted. Does not inform whether XBMC can play the file.
     * Decryption should be done by the client.
     *
     * @return Return true if this channel is encrypted.
     */
    bool IsEncrypted() const;

    /*!
     * @brief Return the encryption system ID for this channel. 0 for FTA.
     *
     * Return the encryption system ID for this channel. 0 for FTA.
     * The values are documented on: http://www.dvb.org/index.php?id=174.
     *
     * @return Return the encryption system ID for this channel.
     */
    int EncryptionSystem() const;

    /*!
     * @return A friendly name for the used encryption system.
     */
    std::string EncryptionName() const;
    //@}

    /*! @name EPG methods
     */
    //@{

    /*!
     * @return The ID of the EPG table to use for this channel or -1 if it isn't set.
     */
    int EpgID() const;

    /*!
     * @brief Create the EPG for this channel, if it does not yet exist
     * @return true if a new epg was created, false otherwise.
     */
    bool CreateEPG();

    /*!
     * @brief Get the EPG table for this channel.
     * @return The EPG for this channel.
     */
    std::shared_ptr<CPVREpg> GetEPG() const;

    /*!
     * @brief Get the EPG tags for this channel.
     * @return The tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEpgTags() const;

    /*!
     * @brief Get all EPG tags for the given time frame, including "gap" tags.
     * @param timelineStart Start of time line.
     * @param timelineEnd End of time line.
     * @param minEventEnd The minimum end time of the events to return.
     * @param maxEventStart The maximum start time of the events to return.
     * @return The matching tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEPGTimeline(
        const CDateTime& timelineStart,
        const CDateTime& timelineEnd,
        const CDateTime& minEventEnd,
        const CDateTime& maxEventStart) const;

    /*!
     * @brief Create a "gap" EPG tag.
     * @param start Start of gap.
     * @param end End of gap.
     * @return The tag.
     */
    std::shared_ptr<CPVREpgInfoTag> CreateEPGGapTag(const CDateTime& start,
                                                    const CDateTime& end) const;

    /*!
     * @brief Clear the EPG for this channel.
     * @return True if it was cleared, false if not.
     */
    bool ClearEPG() const;

    /*!
     * @brief Get the EPG tag that is now active on this channel.
     *
     * Get the EPG tag that is now active on this channel.
     * Will return an empty tag if there is none.
     *
     * @return The EPG tag that is now active.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEPGNow() const;

    /*!
     * @brief Get the EPG tag that was previously active on this channel.
     *
     * Get the EPG tag that was previously active on this channel.
     * Will return an empty tag if there is none.
     *
     * @return The EPG tag that was previously activ.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEPGPrevious() const;

    /*!
     * @brief Get the EPG tag that will be next active on this channel.
     *
     * Get the EPG tag that will be next active on this channel.
     * Will return an empty tag if there is none.
     *
     * @return The EPG tag that will be next active.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEPGNext() const;

    /*!
     * @return Don't use an EPG for this channel if set to false.
     */
    bool EPGEnabled() const;

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
    std::string EPGScraper() const;

    /*!
     * @brief Set the name of the scraper to be used for this channel.
     *
     * Set the name of the scraper to be used for this channel.
     * Set to "client" to load the EPG from the backend
     *
     * @param strScraper The new scraper name.
     * @return True if the something changed, false otherwise.
     */
    bool SetEPGScraper(const std::string& strScraper);

    bool CanRecord() const;

    static std::string GetEncryptionName(int iCaid);

    /*!
     * @brief Get the client order for this channel
     * @return iOrder The order for this channel
     */
    int ClientOrder() const { return m_iOrder; }

    /*!
     * @brief Change the client order for this channel
     * @param iOrder The new order for this channel
     */
    void SetClientOrder(int iOrder);

    /*!
     * @brief CEventStream callback for PVR events.
     * @param event The event.
     */
    void Notify(const PVREvent& event);

    /*!
     * @brief Lock the instance. No other thread gets access to this channel until Unlock was called.
     */
    void Lock() { m_critSection.lock(); }

    /*!
     * @brief Unlock the instance. Other threads may get access to this channel again.
     */
    void Unlock() { m_critSection.unlock(); }

    //@}
  private:
    CPVRChannel();
    CPVRChannel(const CPVRChannel& tag) = delete;
    CPVRChannel& operator=(const CPVRChannel& channel) = delete;

    /*!
     * @brief Update the encryption name after SetEncryptionSystem() has been called.
     */
    void UpdateEncryptionName();

    /*!
     * @brief Reset the EPG instance pointer.
     */
    void ResetEPG();

    /*! @name XBMC related channel data
     */
    //@{
    int m_iChannelId = -1; /*!< the identifier given to this channel by the TV database */
    bool m_bIsRadio = false; /*!< true if this channel is a radio channel, false if not */
    bool m_bIsHidden = false; /*!< true if this channel is hidden, false if not */
    bool m_bIsUserSetName = false; /*!< true if user set the channel name via GUI, false if not */
    bool m_bIsUserSetIcon = false; /*!< true if user set the icon via GUI, false if not */
    bool m_bIsLocked = false; /*!< true if channel is locked, false if not */
    std::string m_strIconPath; /*!< the path to the icon for this channel */
    std::string m_strChannelName; /*!< the name for this channel used by XBMC */
    time_t m_iLastWatched = 0; /*!< last time channel has been watched */
    bool m_bChanged = false; /*!< true if anything in this entry was changed that needs to be persisted */
    CPVRChannelNumber m_channelNumber; /*!< the active channel number this channel has in the currently selected channel group */
    std::shared_ptr<CPVRRadioRDSInfoTag> m_rdsTag; /*! < the radio rds data, if available for the channel. */
    bool m_bHasArchive = false; /*!< true if this channel supports archive */
    //@}

    /*! @name EPG related channel data
     */
    //@{
    int m_iEpgId = -1; /*!< the id of the EPG for this channel */
    bool m_bEPGEnabled = false; /*!< don't use an EPG for this channel if set to false */
    std::string m_strEPGScraper = "client"; /*!< the name of the scraper to be used for this channel */
    std::shared_ptr<CPVREpg> m_epg;
    //@}

    /*! @name Client related channel data
     */
    //@{
    int m_iUniqueId = -1; /*!< the unique identifier for this channel */
    int m_iClientId = -1; /*!< the identifier of the client that serves this channel */
    CPVRChannelNumber m_clientChannelNumber; /*!< the channel number on the client for the currently selected channel group */
    std::string m_strClientChannelName; /*!< the name of this channel on the client */
    std::string
        m_strMimeType; /*!< the stream input type based mime type, see @ref https://www.iana.org/assignments/media-types/media-types.xhtml#video */
    std::string m_strFileNameAndPath; /*!< the filename to be used by PVRManager to open and read the stream */
    int m_iClientEncryptionSystem = -1; /*!< the encryption system used by this channel. 0 for FreeToAir, -1 for unknown */
    std::string m_strClientEncryptionName; /*!< the name of the encryption system used by this channel */
    int m_iOrder = 0; /*!< the order from this channels currently selected group memeber */
    //@}

    mutable CCriticalSection m_critSection;
  };
}
