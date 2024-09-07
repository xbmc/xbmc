/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRMediaTag.h"

#include "ServiceBroker.h"
#include "cores/EdlEdit.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/media/PVRMediaPath.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;

using namespace std::chrono_literals;

CPVRMediaTagUid::CPVRMediaTagUid(int iClientId, const std::string& strMediaTagId)
  : m_iClientId(iClientId), m_strMediaTagId(strMediaTagId)
{
}

bool CPVRMediaTagUid::operator>(const CPVRMediaTagUid& right) const
{
  return (m_iClientId == right.m_iClientId) ? m_strMediaTagId > right.m_strMediaTagId
                                            : m_iClientId > right.m_iClientId;
}

bool CPVRMediaTagUid::operator<(const CPVRMediaTagUid& right) const
{
  return (m_iClientId == right.m_iClientId) ? m_strMediaTagId < right.m_strMediaTagId
                                            : m_iClientId < right.m_iClientId;
}

bool CPVRMediaTagUid::operator==(const CPVRMediaTagUid& right) const
{
  return m_iClientId == right.m_iClientId && m_strMediaTagId == right.m_strMediaTagId;
}

bool CPVRMediaTagUid::operator!=(const CPVRMediaTagUid& right) const
{
  return m_iClientId != right.m_iClientId || m_strMediaTagId != right.m_strMediaTagId;
}

const std::string CPVRMediaTag::IMAGE_OWNER_PATTERN = "pvrmediatag";

CPVRMediaTag::CPVRMediaTag()
  : m_iconPath(IMAGE_OWNER_PATTERN),
    m_thumbnailPath(IMAGE_OWNER_PATTERN),
    m_fanartPath(IMAGE_OWNER_PATTERN)
{
  Reset();
}

CPVRMediaTag::CPVRMediaTag(const PVR_MEDIA_TAG& mediaTag, unsigned int iClientId)
  : m_iconPath(mediaTag.strIconPath ? mediaTag.strIconPath : "", IMAGE_OWNER_PATTERN),
    m_thumbnailPath(mediaTag.strThumbnailPath ? mediaTag.strThumbnailPath : "",
                    IMAGE_OWNER_PATTERN),
    m_fanartPath(mediaTag.strFanartPath ? mediaTag.strFanartPath : "", IMAGE_OWNER_PATTERN)
{
  Reset();

  if (mediaTag.strMediaTagId)
    m_strMediaTagId = mediaTag.strMediaTagId;
  if (mediaTag.strTitle)
    m_strTitle = mediaTag.strTitle;
  if (mediaTag.strEpisodeName)
    m_strShowTitle = mediaTag.strEpisodeName;
  m_iSeason = mediaTag.iSeriesNumber;
  m_iEpisode = mediaTag.iEpisodeNumber;
  m_episodePartNumber = mediaTag.iEpisodePartNumber;
  if (mediaTag.iYear > 0)
    SetYear(mediaTag.iYear);
  m_iClientId = iClientId;
  m_mediaTagTime =
      mediaTag.mediaTagTime +
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  m_iPriority = mediaTag.iPriority;
  if (mediaTag.strDirectory)
    m_strDirectory = mediaTag.strDirectory;
  if (mediaTag.strPlot)
    m_strPlot = mediaTag.strPlot;
  if (mediaTag.strPlotOutline)
    m_strPlotOutline = mediaTag.strPlotOutline;
  if (mediaTag.strFirstAired && strlen(mediaTag.strFirstAired) > 0)
    m_firstAired.SetFromW3CDateTime(mediaTag.strFirstAired);
  m_iFlags = mediaTag.iFlags;
  if (mediaTag.sizeInBytes >= 0)
    m_sizeInBytes = mediaTag.sizeInBytes;
  if (mediaTag.strProviderName)
    m_strProviderName = mediaTag.strProviderName;
  m_iClientProviderUniqueId = mediaTag.iClientProviderUid;

  // Workaround for C++ PVR Add-on API wrapper not initializing this value correctly until API 9.0.1
  //! @todo Remove with next incompatible API bump.
  if (m_iClientProviderUniqueId == 0)
    m_iClientProviderUniqueId = PVR_PROVIDER_INVALID_UID;

  SetGenre(mediaTag.iGenreType, mediaTag.iGenreSubType,
           mediaTag.strGenreDescription ? mediaTag.strGenreDescription : "");
  CVideoInfoTag::SetPlayCount(mediaTag.iPlayCount);
  if (mediaTag.iLastPlayedPosition > 0 && mediaTag.iDuration > mediaTag.iLastPlayedPosition)
    CVideoInfoTag::SetResumePoint(mediaTag.iLastPlayedPosition, mediaTag.iDuration, "");
  SetDuration(mediaTag.iDuration);

  m_parentalRating = mediaTag.iParentalRating;
  if (mediaTag.strParentalRatingCode)
    m_parentalRatingCode = mediaTag.strParentalRatingCode;
  if (mediaTag.strParentalRatingIcon)
    m_parentalRatingIcon = mediaTag.strParentalRatingIcon;
  if (mediaTag.strParentalRatingSource)
    m_parentalRatingSource = mediaTag.strParentalRatingSource;

  //  As the channel a mediaTag was done on (probably long time ago) might no longer be
  //  available today prefer addon-supplied channel type (tv/radio) over channel attribute.
  if (mediaTag.sectionType != PVR_MEDIA_TAG_SECTION_TYPE_UNKNOWN)
  {
    m_bRadio = mediaTag.sectionType == PVR_MEDIA_TAG_SECTION_TYPE_RADIO;
  }
  else
  {
    if (mediaTag.mediaType != PVR_MEDIA_TAG_TYPE_UNKNOWN)
    {
      switch (mediaTag.mediaType)
      {
        case PVR_MEDIA_TAG_TYPE_MUSIC:
        case PVR_MEDIA_TAG_TYPE_RADIO_SHOW:
        case PVR_MEDIA_TAG_TYPE_PODCAST:
          m_bRadio = true;
          break;
        default:
          m_bRadio = false;
      }
    }
    else
    {
      const std::shared_ptr<const CPVRClient> client =
          CServiceBroker::GetPVRManager().GetClient(m_iClientId);
      bool bSupportsRadio = client && client->GetClientCapabilities().SupportsRadio();
      if (bSupportsRadio && client && client->GetClientCapabilities().SupportsTV())
      {
        CLog::Log(LOGWARNING, "Unable to determine section type. Defaulting to TV.");
        m_bRadio = false; // Assume TV.
      }
      else
      {
        m_bRadio = bSupportsRadio;
      }
    }
  }

  m_mediaType = mediaTag.mediaType;

  UpdatePath();
}

bool CPVRMediaTag::operator==(const CPVRMediaTag& right) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return (this == &right) ||
         (m_strMediaTagId == right.m_strMediaTagId && m_iClientId == right.m_iClientId &&
          m_mediaTagTime == right.m_mediaTagTime && GetDuration() == right.GetDuration() &&
          m_strPlotOutline == right.m_strPlotOutline && m_strPlot == right.m_strPlot &&
          m_iPriority == right.m_iPriority && m_strDirectory == right.m_strDirectory &&
          m_strFileNameAndPath == right.m_strFileNameAndPath && m_strTitle == right.m_strTitle &&
          m_strShowTitle == right.m_strShowTitle && m_iSeason == right.m_iSeason &&
          m_iEpisode == right.m_iEpisode && GetPremiered() == right.GetPremiered() &&
          m_iconPath == right.m_iconPath && m_thumbnailPath == right.m_thumbnailPath &&
          m_fanartPath == right.m_fanartPath && m_iMediaTagId == right.m_iMediaTagId &&
          m_bRadio == right.m_bRadio && m_mediaType == right.m_mediaType &&
          m_genre == right.m_genre && m_iGenreType == right.m_iGenreType &&
          m_iGenreSubType == right.m_iGenreSubType && m_firstAired == right.m_firstAired &&
          m_iFlags == right.m_iFlags && m_sizeInBytes == right.m_sizeInBytes &&
          m_strProviderName == right.m_strProviderName &&
          m_iClientProviderUniqueId == right.m_iClientProviderUniqueId &&
          m_parentalRating == right.m_parentalRating &&
          m_parentalRatingCode == right.m_parentalRatingCode &&
          m_parentalRatingIcon == right.m_parentalRatingIcon &&
          m_parentalRatingSource == right.m_parentalRatingSource &&
          m_episodePartNumber == right.m_episodePartNumber);
}

bool CPVRMediaTag::operator!=(const CPVRMediaTag& right) const
{
  return !(*this == right);
}

void CPVRMediaTag::Serialize(CVariant& value) const
{
  CVideoInfoTag::Serialize(value);

  value["directory"] = m_strDirectory;
  value["icon"] = ClientIconPath();
  value["starttime"] = m_mediaTagTime.IsValid() ? m_mediaTagTime.GetAsDBDateTime() : "";
  value["endtime"] = m_mediaTagTime.IsValid() ? EndTimeAsUTC().GetAsDBDateTime() : "";
  value["mediatagid"] = m_iMediaTagId;
  value["radio"] = m_bRadio;
  value["genre"] = m_genre;
  value["parentalrating"] = m_parentalRating;
  value["parentalratingcode"] = m_parentalRatingCode;
  value["parentalratingicon"] = m_parentalRatingIcon;
  value["parentalratingsource"] = m_parentalRatingSource;
  value["episodepart"] = m_episodePartNumber;

  if (!value.isMember("art"))
    value["art"] = CVariant(CVariant::VariantTypeObject);
  if (!ClientThumbnailPath().empty())
    value["art"]["thumb"] = ClientThumbnailPath();
  if (!ClientFanartPath().empty())
    value["art"]["fanart"] = ClientFanartPath();

  value["clientid"] = m_iClientId;
}

void CPVRMediaTag::ToSortable(SortItem& sortable, Field field) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (field == FieldSize)
    sortable[FieldSize] = m_sizeInBytes;
  else if (field == FieldProvider)
    sortable[FieldProvider] = StringUtils::Format("{} {}", m_iClientId, m_iClientProviderUniqueId);
  else
    CVideoInfoTag::ToSortable(sortable, field);
}

void CPVRMediaTag::Reset()
{
  m_strMediaTagId.clear();
  m_iClientId = -1;
  m_strDirectory.clear();
  m_iPriority = -1;
  m_strFileNameAndPath.clear();
  m_bGotMetaData = false;
  m_iMediaTagId = 0;
  m_iSeason = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
  m_iEpisode = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
  m_episodePartNumber = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
  m_bRadio = false;
  m_mediaType = PVR_MEDIA_TAG_TYPE_UNKNOWN;
  m_iFlags = PVR_MEDIA_TAG_FLAG_UNDEFINED;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_sizeInBytes = 0;
  }
  m_strProviderName.clear();
  m_iClientProviderUniqueId = PVR_PROVIDER_INVALID_UID;

  m_mediaTagTime.Reset();

  m_parentalRating = 0;
  m_parentalRatingCode.clear();
  m_parentalRatingIcon.clear();
  m_parentalRatingSource.clear();

  CVideoInfoTag::Reset();
}

bool CPVRMediaTag::SetPlayCount(int count)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaPlayCount())
  {
    if (client->SetMediaTagPlayCount(*this, count) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetPlayCount(count);
}

bool CPVRMediaTag::IncrementPlayCount()
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaPlayCount())
  {
    if (client->SetMediaTagPlayCount(*this, CVideoInfoTag::GetPlayCount() + 1) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::IncrementPlayCount();
}

bool CPVRMediaTag::SetResumePoint(const CBookmark& resumePoint)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaLastPlayedPosition())
  {
    if (client->SetMediaTagLastPlayedPosition(*this, lrint(resumePoint.timeInSeconds)) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(resumePoint);
}

bool CPVRMediaTag::SetResumePoint(double timeInSeconds,
                                  double totalTimeInSeconds,
                                  const std::string& playerState /* = "" */)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaLastPlayedPosition())
  {
    if (client->SetMediaTagLastPlayedPosition(*this, lrint(timeInSeconds)) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(timeInSeconds, totalTimeInSeconds, playerState);
}

CBookmark CPVRMediaTag::GetResumePoint() const
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaLastPlayedPosition() &&
      m_resumePointRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_resumePointRefetchTimeout.Set(10s); // update resume point from backend at most every 10 secs

    int pos = -1;
    client->GetMediaTagLastPlayedPosition(*this, pos);

    if (pos >= 0)
    {
      CBookmark resumePoint(CVideoInfoTag::GetResumePoint());
      resumePoint.timeInSeconds = pos;
      resumePoint.totalTimeInSeconds = (pos == 0) ? 0 : m_duration;
      CPVRMediaTag* pThis = const_cast<CPVRMediaTag*>(this);
      pThis->CVideoInfoTag::SetResumePoint(resumePoint);
    }
  }
  return CVideoInfoTag::GetResumePoint();
}

void CPVRMediaTag::UpdateMetadata(CVideoDatabase& db, const CPVRClient& client)
{
  if (m_bGotMetaData || !db.IsOpen())
    return;

  if (!client.GetClientCapabilities().SupportsMediaPlayCount())
    CVideoInfoTag::SetPlayCount(db.GetPlayCount(m_strFileNameAndPath));

  if (!client.GetClientCapabilities().SupportsMediaLastPlayedPosition())
  {
    CBookmark resumePoint;
    if (db.GetResumeBookMark(m_strFileNameAndPath, resumePoint))
      CVideoInfoTag::SetResumePoint(resumePoint);
  }

  m_lastPlayed = db.GetLastPlayed(m_strFileNameAndPath);

  m_bGotMetaData = true;
}

std::vector<EDL::Edit> CPVRMediaTag::GetEdl() const
{
  std::vector<EDL::Edit> edls;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsMediaEdl())
    client->GetMediaTagEdl(*this, edls);

  return edls;
}

void CPVRMediaTag::Update(const CPVRMediaTag& tag, const CPVRClient& client)
{
  m_strMediaTagId = tag.m_strMediaTagId;
  m_iClientId = tag.m_iClientId;
  m_strTitle = tag.m_strTitle;
  m_strShowTitle = tag.m_strShowTitle;
  m_iSeason = tag.m_iSeason;
  m_iEpisode = tag.m_iEpisode;
  m_episodePartNumber = tag.m_episodePartNumber;
  SetPremiered(tag.GetPremiered());
  m_mediaTagTime = tag.m_mediaTagTime;
  m_iPriority = tag.m_iPriority;
  m_strDirectory = tag.m_strDirectory;
  m_strPlot = tag.m_strPlot;
  m_strPlotOutline = tag.m_strPlotOutline;
  m_genre = tag.m_genre;
  m_parentalRating = tag.m_parentalRating;
  m_parentalRatingCode = tag.m_parentalRatingCode;
  m_parentalRatingIcon = tag.m_parentalRatingIcon;
  m_parentalRatingSource = tag.m_parentalRatingSource;
  m_iconPath = tag.m_iconPath;
  m_thumbnailPath = tag.m_thumbnailPath;
  m_fanartPath = tag.m_fanartPath;
  m_bRadio = tag.m_bRadio;
  m_firstAired = tag.m_firstAired;
  m_iFlags = tag.m_iFlags;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_sizeInBytes = tag.m_sizeInBytes;
    m_strProviderName = tag.m_strProviderName;
    m_iClientProviderUniqueId = tag.m_iClientProviderUniqueId;
  }

  if (client.GetClientCapabilities().SupportsMediaPlayCount())
    CVideoInfoTag::SetPlayCount(tag.GetLocalPlayCount());

  if (client.GetClientCapabilities().SupportsMediaLastPlayedPosition())
    CVideoInfoTag::SetResumePoint(tag.GetLocalResumePoint());

  SetDuration(tag.GetDuration());

  if (m_iGenreType == EPG_GENRE_USE_STRING || m_iGenreSubType == EPG_GENRE_USE_STRING)
  {
    /* No type/subtype. Use the provided description */
    m_genre = tag.m_genre;
  }
  else
  {
    /* Determine genre description by type/subtype */
    m_genre = StringUtils::Split(
        CPVREpg::ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType),
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }

  //Old Method of identifying TV show title and subtitle using m_strDirectory and strPlotOutline (deprecated)
  std::string strShow = StringUtils::Format("{} - ", g_localizeStrings.Get(20364));
  if (StringUtils::StartsWithNoCase(m_strPlotOutline, strShow))
  {
    CLog::Log(LOGWARNING, "PVR addon provides episode name in strPlotOutline which is deprecated");
    std::string strEpisode = m_strPlotOutline;
    std::string strTitle = m_strDirectory;

    size_t pos = strTitle.rfind('/');
    strTitle.erase(0, pos + 1);
    strEpisode.erase(0, strShow.size());
    m_strTitle = strTitle;
    pos = strEpisode.find('-');
    strEpisode.erase(0, pos + 2);
    m_strShowTitle = strEpisode;
  }

  UpdatePath();
}

void CPVRMediaTag::UpdatePath()
{
  m_strFileNameAndPath = CPVRMediaPath(m_bRadio, m_strDirectory, m_strTitle, m_iSeason, m_iEpisode,
                                       GetYear(), m_strShowTitle, m_mediaTagTime, m_strMediaTagId);
}

const CDateTime& CPVRMediaTag::MediaTagTimeAsLocalTime() const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_mediaTagTime);

  return tmp;
}

CDateTime CPVRMediaTag::EndTimeAsUTC() const
{
  unsigned int duration = GetDuration();
  return m_mediaTagTime + CDateTimeSpan(0, 0, duration / 60, duration % 60);
}

CDateTime CPVRMediaTag::EndTimeAsLocalTime() const
{
  CDateTime ret;
  ret.SetFromUTCDateTime(EndTimeAsUTC());
  return ret;
}

std::string CPVRMediaTag::GetTitleFromURL(const std::string& url)
{
  return CPVRMediaPath(url).GetTitle();
}

int CPVRMediaTag::ClientID() const
{
  return m_iClientId;
}

void CPVRMediaTag::SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre)
{
  m_iGenreType = iGenreType;
  m_iGenreSubType = iGenreSubType;

  if ((iGenreType == EPG_GENRE_USE_STRING || iGenreSubType == EPG_GENRE_USE_STRING) &&
      !strGenre.empty())
  {
    /* Type and sub type are not given. Use the provided genre description if available. */
    m_genre = StringUtils::Split(
        strGenre,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
  else
  {
    /* Determine the genre description from the type and subtype IDs */
    m_genre = StringUtils::Split(
        CPVREpg::ConvertGenreIdToString(iGenreType, iGenreSubType),
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
}

const std::string CPVRMediaTag::GetGenresLabel() const
{
  return StringUtils::Join(
      m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}

CDateTime CPVRMediaTag::FirstAired() const
{
  return m_firstAired;
}

void CPVRMediaTag::SetYear(int year)
{
  if (year > 0)
    m_premiered = CDateTime(year, 1, 1, 0, 0, 0);
}

int CPVRMediaTag::GetYear() const
{
  return m_premiered.IsValid() ? m_premiered.GetYear() : 0;
}

bool CPVRMediaTag::HasYear() const
{
  return m_premiered.IsValid();
}

bool CPVRMediaTag::IsNew() const
{
  return (m_iFlags & PVR_MEDIA_TAG_FLAG_IS_NEW) > 0;
}

bool CPVRMediaTag::IsPremiere() const
{
  return (m_iFlags & PVR_MEDIA_TAG_FLAG_IS_PREMIERE) > 0;
}

bool CPVRMediaTag::IsLive() const
{
  return (m_iFlags & PVR_MEDIA_TAG_FLAG_IS_LIVE) > 0;
}

bool CPVRMediaTag::IsFinale() const
{
  return (m_iFlags & PVR_MEDIA_TAG_FLAG_IS_FINALE) > 0;
}

int64_t CPVRMediaTag::GetSizeInBytes() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_sizeInBytes;
}

int CPVRMediaTag::ClientProviderUniqueId() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iClientProviderUniqueId;
}

std::string CPVRMediaTag::ProviderName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strProviderName;
}

std::shared_ptr<CPVRProvider> CPVRMediaTag::GetDefaultProvider() const
{
  return CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId,
                                                                  PVR_PROVIDER_INVALID_UID);
}

bool CPVRMediaTag::HasClientProvider() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iClientProviderUniqueId != PVR_PROVIDER_INVALID_UID;
}

std::shared_ptr<CPVRProvider> CPVRMediaTag::GetProvider() const
{
  auto provider = CServiceBroker::GetPVRManager().Providers()->GetByClient(
      m_iClientId, m_iClientProviderUniqueId);

  if (!provider)
    provider = GetDefaultProvider();

  return provider;
}

unsigned int CPVRMediaTag::GetParentalRating() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_parentalRating;
}

const std::string& CPVRMediaTag::GetParentalRatingCode() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_parentalRatingCode;
}

const std::string& CPVRMediaTag::GetParentalRatingIcon() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_parentalRatingIcon;
}

const std::string& CPVRMediaTag::GetParentalRatingSource() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_parentalRatingSource;
}

int CPVRMediaTag::EpisodePart() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_episodePartNumber;
}
