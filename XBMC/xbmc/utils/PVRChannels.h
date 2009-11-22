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

class cPVRChannelInfoTag : public CVideoInfoTag
{
  friend class cPVREpgs;
  friend class CTVDatabase;

private:
  mutable const cPVREpg *m_Epg;

  int                 m_iIdChannel;           /// Database number
  int                 m_iChannelNum;          /// Channel number for channels on XBMC
  int                 m_iGroupID;             /// Channel group identfier

  CStdString          m_strChannel;           /// Channel name
  CStdString          m_strClientName;

  CStdString          m_IconPath;             /// Path to the logo image

  bool                m_encryptionSystem;     /// Encrypted channel
  bool                m_radio;                /// Radio channel
  bool                m_hide;                 /// Channel is hide inside filelists
  bool                m_isRecording;

  CStdString          m_strNextTitle;

  CDateTime           m_startTime;            /// Start time
  CDateTime           m_endTime;              /// End time
  CDateTimeSpan       m_duration;             /// Duration

  long                m_iIdUnique;            /// Unique Id for this channel
  int                 m_clientID;             /// Id of client channel come from
  int                 m_iClientNum;           /// Channel number on client

  CStdString          m_strStreamURL;         /// URL of the stream, if empty use Client to read stream
  CStdString          m_strFileNameAndPath;   /// Filename for PVRManager to open and read stream

public:
  cPVRChannelInfoTag();
  void Reset();

  bool operator ==(const cPVRChannelInfoTag &right) const;
  bool operator !=(const cPVRChannelInfoTag &right) const;

  CStdString Name(void) const { return m_strChannel; }
  void SetName(CStdString name) { m_strChannel = name; }
  CStdString ClientName(void) const { return m_strClientName; }
  void SetClientName(CStdString name) { m_strClientName = name; }
  int Number(void) const { return m_iChannelNum; }
  void SetNumber(int Number) { m_iChannelNum = Number; }
  int ClientNumber(void) const { return m_iClientNum; }
  void SetClientNumber(int Number) { m_iClientNum = Number; }
  long ClientID(void) const { return m_clientID; }
  void SetClientID(int ClientId) { m_clientID = ClientId; }
  long ChannelID(void) const { return m_iIdChannel; }
  void SetChannelID(int ChannelID) { m_iIdChannel = ChannelID; }
  long UniqueID(void) const { return m_iIdUnique; }
  void SetUniqueID(long id) { m_iIdUnique = id; }
  long GroupID(void) const { return m_iGroupID; }
  void SetGroupID(long group) { m_iGroupID = group; }
  bool IsRadio(void) const { return m_radio; }
  void SetRadio(bool radio) { m_radio = radio; }
  bool IsRecording(void) const { return m_isRecording; }
  void SetRecording(bool rec) { m_isRecording = rec; }
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
  CStdString Stream(void) const { return m_strStreamURL; }
  void SetStream(CStdString stream) { m_strStreamURL = stream; }
  CStdString Path(void) const { return m_strFileNameAndPath; }
  void SetPath(CStdString path) { m_strFileNameAndPath = path; }
  CStdString Icon(void) const { return m_IconPath; }
  void SetIcon(CStdString icon) { m_IconPath = icon; }
  bool IsHidden(void) const { return m_hide; }
  void SetHidden(bool hide) { m_hide = hide; }
  int GetDuration() const;
  int GetTime() const;
  void SetDuration(CDateTimeSpan duration) { m_duration = duration; }
  CDateTime StartTime(void) const { return m_startTime; }
  void SetStartTime(CDateTime time) { m_startTime = time; }
  CDateTime EndTime(void) const { return m_endTime; }
  void SetEndTime(CDateTime time) { m_endTime = time; }
  CStdString NextTitle(void) const { return m_strNextTitle; }
  void SetNextTitle(CStdString title) { m_strNextTitle = title; }
  CStdString Title(void) const { return m_strTitle; }
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
  void Clear();
};

extern cPVRChannels      PVRChannelsTV;
extern cPVRChannels      PVRChannelsRadio;
extern cPVRChannelGroups PVRChannelGroups;
