#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include <memory>
#include <string>

#include "XBDateTime.h"
#include "addons/include/xbmc_pvr_types.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "utils/ISerializable.h"

#define EPG_DEBUGGING 0

/** an EPG info tag */
namespace EPG
{
  class CEpg;

  class CEpgInfoTag;
  typedef std::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;

  class CEpgInfoTag : public ISerializable
  {
    friend class CEpg;
    friend class CEpgDatabase;

  public:
    /*!
     * @brief Create a new empty event .
     */
    static CEpgInfoTagPtr CreateDefaultTag();

  private:
    /*!
     * @brief Create a new empty event.
     */
    CEpgInfoTag(void);

    /*!
     * @brief Create a new empty event without a unique ID.
     */
    CEpgInfoTag(CEpg *epg, PVR::CPVRChannelPtr pvrChannel, const std::string &strTableName = "", const std::string &strIconPath = "");

    /*!
     * @brief Create a new EPG infotag with 'data' as content.
     * @param data The tag's content.
     */
    CEpgInfoTag(const EPG_TAG &data);

    // Prevent copy construction, even for CEpgInfoTag instances and friends.
    // Note: Only declared, but intentionally not implemented
    //       to prevent compiler generated copy ctor and to force
    //       a linker error in case somebody tries to call it.
    CEpgInfoTag(const CEpgInfoTag &tag);

    // Prevent copy by assignment, even for CEpgInfoTag instances and friends.
    // Note: Only declared, but intentionally not implemented
    //       to prevent compiler generated assignment operator and to force
    //       a linker error in case somebody tries to call it.
    CEpgInfoTag &operator =(const CEpgInfoTag &other);

  public:
    virtual ~CEpgInfoTag();

    bool operator ==(const CEpgInfoTag& right) const;
    bool operator !=(const CEpgInfoTag& right) const;

    virtual void Serialize(CVariant &value) const;

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
     * @return True when this event is an upcoming event, false otherwise.
     */
    bool IsUpcoming(void) const;

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
     * @brief Sets the epg reference of this event
     * @param epg The epg item
     */
    void SetEpg(CEpg *epg);

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

    /*!
     * @brief Get the duration of this event in seconds.
     * @return The duration in seconds.
     */
    int GetDuration(void) const;

    /*!
     * @brief Get the title of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The title.
     */
    std::string Title(bool bOverrideParental = false) const;

    /*!
     * @brief Get the plot outline of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The plot outline.
     */
    std::string PlotOutline(bool bOverrideParental = false) const;

    /*!
     * @brief Get the plot of this event.
     * @param bOverrideParental True to override parental control, false check it.
     * @return The plot.
     */
    std::string Plot(bool bOverrideParental = false) const;

    /*!
     * @brief Get the originaltitle of this event.
     * @return The originaltitle.
     */
    std::string OriginalTitle(bool bOverrideParental = false) const;

    /*!
     * @brief Get the cast of this event.
     * @return The cast.
     */
    std::string Cast() const;

    /*!
     * @brief Get the director of this event.
     * @return The director.
     */
    std::string Director() const;

    /*!
     * @brief Get the writer of this event.
     * @return The writer.
     */
    std::string Writer() const;

    /*!
     * @brief Get the year of this event.
     * @return The year.
     */
    int Year() const;

    /*!
     * @brief Get the imdbnumber of this event.
     * @return The imdbnumber.
     */
    std::string IMDBNumber() const;

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
     * @brief Get the first air date of this event.
     * @return The first air date.
     */
    CDateTime FirstAiredAsUTC(void) const;
    CDateTime FirstAiredAsLocalTime(void) const;

    /*!
     * @brief Get the parental rating of this event.
     * @return The parental rating.
     */
    int ParentalRating(void) const;

    /*!
     * @brief Get the star rating of this event.
     * @return The star rating.
     */
    int StarRating(void) const;

    /*!
     * @brief Notify on start if true.
     * @return Notify on start.
     */
    bool Notify(void) const;

    /*!
     * @brief The series number of this event.
     * @return The series number.
     */
    int SeriesNumber(void) const;

    /*!
     * @brief The episode number of this event.
     * @return The episode number.
     */
    int EpisodeNumber(void) const;

    /*!
     * @brief The episode part number of this event.
     * @return The episode part number.
     */
    int EpisodePart(void) const;

    /*!
     * @brief The episode name of this event.
     * @return The episode name.
     */
    std::string EpisodeName(void) const;

    /*!
     * @brief Get the path to the icon for this event.
     * @return The path to the icon
     */
    std::string Icon(void) const;

    /*!
     * @brief The path to this event.
     * @return The path.
     */
    std::string Path(void) const;

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
     * @brief Check whether this event has an active timer schedule.
     * @return True if it has an active timer schedule, false if not.
     */
    bool HasTimerSchedule(void) const;

    /*!
     * @brief Get a pointer to the timer for event or NULL if there is none.
     * @return A pointer to the timer for event or NULL if there is none.
     */
    PVR::CPVRTimerInfoTagPtr Timer(void) const;

    /*!
     * @brief Set a recording for this event or NULL to clear it.
     * @param recording The recording value.
     */
    void SetRecording(PVR::CPVRRecordingPtr recording);

    /*!
     * @brief Clear a recording for this event.
     */
    void ClearRecording(void);

    /*!
     * @brief Check whether this event has a recording tag.
     * @return True if it has a recording tag, false if not.
     */
    bool HasRecording(void) const;

    /*!
     * @brief Get a pointer to the recording for event or NULL if there is none.
     * @return A pointer to the recording for event or NULL if there is none.
     */
    PVR::CPVRRecordingPtr Recording(void) const;

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

    std::string PVRChannelName(void) const;

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
     * @param bUpdateBroadcastId If set to false, the tag BroadcastId (locally unique) will not be chacked/updated
     * @return True if something changed, false otherwise.
     */
    bool Update(const CEpgInfoTag &tag, bool bUpdateBroadcastId = true);

  private:

    /*!
     * @brief Change the genre of this event.
     * @param iGenreType The genre type ID.
     * @param iGenreSubType The genre subtype ID.
     */
    void SetGenre(int iGenreType, int iGenreSubType, const char* strGenre);

    /*!
     * @brief Hook that is called when the start date changed.
     */
    void UpdatePath(void);

    /*!
     * @brief Get current time, taking timeshifting into account.
     */
    CDateTime GetCurrentPlayingTime(void) const;

    bool                     m_bNotify;            /*!< notify on start */

    int                      m_iBroadcastId;       /*!< database ID */
    int                      m_iGenreType;         /*!< genre type */
    int                      m_iGenreSubType;      /*!< genre subtype */
    int                      m_iParentalRating;    /*!< parental rating */
    int                      m_iStarRating;        /*!< star rating */
    int                      m_iSeriesNumber;      /*!< series number */
    int                      m_iEpisodeNumber;     /*!< episode number */
    int                      m_iEpisodePart;       /*!< episode part number */
    int                      m_iUniqueBroadcastID; /*!< unique broadcast ID */
    std::string              m_strTitle;           /*!< title */
    std::string              m_strPlotOutline;     /*!< plot outline */
    std::string              m_strPlot;            /*!< plot */
    std::string              m_strOriginalTitle;   /*!< original title */
    std::string              m_strCast;            /*!< cast */
    std::string              m_strDirector;        /*!< director */
    std::string              m_strWriter;          /*!< writer */
    int                      m_iYear;              /*!< year */
    std::string              m_strIMDBNumber;      /*!< imdb number */
    std::vector<std::string> m_genre;              /*!< genre */
    std::string              m_strEpisodeName;     /*!< episode name */
    std::string              m_strIconPath;        /*!< the path to the icon */
    std::string              m_strFileNameAndPath; /*!< the filename and path */
    CDateTime                m_startTime;          /*!< event start time */
    CDateTime                m_endTime;            /*!< event end time */
    CDateTime                m_firstAired;         /*!< first airdate */

    PVR::CPVRTimerInfoTagPtr m_timer;

    CEpg *                   m_epg;                /*!< the schedule that this event belongs to */

    CCriticalSection         m_critSection;
    PVR::CPVRChannelPtr      m_pvrChannel;
    PVR::CPVRRecordingPtr    m_recording;
  };
}
