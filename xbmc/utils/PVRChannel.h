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
  CPVREpg *                      m_Epg;
  mutable const CPVREpgInfoTag * m_epgNow;

  /* XBMC related channel data */
  long                           m_iDatabaseId;             /**< \brief Database number */
  int                            m_iChannelNumber;          /**< \brief Channel number for channels on XBMC */
  int                            m_iChannelGroupId;         /**< \brief Channel group identfier */
  bool                           m_bIsRadio;                /**< \brief Radio channel */
  bool                           m_bIsHidden;               /**< \brief Channel is hide inside filelists */
  bool                           m_bClientIsRecording;      /**< \brief True if channel is currently recording */
  bool                           m_bGrabEpg;                /**< \brief Load EPG if set to true */
  CStdString                     m_strGrabber;              /**< \brief The EPG grabber name (client for backend reading) */
  CStdString                     m_strIconPath;             /**< \brief Path to the logo image */
  CStdString                     m_strChannelName;          /**< \brief Channel name */
  int                            m_iCountWatched;           /**< \brief The count how much this channel was selected */
  long                           m_iSecondsWatched;         /**< \brief How many seconds this channel was watched */
  CDateTime                      m_lastTimeWatched;         /**< \brief The Date where this channel was selected last time */
  bool                           m_bIsVirtual;              /**< \brief Is a user defined virtual channel if true */

  long                           m_iPortalMasterChannel;    /**< \brief If it is a Portal Slave channel here is the master channel id or 0 for master, -1 for no portal */
  std::vector<CPVRChannel *>     m_PortalChannels;          /**< \brief Stores the slave portal channels if this is a master */

  /* Client related channel data */
  int                            m_iUniqueId;               /**< \brief Unique Id for this channel */
  int                            m_iClientId;               /**< \brief Id of client channel come from */
  int                            m_iClientChannelNumber;    /**< \brief Channel number on client */
  int                            m_iClientEncryptionSystem; /**< \brief Encryption System, 0 for FreeToAir, -1 unknown */
  CStdString                     m_strClientChannelName;    /**< \brief Channel name on client */

  CStdString                     m_strInputFormat;          /**< \brief The stream input type based upon ffmpeg/libavformat/allformats.c */
  CStdString                     m_strStreamURL;            /**< \brief URL of the stream, if empty use Client to read stream */
  CStdString                     m_strFileNameAndPath;      /**< \brief Filename for PVRManager to open and read stream */

  std::vector<long>              m_linkedChannels;          /**< \brief Channels linked to this channel */

  void UpdateEpgPointers();

public:
  CPVRChannel();
  virtual ~CPVRChannel();
  void ResetChannelEPGLinks();
       ///< Clear the EPG links
  bool IsEmpty() const;
       ///< True if no required data is present inside the tag.

  const CPVREpg *GetEpg() const { return m_Epg; }

  bool operator ==(const CPVRChannel &right) const;
  bool operator !=(const CPVRChannel &right) const;

  /* Channel information */
  CStdString ChannelName(void) const { return m_strChannelName; }
       ///< Return the currently by XBMC used name for this channel.
  void SetChannelName(CStdString name);
       ///< Set the name, XBMC uses for this channel.

  int ChannelNumber(void) const { return m_iChannelNumber; }
       ///< Return the currently by XBMC used channel number for this channel.
  void SetChannelNumber(int Number);
       ///< Change the XBMC number for this channel.

  CStdString ClientChannelName(void) const { return m_strClientChannelName; }
       ///< Return the name used by the client driver on the backend.
  void SetClientChannelName(CStdString name);
       ///< Set the name used by the client (is changed only in this tag,
       ///< no client action to change name is performed ).

  int ClientChannelNumber(void) const { return m_iClientChannelNumber; }
       ///< Return the channel number used by the client driver.
  void SetClientNumber(int Number);
       ///< Change the client number for this channel (is changed only in this tag,
       ///< no client action to change name is performed ).

  long ClientID(void) const { return m_iClientId; }
       ///< The client ID this channel belongs to.
  void SetClientID(int ClientId);
       ///< Set the client ID for this channel.

  long ChannelID(void) const { return m_iDatabaseId; }
       ///< Return XBMC own channel ID for this channel which is used in the
       ///< TV Database.
  void SetChannelID(long ChannelID);
       ///< Change the channel ID for this channel (no Action to the Database
       ///< are taken)

  int UniqueID(void) const { return m_iUniqueId; }
       ///< A UniqueID for this channel provider to identify same channels on different clients.
  void SetUniqueID(int id);
       ///< Change the Unique ID for this channel.

  int GroupID(void) const { return m_iChannelGroupId; }
       ///< The Group this channel belongs to, -1 for undefined.
  void SetGroupID(int group);
       ///< Set the group ID for this channel.

  bool IsEncrypted(void) const { return m_iClientEncryptionSystem > 0; }
       ///< Return true if this channel is encrypted. Does not inform if XBMC can play it,
       ///< decryption is done by the client associated backend.
  int EncryptionSystem(void) const { return m_iClientEncryptionSystem; }
       ///< Return the encryption system ID for this channel, 0 for FTA. Is based
       ///< upon: http://www.dvb.org/index.php?id=174.

  CStdString EncryptionName() const;
       ///< Return a human understandable name for the used encryption system.
  void SetEncryptionSystem(int system);
       ///< Set the encryption ID for this channel.

  bool IsRadio(void) const { return m_bIsRadio; }
       ///< Return true if this is a Radio channel.
  void SetRadio(bool radio);
       ///< Set the radio flag.

  bool IsRecording(void) const { return m_bClientIsRecording; }
       ///< True if this channel is currently recording.
  void SetRecording(bool rec);
       ///< Set the recording state.

  CStdString StreamURL(void) const { return m_strStreamURL; }
       ///< The Stream URL to access this channel, it can be all types of protocol and types
       ///< are supported by XBMC or in case the client read the stream leave it empty
  void SetStreamURL(CStdString stream);
       ///< Set the stream URL

  CStdString Path(void) const { return m_strFileNameAndPath; }
       ///< Return the path in the XBMC virtual Filesystem.
  void SetPath(CStdString path);
       ///< Set the path in XBMC Virtual Filesystem.

  CStdString Icon(void) const { return m_strIconPath; }
       ///< Return Path with Filename of the Icon for this channel.
  void SetIcon(CStdString icon);
       ///< Set the path and filename for this channel Icon

  bool IsHidden(void) const { return m_bIsHidden; }
       ///< If this channel is hidden from view it is true.
  void SetHidden(bool hide);
       ///< Mask this channel hidden.

  bool GrabEpg() const { return m_bGrabEpg; }
       ///< If false ignore EPG for this channel.
  void SetGrabEpg(bool grabEpg);
       ///< Change the EPG grabbing flag

  bool IsVirtual() const { return m_bIsVirtual; }
       ///< If true it is a virtual channel
  void SetVirtual(bool virtualChannel);
       ///< Change the virtual flag

  CStdString Grabber(void) const { return m_strGrabber; }
       ///< Get the EPG scraper name
  void SetGrabber(CStdString Grabber);
       ///< Set the EPG scraper name, use "client" for loading  the EPG from Backend

  bool IsPortalMaster() const { return m_iPortalMasterChannel == 0; }
       ///< Return true if this is a Portal Master channel
  bool IsPortalSlave() const { return m_iPortalMasterChannel > 0; }
       ///< Return true if this is a Portal Slave channel
  void SetPortalChannel(long channelID);
       ///< Set the portal channel ID, use -1 for no portal channel, NULL if this is a
       ///< master channel or if it is a slave the master client ID
  void ClearPortalChannels();
       ///< Remove all Portal channels
  void AddPortalChannel(CPVRChannel* channel);
       ///< Add a channel ID to the Portal list
  int GetPortalChannels(CFileItemList* results);
       ///< Returns a File Item list with all portal channels

  /*! \brief Get the input format from the Backend
   If it is empty ffmpeg scanning the stream to find the right input format.
   See "xbmc/cores/dvdplayer/Codecs/ffmpeg/libavformat/allformats.c" for a
   list of the input formats.
   \return The name of the input format
   */
  CStdString InputFormat(void) const { return m_strInputFormat; }

  /*! \brief Set the input format of the Backend this channel belongs to
   \param format The name of the input format
   */
  void SetInputFormat(CStdString format);

  const CPVREpgInfoTag* GetEpgNow() const;
  const CPVREpgInfoTag* GetEpgNext() const;

  /*! \name Linked channel functions, used for portal mode */
  void ClearLinkedChannels();
  void AddLinkedChannel(long LinkedChannel);

  bool ClearEPG();
};
