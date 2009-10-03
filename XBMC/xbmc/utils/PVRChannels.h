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

enum chSrcType
{
  srcNone,
  src_DVBC,
  src_DVBT,
  src_DVBH,
  src_DVBS,
  src_DVBS2,
  srcAnalog
};

enum chInvValues
{
  InvOff,
  InvOn,
  InvAuto
};

enum chCoderate
{
  Coderate_None,
  Coderate_1_2,
  Coderate_2_3,
  Coderate_3_4,
  Coderate_4_5,
  Coderate_5_6,
  Coderate_6_7,
  Coderate_7_8,
  Coderate_8_9,
  Coderate_9_10,
  Coderate_Auto
};

enum chModTypes
{
  modNone = 0,
  modQAM4 = 1,
  modQAM16 = 2,
  modQAM32 = 3,
  modQAM64 = 4,
  modQAM128 = 5,
  modQAM256 = 6,
  modQAM512 = 7,
  modQAM1024 = 8,
  modQAMAuto = 9,
  modBPSK = 10,
  modQPSK = 11,
  modOQPSK = 12,
  mod8PSK = 13,
  mod16APSK = 14,
  mod32APSK = 15,
  modOFDM = 16,
  modCOFDM = 17,
  modVSB8 = 18,
  modVSB16 = 19
};

enum chPolTypes
{
  pol_H,
  pol_V,
  pol_L,
  pol_R
};

enum chBandwidth
{
  bw_5MHz,
  bw_6MHz,
  bw_7MHz,
  bw_8MHz,
  bw_Auto
};

enum chAlpha
{
  alpha_0,
  alpha_1,
  alpha_2,
  alpha_4
};

enum chGuard
{
  guard_1_4,
  guard_1_8,
  guard_1_16,
  guard_1_32,
  guard_Auto
};

enum chTransm
{
  transmission_2K,
  transmission_4K,
  transmission_8K,
  transmission_Auto
};

enum chRolloff
{
  rolloff_Unknown,
  rolloff_20,
  rolloff_25,
  rolloff_35
};

typedef struct
{
  CStdString  m_strProvider;          /// Provider name
  CStdString  m_satellite;            /// Satellite name
  chSrcType   m_SourceType;           /// Source type
  chCoderate  m_CoderateH;            ///
  chCoderate  m_CoderateL;            ///
  chInvValues m_Inversion;            /// DVB-C/S Inversion values
  chModTypes  m_Modulation;           ///
  chPolTypes  m_Polarization;
  chBandwidth m_Bandwidth;
  chAlpha     m_Alpha;
  chGuard     m_Guard;
  chTransm    m_Transmission;
  chRolloff   m_Rolloff;
  bool        m_Priority;
  bool        m_Hierarchie;
  int         m_Freq;                 /// Channel frequency
  int         m_Symbolrate;           /// Channel symbolrate
  int         m_VPID;                 /// Video program Id
  int         m_APID1;                /// First analog audio Id
  int         m_APID2;                /// Second analog audio Id
  int         m_DPID1;                /// First digital auido Id
  int         m_DPID2;                /// Second digital audio Id
  int         m_CAID;                 /// Conditional access Id (!= 0 = encrypted channel)
  int         m_TPID;                 /// Teletext Id
  int         m_SID;                  /// Service Id
  int         m_NID;                  /// Network Id
  int         m_TID;                  /// Trandport Id
  int         m_RID;                  /// Radio Id

  CStdString  m_parameter;            /// Individual parameter string

} TVChannelSettings;

typedef struct
{
  unsigned long m_ID;
  CStdString    m_Title;

} TVGroupData;

typedef std::vector<TVGroupData> CHANNELGROUPS_DATA;

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

  bool                m_encrypted;            /// Encrypted channel
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
  bool IsEncrypted(void) const { return m_encrypted; }
  void SetEncrypted(bool encrypted) { m_encrypted = encrypted; }
  bool IsRadio(void) const { return m_radio; }
  void SetRadio(bool radio) { m_radio = radio; }
  bool IsRecording(void) const { return m_isRecording; }
  void SetRecording(bool rec) { m_isRecording = rec; }
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
};

typedef std::vector<cPVRChannelInfoTag> VECCHANNELS;

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
  static cPVRChannelInfoTag *GetByClientFromAll(int Number, int ClientID);
  static cPVRChannelInfoTag *GetByChannelIDFromAll(long ChannelID);
  static cPVRChannelInfoTag *GetByUniqueIDFromAll(long UniqueID);
};

extern cPVRChannels PVRChannelsTV;
extern cPVRChannels PVRChannelsRadio;
