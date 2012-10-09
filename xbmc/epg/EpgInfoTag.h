#pragma once

/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/include/xbmc_pvr_types.h"
#include "XBDateTime.h"
#include "utils/ISerializable.h"
#include "utils/StringUtils.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/timers/PVRTimerInfoTag.h"

#include <boost/shared_ptr.hpp>

#define EPG_DEBUGGING 0

/** an EPG info tag */
namespace EPG
{
  class CEpg;

  class CEpgInfoTag;
  typedef boost::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;

  class CEpgInfoTag : public ISerializable
  {
    friend class CEpg;
    friend class CEpgDatabase;
    friend class PVR::CPVRTimerInfoTag;

  public:
    CEpgInfoTag(void);

    /*!
     * @brief Create a new empty event without a unique ID.
     */
    CEpgInfoTag(CEpg *epg, PVR::CPVRChannelPtr pvrChannel, const CStdString &strTableName = StringUtils::EmptyString, const CStdString &strIconPath = StringUtils::EmptyString);

    /*!
     * @brief Create a new EPG infotag with 'data' as content.
     * @param data The tag's content.
     */
    CEpgInfoTag(const EPG_TAG &data);

    /*!
     * @brief Create a new EPG infotag with 'tag' as content.
     * @param tag The tag's content.
     */
    CEpgInfoTag(const CEpgInfoTag &tag);
    virtual ~CEpgInfoTag();

    bool operator ==(const CEpgInfoTag& right) const;
    bool operator !=(const CEpgInfoTag& right) const;
    CEpgInfoTag &operator =(const CEpgInfoTag &other);

    virtual void Serialize(CVariant &value) const;

    /*!
     * @brief Check whether this tag has changed and unsaved values.
     * @return True if it has unsaved values, false otherwise.
     */
    bool Changed(void) const;

    /*!
     * @brief Check if this event is currently active.
     * @return True if it's active, false otherwise.
     */
    bool IsActive(void) const;

    /*!
     * @return True when this event has already passed, false otherwise.
     */
    bool WasActive(void) const;

    /*!
     * @return True when this event is in the future, false otherwise.
     */
    bool InTheFuture(void) const;

    /*!
     * @return The current progress of this tag.
     */
    float ProgressPercentage(void) const;

    /*!
     * @return The current progress of this tag in seconds.
     */
    int Progress(void) const;

    /*!
     * @brief Get a pointer to the next event. Set by CEpg in a call to Sort()
     * @return A pointer to the next event or NULL if it's not set.
     */
    CEpgInfoTagPtr GetNextEvent(void) const;

    /*!
     * @brief Get a pointer to the previous event. Set by CEpg in a call to Sort()
     * @return A pointer to the previous event or NULL if it's not set.
     */
    CEpgInfoTagPtr GetPreviousEvent(void) const;

    /*!
     * @brief The table this event belongs to
     * @return The table this event belongs to
     */
    const CEpg *GetTable() const;

    const int EpgID(void) const;

    /*!
     * @brief Change the unique broadcast ID of this event.
     * @param iUniqueBroadcastId The new unique broadcast ID.
     */
    void SetUniqueBroadcastID(int iUniqueBroadcastID);

    /*!
     * @brief Get the unique broadcast ID.
     * @return The unique broadcast ID.
     */
    int UniqueBroadcastID(void) const;

    /*!
     * @brief Change the event's database ID.
     * @param iId The new database ID.
     */
    void SetBroadcastId(int iId);

    /*!
     * @brief Get the event's database ID.
     * @return The database ID.
     */
    int BroadcastId(void) const;

    /*!
     * @brief Get the event's start time.
     * @return The new start time.
     */
    CDateTime StartAsUTC(void) const;
    CDateTime StartAsLocalTime(void) const;

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
    CDateTime EndAsUTC(void) const;
    CDateTime EndAsLocalTime(void) const;

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
    int GetDuration(void) const;

    /*!
     * @brief Change the title of this event.
     * @param strTitle The new title.
     */
    void SetTitle(const CStdString &strTitle);

    /*!
     * @brief Get the title of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The title.
     */
    CStdString Title(bool bOverrideParental = false) const;

    /*!
     * @brief Change the plot outline of this event.
     * @param strPlotOutline The new plot outline.
     */
    void SetPlotOutline(const CStdString &strPlotOutline);

    /*!
     * @brief Get the plot outline of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The plot outline.
     */
    CStdString PlotOutline(bool bOverrideParental = false) const;

    /*!
     * @brief Change the plot of this event.
     * @param strPlot The new plot.
     */
    void SetPlot(const CStdString &strPlot);

    /*!
     * @brief Get the plot of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The plot.
     */
    CStdString Plot(bool bOverrideParental = false) const;

    /*!
     * @brief Change the genre of this event.
     * @param iID The genre type ID.
     * @param iSubID The genre subtype ID.
     */
    void SetGenre(int iID, int iSubID, const char* strGenre);

    /*!
     * @brief Get the genre type ID of this event.
     * @return The genre type ID.
     */
    int GenreType(void) const;

    /*!
     * @brief Get the genre subtype ID of this event.
     * @return The genre subtype ID.
     */
    int GenreSubType(void) const;

    /*!
     * @brief Get the genre as human readable string.
     * @return The genre.
     */
    const std::vector<std::string> Genre(void) const;

    /*!
     * @brief Change the first air date of this event.
     * @param firstAired The new first air date.
     */
    void SetFirstAiredFromUTC(const CDateTime &firstAired);
    void SetFirstAiredFromLocalTime(const CDateTime &firstAired);

    /*!
     * @brief Get the first air date of this event.
     * @return The first air date.
     */
    CDateTime FirstAiredAsUTC(void) const;
    CDateTime FirstAiredAsLocalTime(void) const;

    /*!
     * @brief Change the parental rating of this event.
     * @param iParentalRating The new parental rating.
     */
    void SetParentalRating(int iParentalRating);

    /*!
     * @brief Get the parental rating of this event.
     * @return The parental rating.
     */
    int ParentalRating(void) const;

    /*!
     * @brief Change the star rating of this event.
     * @param iStarRating The new star rating.
     */
    void SetStarRating(int iStarRating);

    /*!
     * @brief Get the star rating of this event.
     * @return The star rating.
     */
    int StarRating(void) const;

    /*!
     * @brief Change the value of notify on start.
     * @param bNotify The new value.
     */
    void SetNotify(bool bNotify);

    /*!
     * @brief Notify on start if true.
     * @return Notify on start.
     */
    bool Notify(void) const;

    /*!
     * @brief Change the series number of this event.
     * @param iSeriesNum The new series number.
     */
    void SetSeriesNum(int iSeriesNum);

    /*!
     * @brief The series number of this event.
     * @return The series number.
     */
    int SeriesNum(void) const;

    /*!
     * @brief Change the episode number of this event.
     * @param iEpisodeNum The new episode number.
     */
    void SetEpisodeNum(int iEpisodeNum);

    /*!
     * @brief The episode number of this event.
     * @return The episode number.
     */
    int EpisodeNum(void) const;

    /*!
     * @brief Change the episode part number of this event.
     * @param iEpisodePart The new episode part number.
     */
    void SetEpisodePart(int iEpisodePart);

    /*!
     * @brief The episode part number of this event.
     * @return The episode part number.
     */
    int EpisodePart(void) const;

    /*!
     * @brief Change the episode name of this event.
     * @param strEpisodeName The new episode name.
     */
    void SetEpisodeName(const CStdString &strEpisodeName);

    /*!
     * @brief The episode name of this event.
     * @return The episode name.
     */
    CStdString EpisodeName(void) const;

    /*!
     * @brief Change the path to the icon for this event.
     * @param strIconPath The new path.
     */
    void SetIcon(const CStdString &strIconPath);

    /*!
     * @brief Get the path to the icon for this event.
     * @return The path to the icon
     */
    CStdString Icon(void) const;

    /*!
     * @brief Change the path to this event.
     * @param strFileNameAndPath The new path.
     */
    void SetPath(const CStdString &strFileNameAndPath);

    /*!
     * @brief The path to this event.
     * @return The path.
     */
    CStdString Path(void) const;

    /*!
     * @brief Set a timer for this event or NULL to clear it.
     * @param newTimer The new timer value.
     */
    void SetTimer(PVR::CPVRTimerInfoTagPtr newTimer);
    void ClearTimer(void);

    /*!
     * @brief Check whether this event has an active timer tag.
     * @return True if it has an active timer tag, false if not.
     */
    bool HasTimer(void) const;

    /*!
     * @brief Get a pointer to the timer for event or NULL if there is none.
     * @return A pointer to the timer for event or NULL if there is none.
     */
    PVR::CPVRTimerInfoTagPtr Timer(void) const;

    /*!
     * @brief Change the channel tag of this epg tag
     * @param channel The new channel
     */
    void SetPVRChannel(PVR::CPVRChannelPtr channel);

    /*!
     * @return True if this tag has a PVR channel set.
     */
    bool HasPVRChannel(void) const;

    int PVRChannelNumber(void) const;

    CStdString PVRChannelName(void) const;

    /*!
     * @brief Get the channel that plays this event.
     * @return a pointer to the channel.
     */
    const PVR::CPVRChannelPtr ChannelTag(void) const;

    /*!
     * @brief Persist this tag in the database.
     * @param bSingleUpdate True if this is a single update, false if more updates will follow.
     * @return True if the tag was persisted correctly, false otherwise.
     */
    bool Persist(bool bSingleUpdate = true);

    /*!
     * @brief Update the information in this tag with the info in the given tag.
     * @param tag The new info.
     */
    void Update(const EPG_TAG &tag);

    /*!
     * @brief Update the information in this tag with the info in the given tag.
     * @param tag The new info.
     * @param bUpdateBroadcastId If set to false, the tag BroadcastId (locally unique) will not be chacked/updated
     * @return True if something changed, false otherwise.
     */
    bool Update(const CEpgInfoTag &tag, bool bUpdateBroadcastId = true);
  protected:
    /*!
     * @brief Hook that is called when the start date changed.
     */
    void UpdatePath(void);

    bool                     m_bNotify;            /*!< notify on start */
    bool                     m_bChanged;           /*!< keep track of changes to this entry */

    int                      m_iBroadcastId;       /*!< database ID */
    int                      m_iGenreType;         /*!< genre type */
    int                      m_iGenreSubType;      /*!< genre subtype */
    int                      m_iParentalRating;    /*!< parental rating */
    int                      m_iStarRating;        /*!< star rating */
    int                      m_iSeriesNumber;      /*!< series number */
    int                      m_iEpisodeNumber;     /*!< episode number */
    int                      m_iEpisodePart;       /*!< episode part number */
    int                      m_iUniqueBroadcastID; /*!< unique broadcast ID */
    CStdString               m_strTitle;           /*!< title */
    CStdString               m_strPlotOutline;     /*!< plot outline */
    CStdString               m_strPlot;            /*!< plot */
    std::vector<std::string> m_genre;            /*!< genre */
    CStdString               m_strEpisodeName;     /*!< episode name */
    CStdString               m_strIconPath;        /*!< the path to the icon */
    CStdString               m_strFileNameAndPath; /*!< the filename and path */
    CDateTime                m_startTime;          /*!< event start time */
    CDateTime                m_endTime;            /*!< event end time */
    CDateTime                m_firstAired;         /*!< first airdate */

    PVR::CPVRTimerInfoTagPtr m_timer;
    CEpg *                   m_epg;                /*!< the schedule that this event belongs to */

    PVR::CPVRChannelPtr    m_pvrChannel;
    CCriticalSection       m_critSection;
  };
}
