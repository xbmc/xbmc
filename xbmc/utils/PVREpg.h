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
#include "utils/PVRChannels.h"
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

class cPVREPGInfoTag;
class cPVRChannelInfoTag;

/* Filter data to check with a EPGEntry */
struct EPGSearchFilter
{
  void SetDefaults();

  CStdString    m_SearchString;
  bool          m_CaseSensitive;
  bool          m_SearchDescription;
  int           m_GenreType;
  int           m_GenreSubType;
  int           m_minDuration;
  int           m_maxDuration;
  SYSTEMTIME    m_startTime;
  SYSTEMTIME    m_endTime;
  SYSTEMTIME    m_startDate;
  SYSTEMTIME    m_endDate;
  int           m_ChannelNumber;
  bool          m_FTAOnly;
  bool          m_IncUnknGenres;
  int           m_Group;
  bool          m_IgnPresentTimers;
  bool          m_IgnPresentRecords;
  bool          m_PreventRepeats;
};


class cPVREpg
{
  friend class cPVREpgs;

private:
  long m_channelID;
  const cPVRChannelInfoTag *m_Channel;
  int m_iTimeCorrection;
  std::vector<cPVREPGInfoTag> tags;

public:
  cPVREpg(long ChannelID);
  long ChannelID(void) const { return m_channelID; }
  const cPVRChannelInfoTag *ChannelTag(void) const { return m_Channel; }
  cPVREPGInfoTag *AddInfoTag(cPVREPGInfoTag *Tag);
  void DelInfoTag(cPVREPGInfoTag *tag);
  void Cleanup(CDateTime Time);
  void Cleanup(void);
  const std::vector<cPVREPGInfoTag> *InfoTags(void) const { return &tags; }
  const cPVREPGInfoTag *GetInfoTagNow(void) const;
  const cPVREPGInfoTag *GetInfoTagNext(void) const;
  const cPVREPGInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const cPVREPGInfoTag *GetInfoTagAround(CDateTime Time) const;
  static bool Add(const PVR_PROGINFO *data, cPVREpg *Epg);
};


class cPVREPGInfoTag : public CVideoInfoTag
{
  friend class cPVREpg;
private:
  cPVREpg *m_Epg;     // The Schedule this event belongs to
  const cPVRTimerInfoTag   *m_Timer;

  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_duration;
  CStdString    m_IconPath;
  CStdString    m_strFileNameAndPath;
  int           m_GenreType;
  int           m_GenreSubType;
  bool          m_isRecording;

  long          m_uniqueBroadcastID; // db's unique identifier for this tag

public:
  cPVREPGInfoTag(long uniqueBroadcastID);
  cPVREPGInfoTag() { Reset(); };
  void Reset();

  long GetUniqueBroadcastID(void) const { return m_uniqueBroadcastID; }
  CDateTime Start(void) const { return m_startTime; }
  void SetStart(CDateTime Start) { m_startTime = Start; }
  CDateTime End(void) const { return m_endTime; }
  void SetEnd(CDateTime Stop) { m_endTime = Stop; }
  int GetDuration() const;
  CStdString Title(void) const { return m_strTitle; }
  void SetTitle(CStdString name) { m_strTitle = name; }
  CStdString PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(CStdString PlotOutline) { m_strPlotOutline = PlotOutline; }
  CStdString Plot(void) const { return m_strPlot; }
  void SetPlot(CStdString Plot) { m_strPlot = Plot; }
  int GenreType(void) const { return m_GenreType; }
  int GenreSubType(void) const { return m_GenreSubType; }
  CStdString Genre(void) const { return m_strGenre; }
  void SetGenre(int ID, int subID);
  CStdString Icon(void) const { return m_IconPath; }
  void SetIcon(CStdString icon) { m_IconPath = icon; }
  CStdString Path(void) const { return m_strFileNameAndPath; }
  void SetPath(CStdString Path) { m_strFileNameAndPath = Path; }
  bool HasTimer() const;

  /* Channel related Data */
  long ChannelID(void) const { return m_Epg->ChannelTag()->ChannelID(); }
  long ChannelNumber(void) const { return m_Epg->ChannelTag()->Number(); }
  CStdString ChannelName(void) const { return m_Epg->ChannelTag()->Name(); }
  CStdString ChanneIcon(void) const { return m_Epg->ChannelTag()->Icon(); }
  long GroupID(void) const { return m_Epg->ChannelTag()->GroupID(); }
  bool IsEncrypted(void) const { return m_Epg->ChannelTag()->IsEncrypted(); }
  bool IsRadio(void) const { return m_Epg->ChannelTag()->IsRadio(); }

  /* Scheduled recording related Data */
  void SetTimer(const cPVRTimerInfoTag *Timer) { m_Timer = Timer; }
  const cPVRTimerInfoTag *Timer(void) const { return m_Timer; }

  CStdString ConvertGenreIdToString(int ID, int subID) const;
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
  static unsigned int m_lastCleanup;
  static cPVREpgs m_epgs;
  int m_locked;
  virtual void Process();
  static bool FilterEntry(const cPVREPGInfoTag &tag, const EPGSearchFilter &filter);

public:
  static const cPVREpgs *EPGs(cPVREpgsLock &PVREpgsLock);
  static void Cleanup(void);
  static bool ClearAll(void);
  static bool ClearChannel(long ChannelID);
  static void Load();
  static void Update(bool Wait = false);
  static int GetEPGSearch(CFileItemList* results, const EPGSearchFilter &filter);
  static int GetEPGAll(CFileItemList* results, bool radio = false);
  static int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  static int GetEPGNow(CFileItemList* results, bool radio = false);
  static int GetEPGNext(CFileItemList* results, bool radio = false);
  static CDateTime GetFirstEPGDate(bool radio = false);
  static CDateTime GetLastEPGDate(bool radio = false);
  cPVREpg *AddEPG(long ChannelID);
  const cPVREpg *GetEPG(long ChannelID) const;
  const cPVREpg *GetEPG(const cPVRChannelInfoTag *Channel, bool AddIfMissing = false) const;
  static void SetVariableData(CFileItemList* results);
  static void AssignChangedChannelTags(bool radio = false);
  void Add(cPVREpg *entry);
};
