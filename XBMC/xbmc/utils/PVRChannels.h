#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

/*
 * for DESCRIPTION see 'PVRChannels.cpp'
 */

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"
#include "../addons/include/xbmc_pvr_types.h"

class cPVREpg;

class cPVRChannelInfoTag
{
  friend class cPVREpgs;
  friend class CTVDatabase;

private:
  void UpdateRunningEvents();
  void DisplayError(PVR_ERROR err) const;

  mutable const cPVREpg *m_Epg;
  mutable const cPVREPGInfoTag *m_epgNow;
  mutable const cPVREPGInfoTag *m_epgNext;

  /* XBMC related channel data */
  int                 m_iIdChannel;           ///> Database number
  int                 m_iChannelNum;          ///> Channel number for channels on XBMC
  int                 m_iGroupID;             ///> Channel group identfier
  int                 m_encryptionSystem;     ///> Encryption System, 0 for FreeToAir, -1 unknown
  bool                m_radio;                ///> Radio channel
  bool                m_hide;                 ///> Channel is hide inside filelists
  bool                m_isRecording;          ///> True if channel is currently recording
  CStdString          m_IconPath;             ///> Path to the logo image
  CStdString          m_strChannel;           ///> Channel name

  /* Client related channel data */
  long                m_iIdUnique;            ///> Unique Id for this channel
  int                 m_clientID;             ///> Id of client channel come from
  int                 m_iClientNum;           ///> Channel number on client
  CStdString          m_strClientName;        ///> Channel name on client

  CStdString          m_strStreamURL;         ///> URL of the stream, if empty use Client to read stream
  CStdString          m_strFileNameAndPath;   ///> Filename for PVRManager to open and read stream

public:
  cPVRChannelInfoTag() { Reset(); };
  void Reset();
       ///< Set the tag to it's initial values.
  bool IsEmpty() const;
       ///< True if no required data is present inside the tag.

  bool operator ==(const cPVRChannelInfoTag &right) const;
  bool operator !=(const cPVRChannelInfoTag &right) const;

  /* Channel information */
  CStdString Name(void) const { return m_strChannel; }
       ///< Return the currently by XBMC used name for this channel.
  void SetName(CStdString name) { m_strChannel = name; }
       ///< Set the name, XBMC uses for this channel.
  int Number(void) const { return m_iChannelNum; }
       ///< Return the currently by XBMC used channel number for this channel.
  void SetNumber(int Number) { m_iChannelNum = Number; }
       ///< Change the XBMC number for this channel.
  CStdString ClientName(void) const { return m_strClientName; }
       ///< Return the name used by the client driver on the backend.
  void SetClientName(CStdString name) { m_strClientName = name; }
       ///< Set the name used by the client (is changed only in this tag,
       ///< no client action to change name is performed ).
  int ClientNumber(void) const { return m_iClientNum; }
       ///< Return the channel number used by the client driver.
  void SetClientNumber(int Number) { m_iClientNum = Number; }
       ///< Change the client number for this channel (is changed only in this tag,
       ///< no client action to change name is performed ).
  long ClientID(void) const { return m_clientID; }
       ///< The client ID this channel belongs to.
  void SetClientID(int ClientId) { m_clientID = ClientId; }
       ///< Set the client ID for this channel.
  long ChannelID(void) const { return m_iIdChannel; }
       ///< Return XBMC own channel ID for this channel which is used in the
       ///< TV Database.
  void SetChannelID(int ChannelID) { m_iIdChannel = ChannelID; }
       ///< Change the channel ID for this channel (no Action to the Database
       ///< are taken)
  long UniqueID(void) const { return m_iIdUnique; }
       ///< A UniqueID for this channel provider to identify same channels on different clients.
  void SetUniqueID(long id) { m_iIdUnique = id; }
       ///< Change the Unique ID for this channel.
  long GroupID(void) const { return m_iGroupID; }
       ///< The Group this channel belongs to, -1 for undefined.
  void SetGroupID(long group) { m_iGroupID = group; }
       ///< Set the group ID for this channel.
  bool IsEncrypted(void) const { return m_encryptionSystem > 0; }
       ///< Return true if this channel is encrypted. Does not inform if XBMC can play it,
       ///< decryption is done by the client associated backend.
  int EncryptionSystem(void) const { return m_encryptionSystem; }
       ///< Return the encryption system ID for this channel, 0 for FTA. Is based
       ///< upon: http://www.dvb.org/index.php?id=174.
  CStdString EncryptionName() const;
       ///< Return a human understandable name for the used encryption system.
  void SetEncryptionSystem(int system) { m_encryptionSystem = system; }
       ///< Set the encryption ID for this channel.
  bool IsRadio(void) const { return m_radio; }
       ///< Return true if this is a Radio channel.
  void SetRadio(bool radio) { m_radio = radio; }
       ///< Set the radio flag.
  bool IsRecording(void) const { return m_isRecording; }
       ///< True if this channel is currently recording.
  void SetRecording(bool rec) { m_isRecording = rec; }
       ///< Set the recording state.
  CStdString StreamURL(void) const { return m_strStreamURL; }
       ///< The Stream URL to access this channel, it can be all types of protocol and types
       ///< are supported by XBMC or in case the client read the stream leave it empty
  void SetStreamURL(CStdString stream) { m_strStreamURL = stream; }
       ///< Set the stream URL
  CStdString Path(void) const { return m_strFileNameAndPath; }
       ///< Return the path in the XBMC virtual Filesystem.
  void SetPath(CStdString path) { m_strFileNameAndPath = path; }
       ///< Set the path in XBMC Virtual Filesystem.
  CStdString Icon(void) const { return m_IconPath; }
       ///< Return Path with Filename of the Icon for this channel.
  void SetIcon(CStdString icon) { m_IconPath = icon; }
       ///< Set the path and filename for this channel Icon
  bool IsHidden(void) const { return m_hide; }
       ///< If this channel is hidden from view it is true.
  void SetHidden(bool hide) { m_hide = hide; }
       ///< Mask this channel hidden.

  /* EPG information for now playing event */
  CStdString NowTitle() const;
  CStdString NowPlotOutline() const;
  CStdString NowPlot() const;
  CDateTime  NowStartTime(void) const;
  CDateTime  NowEndTime(void) const;
  int        NowDuration() const;
  int        NowPlayTime() const;
  CStdString NowGenre(void) const;

  /* EPG information for next playing event */
  CStdString NextTitle() const;
  CStdString NextPlotOutline() const;
  CStdString NextPlot() const;
  CDateTime  NextStartTime(void) const;
  CDateTime  NextEndTime(void) const;
  int        NextDuration() const;
  CStdString NextGenre(void) const;
};

class cPVRChannels : public std::vector<cPVRChannelInfoTag>
{
private:
  bool m_bRadio;
  int m_iHiddenChannels;

public:
  cPVRChannels(void);
  bool Load(bool radio);
  bool Update();
  void ReNumberAndCheck(void);
  void SearchAndSetChannelIcons(bool writeDB = false);
  int GetNumChannels() const { return size(); }
  int GetNumHiddenChannels() const { return m_iHiddenChannels; }
  int GetChannels(CFileItemList* results, int group_id = -1);
  int GetHiddenChannels(CFileItemList* results);
  void MoveChannel(unsigned int oldindex, unsigned int newindex);
  void HideChannel(unsigned int number);
  cPVRChannelInfoTag *GetByNumber(int Number);
  cPVRChannelInfoTag *GetByClient(int Number, int ClientID);
  cPVRChannelInfoTag *GetByChannelID(long ChannelID);
  cPVRChannelInfoTag *GetByUniqueID(long UniqueID);
  CStdString GetNameForChannel(unsigned int Number);
  CStdString GetChannelIcon(unsigned int Number);
  void SetChannelIcon(unsigned int Number, CStdString Icon);
  void Clear();

  static int GetNumChannelsFromAll();
  static void SearchMissingChannelIcons();
  static cPVRChannelInfoTag *GetByClientFromAll(int Number, int ClientID);
  static cPVRChannelInfoTag *GetByChannelIDFromAll(long ChannelID);
  static cPVRChannelInfoTag *GetByUniqueIDFromAll(long UniqueID);
  static cPVRChannelInfoTag *GetByPath(CStdString &path);
  static bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};

class cPVRChannelGroup
{
private:
  unsigned long m_iGroupID;
  CStdString    m_GroupName;

public:
  cPVRChannelGroup(void);

  long GroupID(void) const { return m_iGroupID; }
  void SetGroupID(long group) { m_iGroupID = group; }
  CStdString GroupName(void) const { return m_GroupName; }
  void SetGroupName(CStdString name) { m_GroupName = name; }
};

class cPVRChannelGroups : public std::vector<cPVRChannelGroup>
{
public:
  cPVRChannelGroups(void);

  bool Load();

  int GetGroupList(CFileItemList* results);
  int GetFirstChannelForGroupID(int GroupId, bool radio = false);
  int GetPrevGroupID(int current_group_id);
  int GetNextGroupID(int current_group_id);

  void AddGroup(const CStdString &name);
  bool RenameGroup(unsigned int GroupId, const CStdString &newname);
  bool DeleteGroup(unsigned int GroupId);
  bool ChannelToGroup(const cPVRChannelInfoTag &channel, unsigned int GroupId);
  CStdString GetGroupName(int GroupId);
  int GetGroupId(CStdString GroupName);
  void Clear();
};

extern cPVRChannels      PVRChannelsTV;
extern cPVRChannels      PVRChannelsRadio;
extern cPVRChannelGroups PVRChannelGroups;
