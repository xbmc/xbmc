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

class CPVREpgInfoTag;
class CPVRChannel;
struct PVREpgSearchFilter;

class CPVREpg
{
  friend class CPVREpgs;

private:
  long m_channelID;
  const CPVRChannel *m_Channel;
  std::vector<CPVREpgInfoTag*> m_tags;
  bool m_bUpdateRunning;
  bool m_bValid;

public:
  CPVREpg(long ChannelID);
  long ChannelID(void) const { return m_channelID; }
  bool IsValid(void) const;
  const CPVRChannel *ChannelTag(void) const { return m_Channel; }
  CPVREpgInfoTag *AddInfoTag(CPVREpgInfoTag *Tag);
  void DelInfoTag(CPVREpgInfoTag *tag);
  void Cleanup(CDateTime Time);
  void Cleanup(void);
  void Sort(void);
  const std::vector<CPVREpgInfoTag*> *InfoTags(void) const { return &m_tags; }
  const CPVREpgInfoTag *GetInfoTagNow(void) const;
  const CPVREpgInfoTag *GetInfoTagNext(void) const;
  const CPVREpgInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const CPVREpgInfoTag *GetInfoTagAround(CDateTime Time) const;
  CDateTime GetLastEPGDate();
  bool IsUpdateRunning() const { return m_bUpdateRunning; }
  void SetUpdate(bool OnOff) { m_bUpdateRunning = OnOff; }

  static bool Add(const PVR_PROGINFO *data, CPVREpg *Epg);
  static bool AddDB(const PVR_PROGINFO *data, CPVREpg *Epg);
};


class CPVREpgInfoTag
{
  friend class CPVREpg;
private:
  CPVREpg *m_Epg;     // The Schedule this event belongs to
  const CPVRTimerInfoTag   *m_Timer;

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
  CPVREpgInfoTag(long uniqueBroadcastID);
  CPVREpgInfoTag() { Reset(); };
  virtual ~CPVREpgInfoTag() {};
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
  int ChannelNumber(void) const { return m_Epg->ChannelTag()->ChannelNumber(); }
  CStdString ChannelName(void) const { return m_Epg->ChannelTag()->ChannelName(); }
  CStdString ChanneIcon(void) const { return m_Epg->ChannelTag()->Icon(); }
  long GroupID(void) const { return m_Epg->ChannelTag()->GroupID(); }
  bool IsEncrypted(void) const { return m_Epg->ChannelTag()->IsEncrypted(); }
  bool IsRadio(void) const { return m_Epg->ChannelTag()->IsRadio(); }
  bool ClientID(void) const { return m_Epg->ChannelTag()->ClientID(); }

  /*! \brief Get the CPVRChannel class associated to this epg entry
   \return the pointer to the info tag
   */
  const CPVRChannel *ChannelTag(void) const { return m_Epg->ChannelTag(); }

  /* Scheduled recording related Data */
  void SetTimer(const CPVRTimerInfoTag *Timer) { m_Timer = Timer; }
  const CPVRTimerInfoTag *Timer(void) const { return m_Timer; }

  CStdString ConvertGenreIdToString(int ID, int subID) const;
};

class CPVREpgs : public std::vector<CPVREpg*>
{
  friend class CPVREpg;

private:
  CCriticalSection m_critSection;
  bool  m_bInihibitUpdate;

public:
  CPVREpgs(void);

  CPVREpg *AddEPG(long ChannelID);
  const CPVREpg *GetEPG(long ChannelID) const;
  const CPVREpg *GetEPG(const CPVRChannel *Channel, bool AddIfMissing = false) const;
  void Add(CPVREpg *entry);

  void Cleanup(void);
  bool ClearAll(void);
  bool ClearChannel(long ChannelID);
  void Load();
  void Unload();
  void Update(bool Scan = false);
  void InihibitUpdate(bool yesNo) { m_bInihibitUpdate = yesNo; }
  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  CDateTime GetFirstEPGDate(bool radio = false);
  CDateTime GetLastEPGDate(bool radio = false);
  void SetVariableData(CFileItemList* results);
  void AssignChangedChannelTags(bool radio = false);
};

extern CPVREpgs PVREpgs;
