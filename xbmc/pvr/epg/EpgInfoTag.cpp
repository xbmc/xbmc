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

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "Epg.h"
#include "EpgInfoTag.h"
#include "EpgContainer.h"
#include "EpgDatabase.h"

using namespace PVR;

CPVREpgInfoTagPtr CPVREpgInfoTag::CreateDefaultTag()
{
  return CPVREpgInfoTagPtr(new CPVREpgInfoTag());
}

CPVREpgInfoTag::CPVREpgInfoTag(void) :
    m_bNotify(false),
    m_iBroadcastId(-1),
    m_iGenreType(0),
    m_iGenreSubType(0),
    m_iParentalRating(0),
    m_iStarRating(0),
    m_iSeriesNumber(0),
    m_iEpisodeNumber(0),
    m_iEpisodePart(0),
    m_iUniqueBroadcastID(EPG_TAG_INVALID_UID),
    m_iYear(0),
    m_epg(NULL),
    m_iFlags(EPG_TAG_FLAG_UNDEFINED)
{
}

CPVREpgInfoTag::CPVREpgInfoTag(CPVREpg *epg, const PVR::CPVRChannelPtr &pvrChannel, const std::string &strTableName /* = "" */, const std::string &strIconPath /* = "" */) :
    m_bNotify(false),
    m_iBroadcastId(-1),
    m_iGenreType(0),
    m_iGenreSubType(0),
    m_iParentalRating(0),
    m_iStarRating(0),
    m_iSeriesNumber(0),
    m_iEpisodeNumber(0),
    m_iEpisodePart(0),
    m_iUniqueBroadcastID(EPG_TAG_INVALID_UID),
    m_iYear(0),
    m_strIconPath(strIconPath),
    m_epg(epg),
    m_iFlags(EPG_TAG_FLAG_UNDEFINED),
    m_pvrChannel(pvrChannel)
{
  UpdatePath();
}

CPVREpgInfoTag::CPVREpgInfoTag(const EPG_TAG &data) :
    m_bNotify(false),
    m_iBroadcastId(-1),
    m_iGenreType(0),
    m_iGenreSubType(0),
    m_iParentalRating(0),
    m_iStarRating(0),
    m_iSeriesNumber(0),
    m_iEpisodeNumber(0),
    m_iEpisodePart(0),
    m_iUniqueBroadcastID(EPG_TAG_INVALID_UID),
    m_epg(NULL)
{
  m_startTime = (data.startTime + g_advancedSettings.m_iPVRTimeCorrection);
  m_endTime = (data.endTime + g_advancedSettings.m_iPVRTimeCorrection);
  m_iParentalRating = data.iParentalRating;
  m_iUniqueBroadcastID = data.iUniqueBroadcastId;
  m_bNotify = data.bNotify;
  m_firstAired = (data.firstAired + g_advancedSettings.m_iPVRTimeCorrection);
  m_iSeriesNumber = data.iSeriesNumber;
  m_iEpisodeNumber = data.iEpisodeNumber;
  m_iEpisodePart = data.iEpisodePartNumber;
  m_iStarRating = data.iStarRating;
  m_iYear = data.iYear;
  m_iFlags = data.iFlags;

  SetGenre(data.iGenreType, data.iGenreSubType, data.strGenreDescription);

  // explicit NULL check, because there is no implicit NULL constructor for std::string
  if (data.strTitle)
    m_strTitle = data.strTitle;
  if (data.strPlotOutline)
    m_strPlotOutline = data.strPlotOutline;
  if (data.strPlot)
    m_strPlot = data.strPlot;
  if (data.strOriginalTitle)
    m_strOriginalTitle = data.strOriginalTitle;
  if (data.strCast)
    m_strCast = data.strCast;
  if (data.strDirector)
    m_strDirector = data.strDirector;
  if (data.strWriter)
    m_strWriter = data.strWriter;
  if (data.strIMDBNumber)
    m_strIMDBNumber = data.strIMDBNumber;
  if (data.strEpisodeName)
    m_strEpisodeName = data.strEpisodeName;
  if (data.strIconPath)
    m_strIconPath = data.strIconPath;

  UpdatePath();
}

CPVREpgInfoTag::~CPVREpgInfoTag() = default;

bool CPVREpgInfoTag::operator ==(const CPVREpgInfoTag& right) const
{
  if (this == &right) return true;

  bool bChannelMatch(false);
  {
    CSingleLock lock(m_critSection);
    bChannelMatch = (m_pvrChannel == right.m_pvrChannel);
  }
  return (bChannelMatch &&
          m_bNotify            == right.m_bNotify &&
          m_iBroadcastId       == right.m_iBroadcastId &&
          m_iGenreType         == right.m_iGenreType &&
          m_iGenreSubType      == right.m_iGenreSubType &&
          m_iParentalRating    == right.m_iParentalRating &&
          m_firstAired         == right.m_firstAired &&
          m_iStarRating        == right.m_iStarRating &&
          m_iSeriesNumber      == right.m_iSeriesNumber &&
          m_iEpisodeNumber     == right.m_iEpisodeNumber &&
          m_iEpisodePart       == right.m_iEpisodePart &&
          m_iUniqueBroadcastID == right.m_iUniqueBroadcastID &&
          m_strTitle           == right.m_strTitle &&
          m_strPlotOutline     == right.m_strPlotOutline &&
          m_strPlot            == right.m_strPlot &&
          m_strOriginalTitle   == right.m_strOriginalTitle &&
          m_strCast            == right.m_strCast &&
          m_strDirector        == right.m_strDirector &&
          m_strWriter          == right.m_strWriter &&
          m_iYear              == right.m_iYear &&
          m_strIMDBNumber      == right.m_strIMDBNumber &&
          m_genre              == right.m_genre &&
          m_strEpisodeName     == right.m_strEpisodeName &&
          m_strIconPath        == right.m_strIconPath &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_startTime          == right.m_startTime &&
          m_endTime            == right.m_endTime &&
          m_iFlags             == right.m_iFlags);
}

bool CPVREpgInfoTag::operator !=(const CPVREpgInfoTag& right) const
{
  if (this == &right) return false;

  return !(*this == right);
}

void CPVREpgInfoTag::Serialize(CVariant &value) const
{
  CPVRRecordingPtr recording(Recording());
  value["broadcastid"] = m_iUniqueBroadcastID;
  value["parentalrating"] = m_iParentalRating;
  value["rating"] = m_iStarRating;
  value["title"] = m_strTitle;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["originaltitle"] = m_strOriginalTitle;
  value["cast"] = m_strCast;
  value["director"] = m_strDirector;
  value["writer"] = m_strWriter;
  value["year"] = m_iYear;
  value["imdbnumber"] = m_strIMDBNumber;
  value["genre"] = m_genre;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["starttime"] = m_startTime.IsValid() ? m_startTime.GetAsDBDateTime() : StringUtils::Empty;
  value["endtime"] = m_endTime.IsValid() ? m_endTime.GetAsDBDateTime() : StringUtils::Empty;
  value["runtime"] = GetDuration() / 60;
  value["firstaired"] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : StringUtils::Empty;
  value["progress"] = Progress();
  value["progresspercentage"] = ProgressPercentage();
  value["episodename"] = m_strEpisodeName;
  value["episodenum"] = m_iEpisodeNumber;
  value["episodepart"] = m_iEpisodePart;
  value["hastimer"] = HasTimer();
  value["hastimerrule"] = HasTimerRule();
  value["hasrecording"] = HasRecording();
  value["recording"] = recording ? recording->m_strFileNameAndPath : "";
  value["isactive"] = IsActive();
  value["wasactive"] = WasActive();
  value["isseries"] = IsSeries();
}

CDateTime CPVREpgInfoTag::GetCurrentPlayingTime() const
{
  CDateTime now = CDateTime::GetUTCDateTime();

  CPVRChannelPtr channel(CServiceBroker::GetPVRManager().Clients()->GetPlayingChannel());
  if (channel == ChannelTag())
  {
    // Timeshifting active?
    time_t time = CServiceBroker::GetPVRManager().Clients()->GetPlayingTime();
    if (time > 0) // returns 0 in case no client is currently playing
      now = time;
  }
  return now;
}

bool CPVREpgInfoTag::IsActive(void) const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_startTime <= now && m_endTime > now);
}

bool CPVREpgInfoTag::WasActive(void) const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_endTime < now);
}

bool CPVREpgInfoTag::IsRecordable(void) const
{
  return CServiceBroker::GetPVRManager().Clients()->IsRecordable(shared_from_this());
}

bool CPVREpgInfoTag::IsUpcoming(void) const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_startTime > now);
}

float CPVREpgInfoTag::ProgressPercentage(void) const
{
  float fReturn(0);
  int iDuration;
  time_t currentTime, startTime, endTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);

  m_startTime.GetAsTime(startTime);
  m_endTime.GetAsTime(endTime);
  iDuration = endTime - startTime > 0 ? endTime - startTime : 3600;

  if (currentTime >= startTime && currentTime <= endTime)
    fReturn = static_cast<float>(currentTime - startTime) * 100.0f / iDuration;
  else if (currentTime > endTime)
    fReturn = 100.0f;

  return fReturn;
}

int CPVREpgInfoTag::Progress(void) const
{
  int iDuration;
  time_t currentTime, startTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);

  m_startTime.GetAsTime(startTime);
  iDuration = currentTime - startTime;
  if (iDuration <= 0)
    return 0;

  return iDuration;
}

CPVREpgInfoTagPtr CPVREpgInfoTag::GetNextEvent(void) const
{
  return GetTable()->GetNextEvent(*this);
}

void CPVREpgInfoTag::SetUniqueBroadcastID(unsigned int iUniqueBroadcastID)
{
  m_iUniqueBroadcastID = iUniqueBroadcastID;
}

unsigned int CPVREpgInfoTag::UniqueBroadcastID(void) const
{
  return m_iUniqueBroadcastID;
}

int CPVREpgInfoTag::BroadcastId(void) const
{
  return m_iBroadcastId;
}

CDateTime CPVREpgInfoTag::StartAsUTC(void) const
{
  return m_startTime;
}

CDateTime CPVREpgInfoTag::StartAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_startTime);
  return retVal;
}

CDateTime CPVREpgInfoTag::EndAsUTC(void) const
{
  return m_endTime;
}

CDateTime CPVREpgInfoTag::EndAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_endTime);
  return retVal;
}

void CPVREpgInfoTag::SetEndFromUTC(const CDateTime &end)
{
  m_endTime = end;
}

int CPVREpgInfoTag::GetDuration(void) const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start > 0 ? end - start : 3600;
}

bool CPVREpgInfoTag::IsParentalLocked() const
{
  CPVRChannelPtr channel;
  {
    CSingleLock lock(m_critSection);
    channel = m_pvrChannel;
  }

  return channel && CServiceBroker::GetPVRManager().IsParentalLocked(channel);
}

std::string CPVREpgInfoTag::Title(bool bOverrideParental /* = false */) const
{
  std::string strTitle;

  if (!bOverrideParental && IsParentalLocked())
    strTitle = g_localizeStrings.Get(19266); // parental locked
  else if (m_strTitle.empty() && !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE))
    strTitle = g_localizeStrings.Get(19055); // no information available
  else
    strTitle = m_strTitle;

  return strTitle;
}

std::string CPVREpgInfoTag::PlotOutline(bool bOverrideParental /* = false */) const
{
  std::string retVal;

  if (bOverrideParental || !IsParentalLocked())
    retVal = m_strPlotOutline;

  return retVal;
}

std::string CPVREpgInfoTag::Plot(bool bOverrideParental /* = false */) const
{
  std::string retVal;

  if (bOverrideParental || !IsParentalLocked())
    retVal = m_strPlot;

  return retVal;
}

std::string CPVREpgInfoTag::OriginalTitle(bool bOverrideParental /* = false */) const
{
  std::string retVal;

  if (bOverrideParental || !IsParentalLocked())
    retVal = m_strOriginalTitle;

  return retVal;
}

std::string CPVREpgInfoTag::Cast(void) const
{
  return m_strCast;
}

std::string CPVREpgInfoTag::Director(void) const
{
  return m_strDirector;
}

std::string CPVREpgInfoTag::Writer(void) const
{
  return m_strWriter;
}

int CPVREpgInfoTag::Year(void) const
{
  return m_iYear;
}

std::string CPVREpgInfoTag::IMDBNumber() const
{
  return m_strIMDBNumber;
}

void CPVREpgInfoTag::SetGenre(int iGenreType, int iGenreSubType, const char* strGenre)
{
  if (m_iGenreType != iGenreType || m_iGenreSubType != iGenreSubType)
  {
    m_iGenreType    = iGenreType;
    m_iGenreSubType = iGenreSubType;
    if ((iGenreType == EPG_GENRE_USE_STRING) && (strGenre != NULL) && (strlen(strGenre) > 0))
    {
      /* Type and sub type are not given. No EPG color coding possible
       * Use the provided genre description as backup. */
      m_genre = StringUtils::Split(strGenre, g_advancedSettings.m_videoItemSeparator);
    }
    else
    {
      /* Determine the genre description from the type and subtype IDs */
      m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(iGenreType, iGenreSubType), g_advancedSettings.m_videoItemSeparator);
    }
  }
}

int CPVREpgInfoTag::GenreType(void) const
{
  return m_iGenreType;
}

int CPVREpgInfoTag::GenreSubType(void) const
{
  return m_iGenreSubType;
}

const std::vector<std::string> CPVREpgInfoTag::Genre(void) const
{
  return m_genre;
}

CDateTime CPVREpgInfoTag::FirstAiredAsUTC(void) const
{
  return m_firstAired;
}

CDateTime CPVREpgInfoTag::FirstAiredAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_firstAired);
  return retVal;
}

int CPVREpgInfoTag::ParentalRating(void) const
{
  return m_iParentalRating;
}

int CPVREpgInfoTag::StarRating(void) const
{
  return m_iStarRating;
}

bool CPVREpgInfoTag::Notify(void) const
{
  return m_bNotify;
}

int CPVREpgInfoTag::SeriesNumber(void) const
{
  return m_iSeriesNumber;
}

int CPVREpgInfoTag::EpisodeNumber(void) const
{
  return m_iEpisodeNumber;
}

int CPVREpgInfoTag::EpisodePart(void) const
{
  return m_iEpisodePart;
}

std::string CPVREpgInfoTag::EpisodeName(void) const
{
  return m_strEpisodeName;
}

std::string CPVREpgInfoTag::Icon(void) const
{
  return m_strIconPath;
}

std::string CPVREpgInfoTag::Path(void) const
{
  return m_strFileNameAndPath;
}

bool CPVREpgInfoTag::HasTimer(void) const
{
  return m_timer != NULL;
}

bool CPVREpgInfoTag::HasTimerRule(void) const
{
  CSingleLock lock(m_critSection);
  return m_timer && (m_timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT);
}

CPVRTimerInfoTagPtr CPVREpgInfoTag::Timer(void) const
{
  return m_timer;
}

void CPVREpgInfoTag::SetPVRChannel(const PVR::CPVRChannelPtr &channel)
{
  CSingleLock lock(m_critSection);
  m_pvrChannel = channel;
}

bool CPVREpgInfoTag::HasPVRChannel(void) const
{
  CSingleLock lock(m_critSection);
  return m_pvrChannel.get() != NULL;
}

int CPVREpgInfoTag::PVRChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_pvrChannel ? m_pvrChannel->ChannelNumber() : -1;
}

std::string CPVREpgInfoTag::PVRChannelName(void) const
{
  std::string strReturn;

  {
    CSingleLock lock(m_critSection);
    if (m_pvrChannel)
      strReturn = m_pvrChannel->ChannelName();
  }

  return strReturn;
}

const PVR::CPVRChannelPtr CPVREpgInfoTag::ChannelTag(void) const
{
  CSingleLock lock(m_critSection);
  return m_pvrChannel;
}

bool CPVREpgInfoTag::Update(const CPVREpgInfoTag &tag, bool bUpdateBroadcastId /* = true */)
{
  bool bChanged(false);
  {
    CSingleLock lock(m_critSection);
    bChanged = (m_pvrChannel != tag.m_pvrChannel);
  }

  {
    bChanged |= (
        m_strTitle           != tag.m_strTitle ||
        m_strPlotOutline     != tag.m_strPlotOutline ||
        m_strPlot            != tag.m_strPlot ||
        m_strOriginalTitle   != tag.m_strOriginalTitle ||
        m_strCast            != tag.m_strCast ||
        m_strDirector        != tag.m_strDirector ||
        m_strWriter          != tag.m_strWriter ||
        m_iYear              != tag.m_iYear ||
        m_strIMDBNumber      != tag.m_strIMDBNumber ||
        m_startTime          != tag.m_startTime ||
        m_endTime            != tag.m_endTime ||
        m_iGenreType         != tag.m_iGenreType ||
        m_iGenreSubType      != tag.m_iGenreSubType ||
        m_firstAired         != tag.m_firstAired ||
        m_iParentalRating    != tag.m_iParentalRating ||
        m_iStarRating        != tag.m_iStarRating ||
        m_bNotify            != tag.m_bNotify ||
        m_iEpisodeNumber     != tag.m_iEpisodeNumber ||
        m_iEpisodePart       != tag.m_iEpisodePart ||
        m_iSeriesNumber      != tag.m_iSeriesNumber ||
        m_strEpisodeName     != tag.m_strEpisodeName ||
        m_iUniqueBroadcastID != tag.m_iUniqueBroadcastID ||
        EpgID()              != tag.EpgID() ||
        m_genre              != tag.m_genre ||
        m_strIconPath        != tag.m_strIconPath ||
        m_iFlags             != tag.m_iFlags
    );
    if (bUpdateBroadcastId)
      bChanged |= (m_iBroadcastId != tag.m_iBroadcastId);

    if (bChanged)
    {
      if (bUpdateBroadcastId)
        m_iBroadcastId     = tag.m_iBroadcastId;

      m_strTitle           = tag.m_strTitle;
      m_strPlotOutline     = tag.m_strPlotOutline;
      m_strPlot            = tag.m_strPlot;
      m_strOriginalTitle   = tag.m_strOriginalTitle;
      m_strCast            = tag.m_strCast;
      m_strDirector        = tag.m_strDirector;
      m_strWriter          = tag.m_strWriter;
      m_iYear              = tag.m_iYear;
      m_strIMDBNumber      = tag.m_strIMDBNumber;
      m_startTime          = tag.m_startTime;
      m_endTime            = tag.m_endTime;
      m_iGenreType         = tag.m_iGenreType;
      m_iGenreSubType      = tag.m_iGenreSubType;
      m_epg                = tag.m_epg;
      m_iFlags             = tag.m_iFlags;

      {
        CSingleLock lock(m_critSection);
        m_pvrChannel       = tag.m_pvrChannel;
      }

      if (m_iGenreType == EPG_GENRE_USE_STRING)
      {
        /* No type/subtype. Use the provided description */
        m_genre            = tag.m_genre;
      }
      else
      {
        /* Determine genre description by type/subtype */
        m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType), g_advancedSettings.m_videoItemSeparator);
      }
      m_firstAired         = tag.m_firstAired;
      m_iParentalRating    = tag.m_iParentalRating;
      m_iStarRating        = tag.m_iStarRating;
      m_bNotify            = tag.m_bNotify;
      m_iEpisodeNumber     = tag.m_iEpisodeNumber;
      m_iEpisodePart       = tag.m_iEpisodePart;
      m_iSeriesNumber      = tag.m_iSeriesNumber;
      m_strEpisodeName     = tag.m_strEpisodeName;
      m_iUniqueBroadcastID = tag.m_iUniqueBroadcastID;
      m_strIconPath        = tag.m_strIconPath;
    }
  }
  if (bChanged)
    UpdatePath();

  return bChanged;
}

bool CPVREpgInfoTag::Persist(bool bSingleUpdate /* = true */)
{
  bool bReturn = false;

#if EPG_DEBUGGING
  CLog::Log(LOGDEBUG, "Epg - %s - Infotag '%s' %s, persisting...", __FUNCTION__, m_strTitle.c_str(), m_iBroadcastId > 0 ? "has changes" : "is new");
#endif

  CPVREpgDatabase *database = CServiceBroker::GetPVRManager().EpgContainer().GetDatabase();
  if (!database || (bSingleUpdate && !database->IsOpen()))
  {
    CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
    return bReturn;
  }

  int iId = database->Persist(*this, bSingleUpdate);
  if (iId >= 0)
  {
    bReturn = true;

    if (iId > 0)
      m_iBroadcastId = iId;
  }

  return bReturn;
}

void CPVREpgInfoTag::UpdatePath(void)
{
  m_strFileNameAndPath = StringUtils::Format("pvr://guide/%04i/%s.epg", EpgID(), m_startTime.GetAsDBDateTime().c_str());
}

const CPVREpg *CPVREpgInfoTag::GetTable() const
{
  return m_epg;
}

const int CPVREpgInfoTag::EpgID(void) const
{
  return m_epg ? m_epg->EpgID() : -1;
}

void CPVREpgInfoTag::SetTimer(const CPVRTimerInfoTagPtr &timer)
{
  m_timer = timer;
}

void CPVREpgInfoTag::ClearTimer(void)
{
  CPVRTimerInfoTagPtr previousTag;
  previousTag = m_timer;
  CPVRTimerInfoTagPtr empty;
  m_timer = empty;

  if (previousTag)
    previousTag->ClearEpgTag();
}

void CPVREpgInfoTag::SetRecording(const CPVRRecordingPtr &recording)
{
  CSingleLock lock(m_critSection);
  m_recording = recording;
}

void CPVREpgInfoTag::ClearRecording(void)
{
  CSingleLock lock(m_critSection);
  m_recording.reset();
}

bool CPVREpgInfoTag::HasRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_recording.get() != NULL;
}

CPVRRecordingPtr CPVREpgInfoTag::Recording(void) const
{
  CSingleLock lock(m_critSection);
  return m_recording;
}

void CPVREpgInfoTag::SetEpg(CPVREpg *epg)
{
  m_epg = epg;
}

bool CPVREpgInfoTag::IsSeries(void) const
{
  CSingleLock lock(m_critSection);
  if ((m_iFlags & EPG_TAG_FLAG_IS_SERIES) > 0 || SeriesNumber() > 0 || EpisodeNumber() > 0 || EpisodePart() > 0)
    return true;
  else
    return false;
}
