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
#include "utils/Thread.h"
#include "utils/PVRChannels.h"
#include "../addons/include/xbmc_pvr_types.h"

class cPVREPGInfoTag;
class cPVRChannelInfoTag;

/* Filter data to check with a EPGEntry */
struct EPGSearchFilter
{
  void SetDefaults();
  bool FilterEntry(const cPVREPGInfoTag &tag) const;

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
  std::vector<cPVREPGInfoTag*> m_tags;
  bool m_bUpdateRunning;
  bool m_bValid;

public:
  cPVREpg(long ChannelID);
  long ChannelID(void) const { return m_channelID; }
  bool IsValid(void) const;
  const cPVRChannelInfoTag *ChannelTag(void) const { return m_Channel; }
  cPVREPGInfoTag *AddInfoTag(cPVREPGInfoTag *Tag);
  void DelInfoTag(cPVREPGInfoTag *tag);
  void Cleanup(CDateTime Time);
  void Cleanup(void);
  void Sort(void);
  const std::vector<cPVREPGInfoTag*> *InfoTags(void) const { return &m_tags; }
  const cPVREPGInfoTag *GetInfoTagNow(void) const;
  const cPVREPGInfoTag *GetInfoTagNext(void) const;
  const cPVREPGInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const cPVREPGInfoTag *GetInfoTagAround(CDateTime Time) const;
  CDateTime GetLastEPGDate();
  bool IsUpdateRunning() const { return m_bUpdateRunning; }
  void SetUpdate(bool OnOff) { m_bUpdateRunning = OnOff; }

  static bool Add(const PVR_PROGINFO *data, cPVREpg *Epg);
  static bool AddDB(const PVR_PROGINFO *data, cPVREpg *Epg);
};


class cPVREPGInfoTag
{
  friend class cPVREpg;
private:
  cPVREpg *m_Epg;     // The Schedule this event belongs to
  const cPVRTimerInfoTag   *m_Timer;

  CStdString    m_strTitle;
  CStdString    m_strPlotOutline;
  CStdString    m_strPlot;
  CStdString    m_strGenre;
  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_duration;
  CStdString    m_IconPath;
  CStdString    m_strFileNameAndPath;
  int           m_GenreType;
  int           m_GenreSubType;
  bool          m_isRecording;
  CDateTime     m_firstAired;
  int           m_parentalRating;
  int           m_starRating;
  bool          m_notify;
  CStdString    m_seriesNum;
  CStdString    m_episodeNum;
  CStdString    m_episodePart;
  CStdString    m_episodeName;

  long          m_uniqueBroadcastID; // event's unique identifier for this tag

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
  CDateTime FirstAired(void) const { return m_firstAired; }
  void SetFirstAired(CDateTime FirstAired) { m_firstAired = FirstAired; }
  int ParentalRating(void) const { return m_parentalRating; }
  void SetParentalRating(int ParentalRating) { m_parentalRating = ParentalRating; }
  int StarRating(void) const { return m_starRating; }
  void SetStarRating(int StarRating) { m_starRating = StarRating; }
  bool Notify(void) const { return m_notify; }
  void SetNotify(bool Notify) { m_notify = Notify; }
  CStdString SeriesNum(void) const { return m_seriesNum; }
  void SetSeriesNum(CStdString SeriesNum) { m_seriesNum = SeriesNum; }
  CStdString EpisodeNum(void) const { return m_episodeNum; }
  void SetEpisodeNum(CStdString EpisodeNum) { m_episodeNum = EpisodeNum; }
  CStdString EpisodePart(void) const { return m_episodePart; }
  void SetEpisodePart(CStdString EpisodePart) { m_episodePart = EpisodePart; }
  CStdString EpisodeName(void) const { return m_episodeName; }
  void SetEpisodeName(CStdString EpisodeName) { m_episodeName = EpisodeName; }
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
  bool ClientID(void) const { return m_Epg->ChannelTag()->ClientID(); }

  /*! \brief Get the cPVRChannelInfoTag class associated to this epg entry
   \return the pointer to the info tag
   */
  const cPVRChannelInfoTag *ChannelTag(void) const { return m_Epg->ChannelTag(); }

  /* Scheduled recording related Data */
  void SetTimer(const cPVRTimerInfoTag *Timer) { m_Timer = Timer; }
  const cPVRTimerInfoTag *Timer(void) const { return m_Timer; }

  CStdString ConvertGenreIdToString(int ID, int subID) const;
};

class cPVREpgs : public std::vector<cPVREpg*>
{
  friend class cPVREpg;

private:
  CCriticalSection m_critSection;
  bool  m_bInihibitUpdate;

public:
  cPVREpgs(void);

  cPVREpg *AddEPG(long ChannelID);
  const cPVREpg *GetEPG(long ChannelID) const;
  const cPVREpg *GetEPG(const cPVRChannelInfoTag *Channel, bool AddIfMissing = false) const;
  void Add(cPVREpg *entry);

  void Cleanup(void);
  bool ClearAll(void);
  bool ClearChannel(long ChannelID);
  void Load();
  void Unload();
  void Update(bool Scan = false);
  void InihibitUpdate(bool yesNo) { m_bInihibitUpdate = yesNo; }
  int GetEPGSearch(CFileItemList* results, const EPGSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  CDateTime GetFirstEPGDate(bool radio = false);
  CDateTime GetLastEPGDate(bool radio = false);
  void SetVariableData(CFileItemList* results);
  void AssignChangedChannelTags(bool radio = false);
};

extern cPVREpgs PVREpgs;
