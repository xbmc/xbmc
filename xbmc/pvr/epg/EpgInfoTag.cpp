/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgInfoTag.h"

#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "cores/DataCacheCore.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;

CPVREpgInfoTag::CPVREpgInfoTag(const CPVRChannelPtr &channel, CPVREpg *epg /* = nullptr */, const std::string &strTableName /* = "" */)
: m_iClientId(channel ? channel->ClientID() : -1),
  m_iUniqueChannelID(channel ? channel->UniqueID() : PVR_CHANNEL_INVALID_UID),
  m_strIconPath(channel ? channel->IconPath() : ""),
  m_epg(epg),
  m_channel(channel)
{
  UpdatePath();
}

CPVREpgInfoTag::CPVREpgInfoTag(const EPG_TAG &data, int iClientId)
: m_bNotify(data.bNotify),
  m_iClientId(iClientId),
  m_iParentalRating(data.iParentalRating),
  m_iStarRating(data.iStarRating),
  m_iSeriesNumber(data.iSeriesNumber),
  m_iEpisodeNumber(data.iEpisodeNumber),
  m_iEpisodePart(data.iEpisodePartNumber),
  m_iUniqueBroadcastID(data.iUniqueBroadcastId),
  m_iUniqueChannelID(data.iUniqueChannelId),
  m_iYear(data.iYear),
  m_startTime(data.startTime + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection),
  m_endTime(data.endTime + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection),
  m_firstAired(data.firstAired + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection),
  m_iFlags(data.iFlags)
{
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
    m_cast = Tokenize(data.strCast);
  if (data.strDirector)
    m_directors = Tokenize(data.strDirector);
  if (data.strWriter)
    m_writers = Tokenize(data.strWriter);
  if (data.strIMDBNumber)
    m_strIMDBNumber = data.strIMDBNumber;
  if (data.strEpisodeName)
    m_strEpisodeName = data.strEpisodeName;
  if (data.strIconPath)
    m_strIconPath = data.strIconPath;
  if (data.strSeriesLink)
    m_strSeriesLink = data.strSeriesLink;

  UpdatePath();
}

bool CPVREpgInfoTag::operator ==(const CPVREpgInfoTag& right) const
{
  if (this == &right)
    return true;

  bool bChannelMatch = false;
  {
    CSingleLock lock(m_critSection);
    bChannelMatch = (m_channel == right.m_channel);
  }
  return (bChannelMatch &&
          m_bNotify            == right.m_bNotify &&
          m_iClientId          == right.m_iClientId &&
          m_iDatabaseID        == right.m_iDatabaseID &&
          m_iGenreType         == right.m_iGenreType &&
          m_iGenreSubType      == right.m_iGenreSubType &&
          m_iParentalRating    == right.m_iParentalRating &&
          m_firstAired         == right.m_firstAired &&
          m_iStarRating        == right.m_iStarRating &&
          m_iSeriesNumber      == right.m_iSeriesNumber &&
          m_iEpisodeNumber     == right.m_iEpisodeNumber &&
          m_iEpisodePart       == right.m_iEpisodePart &&
          m_iUniqueBroadcastID == right.m_iUniqueBroadcastID &&
          m_iUniqueChannelID   == right.m_iUniqueChannelID &&
          m_strTitle           == right.m_strTitle &&
          m_strPlotOutline     == right.m_strPlotOutline &&
          m_strPlot            == right.m_strPlot &&
          m_strOriginalTitle   == right.m_strOriginalTitle &&
          m_cast               == right.m_cast &&
          m_directors          == right.m_directors &&
          m_writers            == right.m_writers &&
          m_iYear              == right.m_iYear &&
          m_strIMDBNumber      == right.m_strIMDBNumber &&
          m_genre              == right.m_genre &&
          m_strEpisodeName     == right.m_strEpisodeName &&
          m_strIconPath        == right.m_strIconPath &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_startTime          == right.m_startTime &&
          m_endTime            == right.m_endTime &&
          m_iFlags             == right.m_iFlags &&
          m_strSeriesLink      == right.m_strSeriesLink);
}

bool CPVREpgInfoTag::operator !=(const CPVREpgInfoTag& right) const
{
  if (this == &right)
    return false;

  return !(*this == right);
}

void CPVREpgInfoTag::Serialize(CVariant &value) const
{
  const CPVRRecordingPtr recording = Recording();

  value["broadcastid"] = m_iUniqueBroadcastID;
  value["channeluid"] = m_iUniqueChannelID;
  value["parentalrating"] = m_iParentalRating;
  value["rating"] = m_iStarRating;
  value["title"] = m_strTitle;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["originaltitle"] = m_strOriginalTitle;
  value["cast"] = DeTokenize(m_cast);
  value["director"] = DeTokenize(m_directors);
  value["writer"] = DeTokenize(m_writers);
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
  value["serieslink"] = m_strSeriesLink;
}

void CPVREpgInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  CSingleLock lock(m_critSection);

  if (!m_channel)
    return;

  switch (field)
  {
    case FieldChannelName:
      sortable[FieldChannelName] = m_channel->ChannelName();
      break;
    case FieldChannelNumber:
      sortable[FieldChannelNumber] = m_channel->ChannelNumber().FormattedChannelNumber();
      break;
    case FieldLastPlayed:
    {
      const CDateTime lastWatched(m_channel->LastWatched());
      sortable[FieldLastPlayed] = lastWatched.IsValid() ? lastWatched.GetAsDBDateTime() : StringUtils::Empty;
      break;
    }
    default:
      break;
  }
}

CDateTime CPVREpgInfoTag::GetCurrentPlayingTime() const
{
  if (CServiceBroker::GetPVRManager().GetPlayingChannel() == Channel())
  {
    // start time valid?
    time_t startTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (startTime > 0)
    {
      return CDateTime(startTime + CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
    }
  }

  return CDateTime::GetUTCDateTime();
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

bool CPVREpgInfoTag::IsUpcoming(void) const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_startTime > now);
}

float CPVREpgInfoTag::ProgressPercentage(void) const
{
  float fReturn = 0.0f;

  time_t currentTime, startTime, endTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);
  m_startTime.GetAsTime(startTime);
  m_endTime.GetAsTime(endTime);
  int iDuration = endTime - startTime > 0 ? endTime - startTime : 3600;

  if (currentTime >= startTime && currentTime <= endTime)
    fReturn = static_cast<float>(currentTime - startTime) * 100.0f / iDuration;
  else if (currentTime > endTime)
    fReturn = 100.0f;

  return fReturn;
}

int CPVREpgInfoTag::Progress(void) const
{
  time_t currentTime, startTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);
  m_startTime.GetAsTime(startTime);
  int iDuration = currentTime - startTime;

  if (iDuration <= 0)
    return 0;

  return iDuration;
}

void CPVREpgInfoTag::SetUniqueBroadcastID(unsigned int iUniqueBroadcastID)
{
  m_iUniqueBroadcastID = iUniqueBroadcastID;
}

unsigned int CPVREpgInfoTag::UniqueBroadcastID(void) const
{
  return m_iUniqueBroadcastID;
}

int CPVREpgInfoTag::DatabaseID(void) const
{
  return m_iDatabaseID;
}

unsigned int CPVREpgInfoTag::UniqueChannelID(void) const
{
  return m_iUniqueChannelID;
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
    channel = m_channel;
  }

  return channel && CServiceBroker::GetPVRManager().IsParentalLocked(channel);
}

std::string CPVREpgInfoTag::Title(bool bOverrideParental /* = false */) const
{
  std::string strTitle;

  if (!bOverrideParental && IsParentalLocked())
    strTitle = g_localizeStrings.Get(19266); // parental locked
  else if (m_strTitle.empty() && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE))
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

const std::vector<std::string> CPVREpgInfoTag::Cast(void) const
{
  return m_cast;
}

const std::vector<std::string> CPVREpgInfoTag::Directors(void) const
{
  return m_directors;
}

const std::vector<std::string> CPVREpgInfoTag::Writers(void) const
{
  return m_writers;
}

const std::string CPVREpgInfoTag::GetCastLabel() const
{
  // Note: see CVideoInfoTag::GetCast for reference implementation.
  std::string strLabel;
  for (const auto& castEntry : m_cast)
    strLabel += StringUtils::Format("%s\n", castEntry.c_str());

  return StringUtils::TrimRight(strLabel, "\n");
}

const std::string CPVREpgInfoTag::GetDirectorsLabel() const
{
  return StringUtils::Join(m_directors, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}

const std::string CPVREpgInfoTag::GetWritersLabel() const
{
  return StringUtils::Join(m_writers, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}

const std::string CPVREpgInfoTag::GetGenresLabel() const
{
  return StringUtils::Join(m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
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
      m_genre = Tokenize(strGenre);
    }
    else
    {
      /* Determine the genre description from the type and subtype IDs */
      m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(iGenreType, iGenreSubType), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
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

std::string CPVREpgInfoTag::SeriesLink() const
{
  return m_strSeriesLink;
}

int CPVREpgInfoTag::EpisodeNumber(void) const
{
  return m_iEpisodeNumber;
}

int CPVREpgInfoTag::EpisodePart(void) const
{
  return m_iEpisodePart;
}

std::string CPVREpgInfoTag::EpisodeName(bool bOverrideParental /* = false */) const
{
  std::string retVal;

  if (bOverrideParental || !IsParentalLocked())
    retVal = m_strEpisodeName;

  return retVal;
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
  CSingleLock lock(m_critSection);
  return m_timer != nullptr;
}

bool CPVREpgInfoTag::HasTimerRule(void) const
{
  CSingleLock lock(m_critSection);
  return m_timer && (m_timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT);
}

CPVRTimerInfoTagPtr CPVREpgInfoTag::Timer(void) const
{
  CSingleLock lock(m_critSection);
  return m_timer;
}

void CPVREpgInfoTag::SetChannel(const CPVRChannelPtr &channel)
{
  CSingleLock lock(m_critSection);
  m_channel = channel;
  m_iClientId = m_channel ? m_channel->ClientID() : -1;
  m_iUniqueChannelID = m_channel ? m_channel->UniqueID() : PVR_CHANNEL_INVALID_UID;
}

bool CPVREpgInfoTag::HasChannel(void) const
{
  CSingleLock lock(m_critSection);
  return m_channel != nullptr;
}

const CPVRChannelPtr CPVREpgInfoTag::Channel() const
{
  CSingleLock lock(m_critSection);
  return m_channel;
}

bool CPVREpgInfoTag::Update(const CPVREpgInfoTag &tag, bool bUpdateBroadcastId /* = true */)
{
  bool bChanged = false;
  {
    CSingleLock lock(m_critSection);
    bChanged = (m_channel != tag.m_channel);
  }

  {
    bChanged |= (
        m_iClientId          != tag.m_iClientId ||
        m_strTitle           != tag.m_strTitle ||
        m_strPlotOutline     != tag.m_strPlotOutline ||
        m_strPlot            != tag.m_strPlot ||
        m_strOriginalTitle   != tag.m_strOriginalTitle ||
        m_cast               != tag.m_cast ||
        m_directors          != tag.m_directors ||
        m_writers            != tag.m_writers ||
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
        m_iUniqueChannelID   != tag.m_iUniqueChannelID ||
        EpgID()              != tag.EpgID() ||
        m_genre              != tag.m_genre ||
        m_strIconPath        != tag.m_strIconPath ||
        m_iFlags             != tag.m_iFlags ||
        m_strSeriesLink      != tag.m_strSeriesLink
    );
    if (bUpdateBroadcastId)
      bChanged |= (m_iDatabaseID != tag.m_iDatabaseID);

    if (bChanged)
    {
      if (bUpdateBroadcastId)
        m_iDatabaseID      = tag.m_iDatabaseID;

      m_iClientId          = tag.m_iClientId;
      m_strTitle           = tag.m_strTitle;
      m_strPlotOutline     = tag.m_strPlotOutline;
      m_strPlot            = tag.m_strPlot;
      m_strOriginalTitle   = tag.m_strOriginalTitle;
      m_cast               = tag.m_cast;
      m_directors          = tag.m_directors;
      m_writers            = tag.m_writers;
      m_iYear              = tag.m_iYear;
      m_strIMDBNumber      = tag.m_strIMDBNumber;
      m_startTime          = tag.m_startTime;
      m_endTime            = tag.m_endTime;
      m_iGenreType         = tag.m_iGenreType;
      m_iGenreSubType      = tag.m_iGenreSubType;
      m_epg                = tag.m_epg;
      m_iFlags             = tag.m_iFlags;
      m_strSeriesLink      = tag.m_strSeriesLink;

      {
        CSingleLock lock(m_critSection);
        m_channel          = tag.m_channel;
      }

      if (m_iGenreType == EPG_GENRE_USE_STRING)
      {
        /* No type/subtype. Use the provided description */
        m_genre            = tag.m_genre;
      }
      else
      {
        /* Determine genre description by type/subtype */
        m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
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
      m_iUniqueChannelID   = tag.m_iUniqueChannelID;
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

  const CPVREpgDatabasePtr database = CServiceBroker::GetPVRManager().EpgContainer().GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "Could not open the EPG database");
    return bReturn;
  }

  int iId = database->Persist(*this, bSingleUpdate);
  if (iId >= 0)
  {
    bReturn = true;

    if (iId > 0)
      m_iDatabaseID = iId;
  }

  return bReturn;
}

std::vector<PVR_EDL_ENTRY> CPVREpgInfoTag::GetEdl() const
{
  std::vector<PVR_EDL_ENTRY> edls;
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);

  if (client && client->GetClientCapabilities().SupportsEpgTagEdl())
    client->GetEpgTagEdl(shared_from_this(), edls);

  return edls;
}

void CPVREpgInfoTag::UpdatePath(void)
{
  m_strFileNameAndPath = StringUtils::Format("pvr://guide/%04i/%s.epg", EpgID(), m_startTime.GetAsDBDateTime().c_str());
}

int CPVREpgInfoTag::EpgID(void) const
{
  return m_epg ? m_epg->EpgID() : -1;
}

void CPVREpgInfoTag::SetTimer(const CPVRTimerInfoTagPtr &timer)
{
  CSingleLock lock(m_critSection);
  m_timer = timer;
}

void CPVREpgInfoTag::ClearTimer(void)
{
  CPVRTimerInfoTagPtr previousTag;
  {
    CSingleLock lock(m_critSection);
    previousTag = std::move(m_timer);
  }

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
  return m_recording != nullptr;
}

CPVRRecordingPtr CPVREpgInfoTag::Recording(void) const
{
  CSingleLock lock(m_critSection);
  return m_recording;
}

bool CPVREpgInfoTag::IsRecordable(void) const
{
  bool bIsRecordable = false;
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (!client || (client->IsRecordable(shared_from_this(), bIsRecordable) != PVR_ERROR_NO_ERROR))
  {
    // event end time based fallback
    bIsRecordable = EndAsLocalTime() > CDateTime::GetCurrentDateTime();
  }
  return bIsRecordable;
}

bool CPVREpgInfoTag::IsPlayable(void) const
{
  bool bIsPlayable = false;
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (!client || (client->IsPlayable(shared_from_this(), bIsPlayable) != PVR_ERROR_NO_ERROR))
  {
    // fallback
    bIsPlayable = false;
  }
  return bIsPlayable;
}

void CPVREpgInfoTag::SetEpg(CPVREpg *epg)
{
  m_epg = epg;
  UpdatePath(); // Note: path contains epg id.
}

bool CPVREpgInfoTag::IsSeries(void) const
{
  if ((m_iFlags & EPG_TAG_FLAG_IS_SERIES) > 0 || SeriesNumber() > 0 || EpisodeNumber() > 0 || EpisodePart() > 0)
    return true;
  else
    return false;
}

const std::vector<std::string> CPVREpgInfoTag::Tokenize(const std::string &str)
{
  return StringUtils::Split(str.c_str(), EPG_STRING_TOKEN_SEPARATOR);
}

const std::string CPVREpgInfoTag::DeTokenize(const std::vector<std::string> &tokens)
{
  return StringUtils::Join(tokens, EPG_STRING_TOKEN_SEPARATOR);
}
