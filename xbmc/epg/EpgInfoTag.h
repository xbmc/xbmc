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
#include "Epg.h"

class CEpg;
class CPVREpg;
class CPVREpgInfoTag;

/** an EPG info tag */

class CEpgInfoTag
{
  friend class CEpg;
  friend class CEpgDatabase;

  friend class CPVREpg;
  friend class CPVREpgInfoTag;

private:
  const CEpg *               m_Epg;                /*!< the schedule this event belongs to */

  int                        m_iBroadcastId;       /*!< database ID */
  CStdString                 m_strTitle;           /*!< title */
  CStdString                 m_strPlotOutline;     /*!< plot outline */
  CStdString                 m_strPlot;            /*!< plot */
  CStdString                 m_strGenre;           /*!< genre */
  CDateTime                  m_startTime;          /*!< event start time */
  CDateTime                  m_endTime;            /*!< event end time */
  CStdString                 m_strIconPath;        /*!< the path to the icon */
  CStdString                 m_strFileNameAndPath; /*!< the filename and path */
  int                        m_iGenreType;         /*!< genre type */
  int                        m_iGenreSubType;      /*!< genre subtype */
  CDateTime                  m_firstAired;         /*!< first airdate */
  int                        m_iParentalRating;    /*!< parental rating */
  int                        m_iStarRating;        /*!< star rating */
  bool                       m_bNotify;            /*!< notify on start */
  int                        m_iSeriesNumber;      /*!< series number */
  int                        m_iEpisodeNumber;     /*!< episode number */
  int                        m_iEpisodePart;       /*!< episode part number */
  CStdString                 m_strEpisodeName;     /*!< episode name */

  mutable const CEpgInfoTag *m_nextEvent;          /*!< the event that will occur after this one */
  mutable const CEpgInfoTag *m_previousEvent;      /*!< the event that occurred before this one */
  int                        m_iUniqueBroadcastID; /*!< unique broadcast ID */

  bool                       m_bChanged;           /*!< keep track of changes to this entry */

  /*!
   * @brief Convert a genre id and subid to a human readable name.
   * @param iID The genre ID.
   * @param iSubID The genre sub ID.
   * @return A human readable name.
   */
  const CStdString &ConvertGenreIdToString(int iID, int iSubID) const;


  /*!
   * @brief Hook that is called when the start date changed.
   */
  virtual void UpdatePath() {}

  /*!
   * @brief Change the pointer to the next event.
   * @param event The next event.
   */
  void SetNextEvent(const CEpgInfoTag *event) { m_nextEvent = event; }

  /*!
   * @brief Change the pointer to the previous event.
   * @param event The previous event.
   */
  void SetPreviousEvent(const CEpgInfoTag *event) { m_previousEvent = event; }

public:
  /*!
   * @brief Create a new EPG event.
   * @param iUniqueBroadcastId The unique broadcast ID for this event.
   */
  CEpgInfoTag(int iUniqueBroadcastId);

  /*!
   * @brief Create a new empty event without a unique ID.
   */
  CEpgInfoTag() { Reset(); };

  /*!
   * @brief Destroy this instance.
   */
  virtual ~CEpgInfoTag();

  bool operator ==(const CEpgInfoTag& right) const;
  bool operator !=(const CEpgInfoTag& right) const;

  /*!
   * @brief Check whether this tag has changed and unsaved values.
   * @return True if it has unsaved values, false otherwise.
   */
  bool Changed(void) const { return m_bChanged; }

  /*!
   * @brief Clear this event.
   */
  virtual void Reset();

  /*!
   * @brief The table this event belongs to
   * @return The table this event belongs to
   */
  virtual const CEpg *GetTable() const { return m_Epg; }

  /*!
   * @brief Change the unique broadcast ID of this event.
   * @param iUniqueBroadcastId The new unique broadcast ID.
   */
  void SetUniqueBroadcastID(int iUniqueBroadcastID);

  /*!
   * @brief Get the unique broadcast ID.
   * @return The unique broadcast ID.
   */
  int UniqueBroadcastID(void) const { return m_iUniqueBroadcastID; }

  /*!
   * @brief Get the event's database ID.
   * @return The database ID.
   */
  int BroadcastId(void) const { return m_iBroadcastId; }

  /*!
   * @brief Change the event's database ID.
   * @param iId The new database ID.
   */
  void SetBroadcastId(int iId);

  /*!
   * @brief Get the event's start time.
   * @return The new start time.
   */
  const CDateTime &StartAsUTC(void) const { return m_startTime; }
  const CDateTime &StartAsLocalTime(void) const;

  /*!
   * @brief Change the event's start time.
   * @param start The new start time.
   */
  void SetStartFromUTC(const CDateTime &start);
  void SetStartFromLocalTime(const CDateTime &start);

  /*!
   * @brief Get the event's end time.
   * @return The new start time.
   */
  const CDateTime &EndAsUTC(void) const { return m_endTime; }
  const CDateTime &EndAsLocalTime(void) const;

  /*!
   * @brief Change the event's end time.
   * @param end The new end time.
   */
  void SetEndFromUTC(const CDateTime &end);
  void SetEndFromLocalTime(const CDateTime &end);

  /*!
   * @brief Get the duration of this event in seconds.
   * @return The duration in seconds.
   */
  int GetDuration() const;

  /*!
   * @brief Get the title of this event.
   * @return The title.
   */
  const CStdString &Title(void) const { return (m_strTitle.IsEmpty()) ? g_localizeStrings.Get(19055) : m_strTitle; }

  /*!
   * @brief Change the title of this event.
   * @param strTitle The new title.
   */
  void SetTitle(const CStdString &strTitle);

  /*!
   * @brief Get the plot outline of this event.
   * @return The plot outline.
   */
  const CStdString &PlotOutline(void) const { return m_strPlotOutline; }

  /*!
   * @brief Change the plot outline of this event.
   * @param strPlotOutline The new plot outline.
   */
  void SetPlotOutline(const CStdString &strPlotOutline);

  /*!
   * @brief Get the plot of this event.
   * @return The plot.
   */
  const CStdString &Plot(void) const { return m_strPlot; }

  /*!
   * @brief Change the plot of this event.
   * @param strPlot The new plot.
   */
  void SetPlot(const CStdString &strPlot);

  /*!
   * @brief Get the genre type ID of this event.
   * @return The genre type ID.
   */
  int GenreType(void) const { return m_iGenreType; }

  /*!
   * @brief Get the genre subtype ID of this event.
   * @return The genre subtype ID.
   */
  int GenreSubType(void) const { return m_iGenreSubType; }

  /*!
   * @brief Get the genre as human readable string.
   * @return The genre.
   */
  const CStdString &Genre(void) const { return m_strGenre; }

  /*!
   * @brief Change the genre of this event.
   * @param iID The genre type ID.
   * @param iSubID The genre subtype ID.
   */
  void SetGenre(int iID, int iSubID);

  /*!
   * @brief Get the first air date of this event.
   * @return The first air date.
   */
  const CDateTime &FirstAiredAsUTC(void) const { return m_firstAired; }
  const CDateTime &FirstAiredAsLocalTime(void) const;

  /*!
   * @brief Change the first air date of this event.
   * @param firstAired The new first air date.
   */
  void SetFirstAiredFromUTC(const CDateTime &firstAired);
  void SetFirstAiredFromLocalTime(const CDateTime &firstAired);

  /*!
   * @brief Get the parental rating of this event.
   * @return The parental rating.
   */
  int ParentalRating(void) const { return m_iParentalRating; }

  /*!
   * @brief Change the parental rating of this event.
   * @param iParentalRating The new parental rating.
   */
  void SetParentalRating(int iParentalRating);

  /*!
   * @brief Get the star rating of this event.
   * @return The star rating.
   */
  int StarRating(void) const { return m_iStarRating; }

  /*!
   * @brief Change the star rating of this event.
   * @param iStarRating The new star rating.
   */
  void SetStarRating(int iStarRating);

  /*!
   * @brief Notify on start if true.
   * @return Notify on start.
   */
  bool Notify(void) const { return m_bNotify; }

  /*!
   * @brief Change the value of notify on start.
   * @param bNotify The new value.
   */
  void SetNotify(bool bNotify);

  /*!
   * @brief The series number of this event.
   * @return The series number.
   */
  int SeriesNum(void) const { return m_iSeriesNumber; }

  /*!
   * @brief Change the series number of this event.
   * @param strSeriesNum The new series number.
   */
  void SetSeriesNum(int iSeriesNum);

  /*!
   * @brief The episode number of this event.
   * @return The episode number.
   */
  int EpisodeNum(void) const { return m_iEpisodeNumber; }

  /*!
   * @brief Change the episode number of this event.
   * @param strEpisodeNum The new episode number.
   */
  void SetEpisodeNum(int iEpisodeNum);

  /*!
   * @brief The episode part number of this event.
   * @return The episode part number.
   */
  int EpisodePart(void) const { return m_iEpisodePart; }

  /*!
   * @brief Change the episode part number of this event.
   * @param strEpisodePart The new episode part number.
   */
  void SetEpisodePart(int iEpisodePart);

  /*!
   * @brief The episode name of this event.
   * @return The episode name.
   */
  const CStdString &EpisodeName(void) const { return m_strEpisodeName; }

  /*!
   * @brief Change the episode name of this event.
   * @param strEpisodeName The new episode name.
   */
  void SetEpisodeName(const CStdString &strEpisodeName);

  /*!
   * @brief Get the path to the icon for this event.
   * @return The path to the icon
   */
  virtual const CStdString &Icon(void) const { return m_strIconPath; }

  /*!
   * @brief Change the path to the icon for this event.
   * @param strIconPath The new path.
   */
  void SetIcon(const CStdString &strIconPath);

  /*!
   * @brief The path to this event.
   * @return The path.
   */
  const CStdString &Path(void) const { return m_strFileNameAndPath; }

  /*!
   * @brief Change the path to this event.
   * @param strFileNameAndPath The new path.
   */
  void SetPath(const CStdString &strFileNameAndPath);

  /*!
   * @brief Get a pointer to the next event. Set by CEpg in a call to Sort()
   * @return A pointer to the next event or NULL if it's not set.
   */
  const CEpgInfoTag *GetNextEvent() const;

  /*!
   * @brief Get a pointer to the previous event. Set by CEpg in a call to Sort()
   * @return A pointer to the previous event or NULL if it's not set.
   */
  const CEpgInfoTag *GetPreviousEvent() const;

  /*!
   * @brief Update the information in this tag with the info in the given tag.
   * @param tag The new info.
   * @return True if something changed, false otherwise.
   */
  virtual bool Update(const CEpgInfoTag &tag);

  /*!
   * @brief Check if this event is currently active.
   * @return True if it's active, false otherwise.
   */
  bool IsActive(void) const;

  /*!
   * @brief Persist this tag in the database.
   * @param bSingleUpdate True if this is a single update, false if more updates will follow.
   * @param bLastUpdate True to commit a batch of changes, false otherwise.
   * @return True if the tag was persisted correctly, false otherwise.
   */
  bool Persist(bool bSingleUpdate = true, bool bLastUpdate = false);
};
