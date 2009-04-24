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
 * for DESCRIPTION see 'TVChannelInfoTag.cpp'
 */

#include "DateTime.h"
#include "FileItem.h"

class CTVEPGInfoTag;
class CFileItemList;

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


class CTVChannelInfoTag/* : public CVideoInfoTag*/
{
public:
  CTVChannelInfoTag();
  void Reset();

  bool GetEPGNowInfo(CTVEPGInfoTag &result);
  bool GetEPGNextInfo(CTVEPGInfoTag &result);
  bool GetEPGLastEntry(CTVEPGInfoTag &result);
  void CleanupEPG();
  const CFileItemList& GetEPG();

  bool operator ==(const CTVChannelInfoTag &right) const;
  bool operator !=(const CTVChannelInfoTag &right) const;

  int                 m_iIdChannel;           /// Database number
  int                 m_iChannelNum;          /// Channel number for channels on XBMC
  int                 m_iClientNum;           /// Channel number on client
  int                 m_iGroupID;             /// Channel group identfier

  CStdString          m_strChannel;           /// Channel name

  CStdString          m_IconPath;             /// Path to the logo image

  bool                m_encrypted;            /// Encrypted channel
  bool                m_radio;                /// Radio channel
  bool                m_hide;                 /// Channel is hide inside filelists
  bool                m_isRecording;

  CStdString          m_strNextTitle;

  CDateTime           m_startTime;            /// Start time
  CDateTime           m_endTime;              /// End time
  CDateTimeSpan       m_duration;             /// Duration

  CStdString          m_strFileNameAndPath;   /// Filename for PVRManager to open and read stream

  TVChannelSettings   m_Settings;             /// Channel settings must be received manually

  long                m_clientID;

private:
  friend class CEPG;
  CFileItemList       m_EPG;                  /// EPG Data for Channel
};

typedef std::vector<CTVChannelInfoTag> VECCHANNELS;
