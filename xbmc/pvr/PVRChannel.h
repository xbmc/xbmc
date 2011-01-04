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

#include "DateTime.h"
#include "FileItem.h"
#include "Observer.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVREpg;

class CPVRChannel : public Observable
{
  friend class CPVREpgs;
  friend class CTVDatabase;

private:
  /********** XBMC related channel data **********/
  long                           m_iDatabaseId;             /* the identifier given to this channel by the TV database */
  int                            m_iChannelNumber;          /* the channel number used by XBMC */
  int                            m_iChannelGroupId;         /* the identifier of the group where this channel belongs to */
  bool                           m_bIsRadio;                /* true if this channel is a radio channel, false if not */
  bool                           m_bIsHidden;               /* true if this channel is hidden, false if not */
  bool                           m_bClientIsRecording;      /* true if a recording is currently running on this channel, false if not */
  CStdString                     m_strIconPath;             /* the path to the icon for this channel */
  CStdString                     m_strChannelName;          /* the name for this channel used by XBMC */
  bool                           m_bIsVirtual;              /* true if this channel is marked as virtual, false if not */

  /********** EPG related channel data **********/
  CPVREpg *                      m_EPG;                     /* the EPG table for this channel */
  mutable const CPVREpgInfoTag * m_EPGNow;                  /* the EPG tag that is active on this channel now */
  bool                           m_bEPGEnabled;             /* don't use an EPG for this channel if set to false */
  CStdString                     m_strEPGScraper;           /* the name of the scraper to be used for this channel */

  /********** Client related channel data **********/
  int                            m_iUniqueId;               /* the unique identifier for this channel */
  int                            m_iClientId;               /* the identifier of the client that serves this channel */
  int                            m_iClientChannelNumber;    /* the channel number on the client */
  CStdString                     m_strClientChannelName;    /* the name of this channel on the client */
  CStdString                     m_strInputFormat;          /* the stream input type based on ffmpeg/libavformat/allformats.c */
  CStdString                     m_strStreamURL;            /* URL of the stream. Use the client to read stream if this is empty */
  CStdString                     m_strFileNameAndPath;      /* the filename to be used by PVRManager to open and read the stream */
  int                            m_iClientEncryptionSystem; /* the encryption system used by this channel. 0 for FreeToAir, -1 for unknown */

public:
  CPVRChannel();

  bool operator ==(const CPVRChannel &right) const;
  bool operator !=(const CPVRChannel &right) const;

  /********** XBMC related channel methods **********/

  /**
   * Updates this channel tag with the data of the given channel tag.
   * Returns true if something changed, false otherwise.
   */
  bool UpdateFromClient(const CPVRChannel &channel);

  /**
   * Persists the changes in the database.
   * Returns true if the changes were saved succesfully, false otherwise.
   */
  bool Persist(void);

  /**
   * The identifier given to this channel by the TV database.
   */
  long ChannelID(void) const { return m_iDatabaseId; }

  /**
   * Set the identifier for this channel.
   * Nothing will be changed in the database.
   */
  void SetChannelID(long iDatabaseId, bool bSaveInDb = false);

  /**
   * The channel number used by XBMC.
   */
  int ChannelNumber(void) const { return m_iChannelNumber; }

  /**
   * Change the channel number used by XBMC.
   * Nothing will be changed in the database.
   */
  void SetChannelNumber(int iChannelNumber, bool bSaveInDb = false);

  /**
   * The identifier of the group this channel belongs to.
   * -1 for undefined.
   */
  int GroupID(void) const { return m_iChannelGroupId; }

  /**
   * Set the identifier of the group this channel belongs to.
   * Nothing will be changed in the database.
   */
  void SetGroupID(int iChannelGroupId, bool bSaveInDb = false);

  /**
   * True if this channel is a radio channel, false if not.
   */
  bool IsRadio(void) const { return m_bIsRadio; }

  /**
   * Set to true if this channel is a radio channel. Set to false if not.
   * Nothing will be changed in the database.
   */
  void SetRadio(bool bIsRadio, bool bSaveInDb = false);

  /**
   * True if this channel is hidden. False if not.
   */
  bool IsHidden(void) const { return m_bIsHidden; }

  /**
   * Set to true to hide this channel. Set to false to unhide it.
   * The EPG of hidden channels won't be updated.
   * Nothing will be changed in the database.
   */
  void SetHidden(bool bIsHidden, bool bSaveInDb = false);

  /**
   * True if a recording is currently running on this channel. False if not.
   */
  bool IsRecording(void) const { return m_bClientIsRecording; }

  /**
   * Set to true if a recording is currently running on this channel. Set to false if not.
   * Nothing will be changed in the database.
   */
  void SetRecording(bool bClientIsRecording);

  /**
   * The path to the icon for this channel.
   */
  CStdString IconPath(void) const { return m_strIconPath; }

  /**
   * Set the path to the icon for this channel.
   */
  bool SetIconPath(const CStdString &strIconPath, bool bSaveInDb = false);

  /**
   * The name for this channel used by XBMC.
   */
  CStdString ChannelName(void) const { return m_strChannelName; }

  /**
   * Set the name for this channel used by XBMC.
   */
  void SetChannelName(const CStdString &strChannelName, bool bSaveInDb = false);

  /**
   * True if this channel is marked as virtual. False if not.
   */
  bool IsVirtual() const { return m_bIsVirtual; }

  /**
   * True if this channel is marked as virtual. False if not.
   */
  void SetVirtual(bool bIsVirtual, bool bSaveInDb = false);

  /**
   * True if this channel has no file or stream name
   */
  bool IsEmpty() const;

  /********** Client related channel methods **********/

  /**
   * A unique identifier for this channel.
   * It can be used to find the same channel on different providers
   */
  int UniqueID(void) const { return m_iUniqueId; }

  /**
   * Change the unique identifier for this channel.
   */
  void SetUniqueID(int iUniqueId, bool bSaveInDb = false);

  /**
   * The identifier of the client that serves this channel.
   */
  int ClientID(void) const { return m_iClientId; }

  /**
   * Set the identifier of the client that serves this channel.
   */
  void SetClientID(int iClientId, bool bSaveInDb = false);

  /**
   * The channel number on the client.
   */
  int ClientChannelNumber(void) const { return m_iClientChannelNumber; }

  /**
   * Set the channel number on the client.
   * It will only be changed in this tag and won't change anything on the client.
   */
  void SetClientChannelNumber(int iClientChannelNumber, bool bSaveInDb = false);

  /**
   * The name of this channel on the client.
   */
  CStdString ClientChannelName(void) const { return m_strClientChannelName; }
  /**
   * Set the name of this channel on the client.
   * It will only be changed in this tag and won't change anything on the client.
   */
  void SetClientChannelName(const CStdString &strClientChannelName, bool bSaveInDb = false);

  /**
   * The stream input type
   * If it is empty, ffmpeg will try to scan the stream to find the right input format.
   * See "xbmc/cores/dvdplayer/Codecs/ffmpeg/libavformat/allformats.c" for a
   * list of the input formats.
   */
  CStdString InputFormat(void) const { return m_strInputFormat; }

  /**
   * Set the stream input type
   */
  void SetInputFormat(const CStdString &strInputFormat, bool bSaveInDb = false);

  /**
   * The stream URL to access this channel.
   * If this is empty, then the client should be used to read from the channel.
   */
  CStdString StreamURL(void) const { return m_strStreamURL; }

  /**
   * Set the stream URL to access this channel.
   * If this is empty, then the client should be used to read from the channel.
   */
  void SetStreamURL(const CStdString &strStreamURL, bool bSaveInDb = false);

  /**
   * The path in the XBMC VFS to be used by PVRManager to open and read the stream.
   */
  CStdString Path(void) const { return m_strFileNameAndPath; }

private:
  /**
   * Updates the path after the channel number or radio flag has changed
   */
  void UpdatePath(void);

public:
  /**
   * Return true if this channel is encrypted. Does not inform whether XBMC can play the file.
   * Decryption should be done by the client.
   */
  bool IsEncrypted(void) const { return m_iClientEncryptionSystem > 0; }

  /**
   * Return the encryption system ID for this channel. 0 for FTA. The values are documented
   * on: http://www.dvb.org/index.php?id=174.
   */
  int EncryptionSystem(void) const { return m_iClientEncryptionSystem; }

  /**
   * Set the encryption ID (CAID) for this channel.
   */
  void SetEncryptionSystem(int iClientEncryptionSystem, bool bSaveInDb = false);

  /**
   * Return a friendly name for the used encryption system.
   */
  CStdString EncryptionName() const;

  /********** EPG methods **********/

  /**
   * Get the EPG table for this channel.
   * Will be created if it doesn't exist.
   */
  CPVREpg *GetEPG();

  /**
   * Clear the EPG for this channel.
   */
  bool ClearEPG();

  /**
   * Get the EPG tag that is active on this channel now.
   * Will return an empty tag if there is none.
   */
  const CPVREpgInfoTag* GetEPGNow() const;

  /**
   * Get the EPG tag that is active on this channel next
   * Will return an empty tag if there is none.
   */
  const CPVREpgInfoTag* GetEPGNext() const;

  /**
   * Don't use an EPG for this channel if set to false
   */
  bool EPGEnabled() const { return m_bEPGEnabled; }

  /**
   * Set to true if an EPG should be used for this channel. Set to false otherwise.
   */
  void SetEPGEnabled(bool bEPGEnabled = true, bool bSaveInDb = false);

  /**
   * Get the name of the scraper to be used for this channel
   */
  CStdString EPGScraper(void) const { return m_strEPGScraper; }

  /**
   * Set the name of the scraper to be used for this channel.
   * Set to "client" to load the EPG from the backend
   */
  void SetEPGScraper(const CStdString &strScraper, bool bSaveInDb = false);

private:
  /**
   * Updates the EPG "now" and "next" pointers
   */
  void UpdateEPGPointers();
};
