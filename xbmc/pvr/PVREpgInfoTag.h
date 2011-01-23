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
#include "PVREpg.h"

class CPVRTimerInfoTag;

class CPVREpgInfoTag
{
  friend class CPVREpg;

private:
  CPVREpg *                     m_Epg;     // The Schedule this event belongs to
  const CPVRTimerInfoTag *      m_Timer;

  int                           m_iBroadcastId;
  CStdString                    m_strTitle;
  CStdString                    m_strPlotOutline;
  CStdString                    m_strPlot;
  CStdString                    m_strGenre;
  CDateTime                     m_startTime;
  CDateTime                     m_endTime;
  CDateTimeSpan                 m_duration;
  CStdString                    m_strIconPath; // XXX not persisted?
  CStdString                    m_strFileNameAndPath;
  int                           m_iGenreType;
  int                           m_iGenreSubType;
  bool                          m_isRecording; //XXX
  CDateTime                     m_strFirstAired;
  int                           m_iParentalRating;
  int                           m_iStarRating;
  bool                          m_notify;
  CStdString                    m_seriesNum;
  CStdString                    m_episodeNum;
  CStdString                    m_episodePart;
  CStdString                    m_episodeName;

  mutable const CPVREpgInfoTag *m_nextEvent;
  mutable const CPVREpgInfoTag *m_previousEvent;
  int                           m_iUniqueBroadcastID; // event's unique identifier for this tag

public:
  CPVREpgInfoTag(int iUniqueBroadcastId);
  CPVREpgInfoTag() { Reset(); };
  ~CPVREpgInfoTag();
  void Reset();

  void SetUniqueBroadcastID(int iUniqueBroadcastID) { m_iUniqueBroadcastID = iUniqueBroadcastID; }
  int UniqueBroadcastID(void) const { return m_iUniqueBroadcastID; }

  int BroadcastId(void) const { return m_iBroadcastId; }
  void SetBroadcastId(int iId) { m_iBroadcastId = iId; }

  CDateTime Start(void) const { return m_startTime; }
  void SetStart(CDateTime Start);

  CDateTime End(void) const { return m_endTime; }
  void SetEnd(CDateTime Stop) { m_endTime = Stop; }

  int GetDuration() const;

  CStdString Title(void) const { return m_strTitle; }
  void SetTitle(CStdString name) { m_strTitle = name; }

  CStdString PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(CStdString PlotOutline) { m_strPlotOutline = PlotOutline; }

  CStdString Plot(void) const { return m_strPlot; }
  void SetPlot(CStdString Plot) { m_strPlot = Plot; }

  int GenreType(void) const { return m_iGenreType; }
  int GenreSubType(void) const { return m_iGenreSubType; }
  CStdString Genre(void) const { return m_strGenre; }
  void SetGenre(int ID, int subID);

  CDateTime FirstAired(void) const { return m_strFirstAired; }
  void SetFirstAired(CDateTime FirstAired) { m_strFirstAired = FirstAired; }

  int ParentalRating(void) const { return m_iParentalRating; }
  void SetParentalRating(int ParentalRating) { m_iParentalRating = ParentalRating; }

  int StarRating(void) const { return m_iStarRating; }
  void SetStarRating(int StarRating) { m_iStarRating = StarRating; }

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

  CStdString Icon(void) const { return m_strIconPath; }
  void SetIcon(CStdString icon) { m_strIconPath = icon; }

  CStdString Path(void) const { return m_strFileNameAndPath; }
  void SetPath(CStdString Path) { m_strFileNameAndPath = Path; }
  void UpdatePath();

  /*! \brief Get the CPVRChannel class associated to this epg entry
   \return the pointer to the info tag
   */
  const CPVRChannel *ChannelTag(void) const { return m_Epg->Channel(); }

  /* Scheduled recording related Data */
  bool HasTimer() const;
  void SetTimer(const CPVRTimerInfoTag *Timer);
  const CPVRTimerInfoTag *Timer(void) const { return m_Timer; }

  CStdString ConvertGenreIdToString(int ID, int subID) const;

  const CPVREpgInfoTag *GetNextEvent() const;
  const CPVREpgInfoTag *GetPreviousEvent() const;

  void SetNextEvent(const CPVREpgInfoTag *event) { m_nextEvent = event; }
  void SetPreviousEvent(const CPVREpgInfoTag *event) { m_previousEvent = event; }

  void Update(const CPVREpgInfoTag &tag);
  void Update(const PVR_PROGINFO *data);
};
