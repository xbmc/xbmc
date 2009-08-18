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

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "utils/Thread.h"
#include "../addons/include/xbmc_pvr_types.h"

#define EVCONTENTMASK_MOVIEDRAMA               0x10
#define EVCONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EVCONTENTMASK_SHOW                     0x30
#define EVCONTENTMASK_SPORTS                   0x40
#define EVCONTENTMASK_CHILDRENYOUTH            0x50
#define EVCONTENTMASK_MUSICBALLETDANCE         0x60
#define EVCONTENTMASK_ARTSCULTURE              0x70
#define EVCONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EVCONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EVCONTENTMASK_LEISUREHOBBIES           0xA0
#define EVCONTENTMASK_SPECIAL                  0xB0
#define EVCONTENTMASK_USERDEFINED              0xF0

class cPVREpg;
class cPVRChannelInfoTag;

class CTVEPGInfoTag : public CVideoInfoTag
{
  friend class cPVREpg;
private:
  cPVREpg *Epg;     // The Schedule this event belongs to
  const cPVRChannelInfoTag *m_Channel;

public:
  int           m_idChannel;
  CStdString    m_strSource;
  CStdString    m_strBouquet;
  int           m_bouquetNum;
  CStdString    m_strChannel;
  int           m_channelNum;
  CStdString    m_IconPath;

  CStdString    m_strFileNameAndPath;

  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_duration;
  CDateTime     m_firstAired;
  bool          m_repeat;

  bool          m_isRadio;
  bool          m_commFree;
  bool          m_isRecording;
  bool          m_bAutoSwitch;
  int           m_GenreType;
  int           m_GenreSubType;

private:
  long m_uniqueBroadcastID; // db's unique identifier for this tag

public:
  CTVEPGInfoTag(long uniqueBroadcastID);
  CTVEPGInfoTag() { Reset(); };
  void Reset();

  CDateTime Start(void) const { return m_startTime; }
  void SetStart(CDateTime Start) { m_startTime = Start; }
  CDateTime End(void) const { return m_endTime; }
  void SetEnd(CDateTime Stop) { m_endTime = Stop; }
  CStdString Title(void) const { return m_strTitle; }
  void SetTitle(CStdString name) { m_strTitle = name; }
  CStdString PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(CStdString PlotOutline) { m_strPlotOutline = PlotOutline; }
  CStdString Plot(void) const { return m_strPlot; }
  void SetPlot(CStdString Plot) { m_strPlot = Plot; }
  int GenreType(void) const { return m_GenreType; }
  void SetGenreType(int GenreType) { m_GenreType = GenreType; }
  int GenreSubType(void) const { return m_GenreSubType; }
  void SetGenreSubType(int GenreSubType) { m_GenreSubType = GenreSubType; }
  CStdString Genre(void) const { return m_strGenre; }
  void SetGenre(CStdString Genre) { m_strGenre = Genre; }
  CDateTimeSpan Duration(void) const { return m_duration; }
  void SetDuration(CDateTimeSpan duration) { m_duration = duration; }
  long ChannelID(void) const { return m_idChannel; }
  void SetChannelID(int ChannelID) { m_idChannel = ChannelID; }
  void SetChannel(const cPVRChannelInfoTag *Channel) { m_Channel = Channel; }
  bool HasTimer() const;
};


class cPVREpg
{
  friend class cPVREpgs;

private:
  long m_channelID;
  const cPVRChannelInfoTag *m_Channel;
  std::vector<CTVEPGInfoTag> tags;

public:
  cPVREpg(long ChannelID);
  long ChannelID(void) const { return m_channelID; }
  CTVEPGInfoTag *AddInfoTag(CTVEPGInfoTag *Tag);
  void DelInfoTag(CTVEPGInfoTag *tag);
  void Cleanup(CDateTime Time);
  void Cleanup(void);
  const std::vector<CTVEPGInfoTag> *InfoTags(void) const { return &tags; }
  const CTVEPGInfoTag *GetInfoTagNow(void) const;
  const CTVEPGInfoTag *GetInfoTagNext(void) const;
  const CTVEPGInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const CTVEPGInfoTag *GetInfoTagAround(CDateTime Time) const;
  static bool Add(const PVR_PROGINFO *data, cPVREpg *Epg);
};


class cPVREpgsLock 
{
private:
  int m_locked;
  bool m_WriteLock;
public:
  cPVREpgsLock(bool WriteLock = false);
  ~cPVREpgsLock();
  bool Locked(void);
};


class cPVREpgs : public std::vector<cPVREpg>
               , private CThread 
{
  friend class cPVREpg;
  friend class cPVREpgsLock;

private:
  CRITICAL_SECTION m_critSection;
  static DWORD m_lastCleanup;
  static cPVREpgs m_epgs;
  int m_locked;
  virtual void Process();

public:
  static const cPVREpgs *EPGs(cPVREpgsLock &PVREpgsLock);
  static void Cleanup(void);
  static bool ClearAll(void);
  static bool ClearChannel(long ChannelID);
  static bool Load();
  static bool Update(bool Wait = false);
  static int GetEPGAll(CFileItemList* results, bool radio = false);
  static int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  static int GetEPGNow(CFileItemList* results, bool radio = false);
  static int GetEPGNext(CFileItemList* results, bool radio = false);
  cPVREpg *AddEPG(long ChannelID);
  const cPVREpg *GetEPG(long ChannelID) const;
  const cPVREpg *GetEPG(const cPVRChannelInfoTag *Channel, bool AddIfMissing = false) const;
  void Add(cPVREpg *entry);
};
