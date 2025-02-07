/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgInfoTag.h"

#include "ServiceBroker.h"
#include "cores/EdlEdit.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgDatabase.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;

const std::string CPVREpgInfoTag::IMAGE_OWNER_PATTERN = "epgtag_{}";

CPVREpgInfoTag::CPVREpgInfoTag(int iEpgID,
                               const std::string& iconPath,
                               const std::string& parentalRatingIconPath)
  : m_parentalRatingIcon(parentalRatingIconPath, StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_iUniqueBroadcastID(EPG_TAG_INVALID_UID),
    m_iconPath(iconPath, StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_iFlags(EPG_TAG_FLAG_UNDEFINED),
    m_channelData(new CPVREpgChannelData),
    m_iEpgID(iEpgID)
{
}

CPVREpgInfoTag::CPVREpgInfoTag(const std::shared_ptr<CPVREpgChannelData>& channelData,
                               int iEpgID,
                               const CDateTime& start,
                               const CDateTime& end,
                               bool bIsGapTag)
  : m_parentalRatingIcon(StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_iUniqueBroadcastID(EPG_TAG_INVALID_UID),
    m_iconPath(StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_iFlags(EPG_TAG_FLAG_UNDEFINED),
    m_bIsGapTag(bIsGapTag),
    m_iEpgID(iEpgID)
{
  if (channelData)
    m_channelData = channelData;
  else
    m_channelData = std::make_shared<CPVREpgChannelData>();

  const CDateTimeSpan correction(
      0, 0, 0, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection);
  m_startTime = start + correction;
  m_endTime = end + correction;
}

CPVREpgInfoTag::CPVREpgInfoTag(const EPG_TAG& data,
                               int iClientId,
                               const std::shared_ptr<CPVREpgChannelData>& channelData,
                               int iEpgID)
  : m_iGenreType(data.iGenreType),
    m_iGenreSubType(data.iGenreSubType),
    m_parentalRating(data.iParentalRating),
    m_parentalRatingIcon(data.strParentalRatingIcon ? data.strParentalRatingIcon : "",
                         StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_iStarRating(data.iStarRating),
    m_iSeriesNumber(data.iSeriesNumber),
    m_iEpisodeNumber(data.iEpisodeNumber),
    m_iEpisodePart(data.iEpisodePartNumber),
    m_iUniqueBroadcastID(data.iUniqueBroadcastId),
    m_iYear(data.iYear),
    m_iconPath(data.strIconPath ? data.strIconPath : "",
               StringUtils::Format(IMAGE_OWNER_PATTERN, iEpgID)),
    m_startTime(
        data.startTime +
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection),
    m_endTime(data.endTime +
              CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection),
    m_iFlags(data.iFlags),
    m_iEpgID(iEpgID)
{
  // strFirstAired is optional, so check if supported before assigning it
  if (data.strFirstAired && strlen(data.strFirstAired) > 0)
    m_firstAired.SetFromW3CDate(data.strFirstAired);

  if (channelData)
  {
    m_channelData = channelData;

    if (m_channelData->ClientId() != iClientId)
      CLog::LogF(LOGERROR, "Client id mismatch (channel: {}, epg: {})!", m_channelData->ClientId(),
                 iClientId);
    if (m_channelData->UniqueClientChannelId() != static_cast<int>(data.iUniqueChannelId))
      CLog::LogF(LOGERROR, "Channel uid mismatch (channel: {}, epg: {})!",
                 m_channelData->UniqueClientChannelId(), data.iUniqueChannelId);
  }
  else
  {
    // provide minimalistic channel data until we get fully initialized later
    m_channelData = std::make_shared<CPVREpgChannelData>(iClientId, data.iUniqueChannelId);
  }

  // explicit NULL check, because there is no implicit NULL constructor for std::string
  if (data.strTitle)
    m_strTitle = data.strTitle;
  if (data.strTitleExtraInfo)
    m_titleExtraInfo = data.strTitleExtraInfo;
  if (data.strGenreDescription)
    m_strGenreDescription = data.strGenreDescription;
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
  if (data.strSeriesLink)
    m_strSeriesLink = data.strSeriesLink;
  if (data.strParentalRatingCode)
    m_parentalRatingCode = data.strParentalRatingCode;
  if (data.strParentalRatingSource)
    m_parentalRatingSource = data.strParentalRatingSource;
}

void CPVREpgInfoTag::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (data)
    m_channelData = data;
  else
    m_channelData = std::make_shared<CPVREpgChannelData>();
}

bool CPVREpgInfoTag::operator==(const CPVREpgInfoTag& right) const
{
  if (this == &right)
    return true;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return (m_iUniqueBroadcastID == right.m_iUniqueBroadcastID && m_channelData &&
          right.m_channelData &&
          m_channelData->UniqueClientChannelId() == right.m_channelData->UniqueClientChannelId() &&
          m_channelData->ClientId() == right.m_channelData->ClientId());
}

bool CPVREpgInfoTag::operator!=(const CPVREpgInfoTag& right) const
{
  if (this == &right)
    return false;

  return !(*this == right);
}

void CPVREpgInfoTag::Serialize(CVariant& value) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  value["broadcastid"] = m_iDatabaseID; // Use DB id here as it is unique across PVR clients
  value["channeluid"] = m_channelData->UniqueClientChannelId();
  value["parentalrating"] = m_parentalRating;
  value["parentalratingcode"] = m_parentalRatingCode;
  value["parentalratingicon"] = ClientParentalRatingIconPath();
  value["parentalratingsource"] = m_parentalRatingSource;
  value["rating"] = m_iStarRating;
  value["title"] = m_strTitle;
  value["titleextrainfo"] = m_titleExtraInfo;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["originaltitle"] = m_strOriginalTitle;
  value["thumbnail"] = ClientIconPath();
  value["cast"] = DeTokenize(m_cast);
  value["director"] = DeTokenize(m_directors);
  value["writer"] = DeTokenize(m_writers);
  value["year"] = m_iYear;
  value["imdbnumber"] = m_strIMDBNumber;
  value["genre"] = Genre();
  value["filenameandpath"] = Path();
  value["starttime"] = m_startTime.IsValid() ? m_startTime.GetAsDBDateTime() : StringUtils::Empty;
  value["endtime"] = m_endTime.IsValid() ? m_endTime.GetAsDBDateTime() : StringUtils::Empty;
  value["runtime"] = GetDuration() / 60;
  value["firstaired"] = m_firstAired.IsValid() ? m_firstAired.GetAsDBDate() : StringUtils::Empty;
  value["progress"] = Progress();
  value["progresspercentage"] = ProgressPercentage();
  value["episodename"] = m_strEpisodeName;
  value["episode"] = m_iEpisodeNumber;
  value["episodenum"] = m_iEpisodeNumber;
  value["episodepart"] = m_iEpisodePart;
  value["season"] = m_iSeriesNumber;
  value["seasonnum"] = m_iSeriesNumber;
  value["isactive"] = IsActive();
  value["wasactive"] = WasActive();
  value["isseries"] = IsSeries();
  value["serieslink"] = m_strSeriesLink;
  value["clientid"] = m_channelData->ClientId();
}

int CPVREpgInfoTag::ClientID() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelData->ClientId();
}

CDateTime CPVREpgInfoTag::GetCurrentPlayingTime() const
{
  return CServiceBroker::GetPVRManager().PlaybackState()->GetChannelPlaybackTime(ClientID(),
                                                                                 UniqueChannelID());
}

bool CPVREpgInfoTag::IsActive() const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_startTime <= now && m_endTime > now);
}

bool CPVREpgInfoTag::WasActive() const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_endTime < now);
}

bool CPVREpgInfoTag::IsUpcoming() const
{
  CDateTime now = GetCurrentPlayingTime();
  return (m_startTime > now);
}

double CPVREpgInfoTag::ProgressPercentage() const
{
  double ret = 0.0;

  time_t currentTime, startTime, endTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);
  m_startTime.GetAsTime(startTime);
  m_endTime.GetAsTime(endTime);

  if (currentTime >= startTime && currentTime <= endTime)
  {
    const std::chrono::duration<double> total{endTime - startTime > 0 ? endTime - startTime
                                                                      : 3600.0};
    if (total.count())
    {
      const std::chrono::duration<double> current{currentTime - startTime};
      ret = current.count() * 100.0 / total.count();
    }
  }
  else if (currentTime > endTime)
  {
    ret = 100.0;
  }
  return ret;
}

unsigned int CPVREpgInfoTag::Progress() const
{
  time_t currentTime, startTime;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(currentTime);
  m_startTime.GetAsTime(startTime);

  if (currentTime > startTime)
  {
    const std::chrono::duration<unsigned int> duration{currentTime - startTime};
    return duration.count();
  }
  else
  {
    return 0;
  }
}

void CPVREpgInfoTag::SetUniqueBroadcastID(unsigned int iUniqueBroadcastID)
{
  m_iUniqueBroadcastID = iUniqueBroadcastID;
}

unsigned int CPVREpgInfoTag::UniqueBroadcastID() const
{
  return m_iUniqueBroadcastID;
}

int CPVREpgInfoTag::DatabaseID() const
{
  return m_iDatabaseID;
}

int CPVREpgInfoTag::UniqueChannelID() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelData->UniqueClientChannelId();
}

std::string CPVREpgInfoTag::ChannelIconPath() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelData->ChannelIconPath();
}

CDateTime CPVREpgInfoTag::StartAsUTC() const
{
  return m_startTime;
}

CDateTime CPVREpgInfoTag::StartAsLocalTime() const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_startTime);
  return retVal;
}

CDateTime CPVREpgInfoTag::EndAsUTC() const
{
  return m_endTime;
}

CDateTime CPVREpgInfoTag::EndAsLocalTime() const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_endTime);
  return retVal;
}

void CPVREpgInfoTag::SetEndFromUTC(const CDateTime& end)
{
  m_endTime = end;
}

unsigned int CPVREpgInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);

  if (end > start)
  {
    const std::chrono::duration<unsigned int> duration{end - start};
    return duration.count();
  }
  else
  {
    return 3600;
  }
}

const std::string CPVREpgInfoTag::GetCastLabel(const std::string& separator) const
{
  // Note: see CVideoInfoTag::GetCast for reference implementation.
  const std::string sep{separator.empty() ? "\n" : separator};
  return StringUtils::Join(m_cast, sep);
}

const std::string CPVREpgInfoTag::GetDirectorsLabel(const std::string& separator) const
{
  const std::string sep{
      separator.empty()
          ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator
          : separator};
  return StringUtils::Join(m_directors, sep);
}

const std::string CPVREpgInfoTag::GetWritersLabel(const std::string& separator) const
{
  const std::string sep{
      separator.empty()
          ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator
          : separator};
  return StringUtils::Join(m_writers, sep);
}

const std::string CPVREpgInfoTag::GetGenresLabel(const std::string& separator) const
{
  const std::string sep{
      separator.empty()
          ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator
          : separator};
  return StringUtils::Join(Genre(), sep);
}

int CPVREpgInfoTag::Year() const
{
  return m_iYear;
}

int CPVREpgInfoTag::GenreType() const
{
  return m_iGenreType;
}

int CPVREpgInfoTag::GenreSubType() const
{
  return m_iGenreSubType;
}

const std::vector<std::string> CPVREpgInfoTag::Genre() const
{
  if (m_genre.empty())
  {
    if ((m_iGenreType == EPG_GENRE_USE_STRING || m_iGenreSubType == EPG_GENRE_USE_STRING) &&
        !m_strGenreDescription.empty())
    {
      // Type and sub type are both not given. No EPG color coding possible unless sub type is
      // used to specify EPG_GENRE_USE_STRING leaving type available for genre category, use the
      // provided genre description for the text.
      m_genre = Tokenize(m_strGenreDescription);
    }

    if (m_genre.empty())
    {
      // Determine the genre from the type and subtype IDs.
      m_genre = Tokenize(CPVREpg::ConvertGenreIdToString(m_iGenreType, m_iGenreSubType));
    }
  }
  return m_genre;
}

CDateTime CPVREpgInfoTag::FirstAired() const
{
  return m_firstAired;
}

unsigned int CPVREpgInfoTag::ParentalRating() const
{
  return m_parentalRating;
}

int CPVREpgInfoTag::StarRating() const
{
  return m_iStarRating;
}

int CPVREpgInfoTag::SeriesNumber() const
{
  return m_iSeriesNumber;
}

int CPVREpgInfoTag::EpisodeNumber() const
{
  return m_iEpisodeNumber;
}

int CPVREpgInfoTag::EpisodePart() const
{
  return m_iEpisodePart;
}

std::string CPVREpgInfoTag::IconPath() const
{
  return m_iconPath.GetLocalImage();
}

std::string CPVREpgInfoTag::ClientIconPath() const
{
  return m_iconPath.GetClientImage();
}

std::string CPVREpgInfoTag::Path() const
{
  return StringUtils::Format("pvr://guide/{:04}/{}.epg", EpgID(), m_startTime.GetAsDBDateTime());
}

bool CPVREpgInfoTag::Update(const CPVREpgInfoTag& tag, bool bUpdateBroadcastId /* = true */)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  bool bChanged =
      (m_strTitle != tag.m_strTitle || m_strPlotOutline != tag.m_strPlotOutline ||
       m_strPlot != tag.m_strPlot || m_strOriginalTitle != tag.m_strOriginalTitle ||
       m_cast != tag.m_cast || m_directors != tag.m_directors || m_writers != tag.m_writers ||
       m_iYear != tag.m_iYear || m_strIMDBNumber != tag.m_strIMDBNumber ||
       m_startTime != tag.m_startTime || m_endTime != tag.m_endTime ||
       m_iGenreType != tag.m_iGenreType || m_iGenreSubType != tag.m_iGenreSubType ||
       m_strGenreDescription != tag.m_strGenreDescription || m_firstAired != tag.m_firstAired ||
       m_parentalRating != tag.m_parentalRating ||
       m_parentalRatingCode != tag.m_parentalRatingCode ||
       m_parentalRatingIcon != tag.m_parentalRatingIcon ||
       m_parentalRatingSource != tag.m_parentalRatingSource || m_iStarRating != tag.m_iStarRating ||
       m_iEpisodeNumber != tag.m_iEpisodeNumber || m_iEpisodePart != tag.m_iEpisodePart ||
       m_iSeriesNumber != tag.m_iSeriesNumber || m_strEpisodeName != tag.m_strEpisodeName ||
       m_iUniqueBroadcastID != tag.m_iUniqueBroadcastID || m_iEpgID != tag.m_iEpgID ||
       m_genre != tag.m_genre || m_iconPath != tag.m_iconPath || m_iFlags != tag.m_iFlags ||
       m_strSeriesLink != tag.m_strSeriesLink || m_channelData != tag.m_channelData ||
       m_titleExtraInfo != tag.m_titleExtraInfo);

  if (bUpdateBroadcastId)
    bChanged |= (m_iDatabaseID != tag.m_iDatabaseID);

  if (bChanged)
  {
    if (bUpdateBroadcastId)
      m_iDatabaseID = tag.m_iDatabaseID;

    m_strTitle = tag.m_strTitle;
    m_titleExtraInfo = tag.m_titleExtraInfo;
    m_strPlotOutline = tag.m_strPlotOutline;
    m_strPlot = tag.m_strPlot;
    m_strOriginalTitle = tag.m_strOriginalTitle;
    m_cast = tag.m_cast;
    m_directors = tag.m_directors;
    m_writers = tag.m_writers;
    m_iYear = tag.m_iYear;
    m_strIMDBNumber = tag.m_strIMDBNumber;
    m_startTime = tag.m_startTime;
    m_endTime = tag.m_endTime;
    m_iGenreType = tag.m_iGenreType;
    m_iGenreSubType = tag.m_iGenreSubType;
    m_strGenreDescription = tag.m_strGenreDescription;
    m_genre = tag.m_genre;
    m_iEpgID = tag.m_iEpgID;
    m_iFlags = tag.m_iFlags;
    m_strSeriesLink = tag.m_strSeriesLink;
    m_firstAired = tag.m_firstAired;
    m_parentalRating = tag.m_parentalRating;
    m_parentalRatingCode = tag.m_parentalRatingCode;
    m_parentalRatingIcon = tag.m_parentalRatingIcon;
    m_parentalRatingSource = tag.m_parentalRatingSource;
    m_iStarRating = tag.m_iStarRating;
    m_iEpisodeNumber = tag.m_iEpisodeNumber;
    m_iEpisodePart = tag.m_iEpisodePart;
    m_iSeriesNumber = tag.m_iSeriesNumber;
    m_strEpisodeName = tag.m_strEpisodeName;
    m_iUniqueBroadcastID = tag.m_iUniqueBroadcastID;
    m_iconPath = tag.m_iconPath;
    m_channelData = tag.m_channelData;
  }

  return bChanged;
}

bool CPVREpgInfoTag::QueuePersistQuery(const std::shared_ptr<CPVREpgDatabase>& database)
{
  if (!database)
  {
    CLog::LogF(LOGERROR, "Could not open the EPG database");
    return false;
  }

  return database->QueuePersistQuery(*this);
}

std::vector<EDL::Edit> CPVREpgInfoTag::GetEdl() const
{
  std::vector<EDL::Edit> edls;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_channelData->ClientId());

  if (client && client->GetClientCapabilities().SupportsEpgTagEdl())
    client->GetEpgTagEdl(shared_from_this(), edls);

  return edls;
}

int CPVREpgInfoTag::EpgID() const
{
  return m_iEpgID;
}

void CPVREpgInfoTag::SetEpgID(int iEpgID)
{
  m_iEpgID = iEpgID;
  m_iconPath.SetOwner(StringUtils::Format(IMAGE_OWNER_PATTERN, m_iEpgID));
  m_parentalRatingIcon.SetOwner(StringUtils::Format(IMAGE_OWNER_PATTERN, m_iEpgID));
}

bool CPVREpgInfoTag::IsRecordable() const
{
  bool bIsRecordable = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_channelData->ClientId());
  if (!client || (client->IsRecordable(shared_from_this(), bIsRecordable) != PVR_ERROR_NO_ERROR))
  {
    // event end time based fallback
    bIsRecordable = EndAsLocalTime() > CDateTime::GetCurrentDateTime();
  }
  return bIsRecordable;
}

bool CPVREpgInfoTag::IsPlayable() const
{
  bool bIsPlayable = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_channelData->ClientId());
  if (!client || (client->IsPlayable(shared_from_this(), bIsPlayable) != PVR_ERROR_NO_ERROR))
  {
    // fallback
    bIsPlayable = false;
  }
  return bIsPlayable;
}

bool CPVREpgInfoTag::IsSeries() const
{
  if ((m_iFlags & EPG_TAG_FLAG_IS_SERIES) > 0 || SeriesNumber() >= 0 || EpisodeNumber() >= 0 ||
      EpisodePart() >= 0)
    return true;
  else
    return false;
}

bool CPVREpgInfoTag::IsRadio() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelData->IsRadio();
}

bool CPVREpgInfoTag::IsParentalLocked() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelData->IsLocked();
}

bool CPVREpgInfoTag::IsGapTag() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bIsGapTag;
}

bool CPVREpgInfoTag::IsNew() const
{
  return (m_iFlags & EPG_TAG_FLAG_IS_NEW) > 0;
}

bool CPVREpgInfoTag::IsPremiere() const
{
  return (m_iFlags & EPG_TAG_FLAG_IS_PREMIERE) > 0;
}

bool CPVREpgInfoTag::IsFinale() const
{
  return (m_iFlags & EPG_TAG_FLAG_IS_FINALE) > 0;
}

bool CPVREpgInfoTag::IsLive() const
{
  return (m_iFlags & EPG_TAG_FLAG_IS_LIVE) > 0;
}

const std::vector<std::string> CPVREpgInfoTag::Tokenize(const std::string& str)
{
  return StringUtils::Split(str, EPG_STRING_TOKEN_SEPARATOR);
}

const std::string CPVREpgInfoTag::DeTokenize(const std::vector<std::string>& tokens)
{
  return StringUtils::Join(tokens, EPG_STRING_TOKEN_SEPARATOR);
}
