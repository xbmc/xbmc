/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecording.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_recordings.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/recordings/PVRRecordingsPath.h"
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

CPVRRecordingUid::CPVRRecordingUid(int iClientId, const std::string& strRecordingId)
  : m_iClientId(iClientId), m_strRecordingId(strRecordingId)
{
}

bool CPVRRecordingUid::operator>(const CPVRRecordingUid& right) const
{
  return (m_iClientId == right.m_iClientId) ? m_strRecordingId > right.m_strRecordingId
                                            : m_iClientId > right.m_iClientId;
}

bool CPVRRecordingUid::operator<(const CPVRRecordingUid& right) const
{
  return (m_iClientId == right.m_iClientId) ? m_strRecordingId < right.m_strRecordingId
                                            : m_iClientId < right.m_iClientId;
}

bool CPVRRecordingUid::operator==(const CPVRRecordingUid& right) const
{
  return m_iClientId == right.m_iClientId && m_strRecordingId == right.m_strRecordingId;
}

bool CPVRRecordingUid::operator!=(const CPVRRecordingUid& right) const
{
  return m_iClientId != right.m_iClientId || m_strRecordingId != right.m_strRecordingId;
}

const std::string CPVRRecording::IMAGE_OWNER_PATTERN = "pvrrecording";

CPVRRecording::CPVRRecording()
  : m_iconPath(IMAGE_OWNER_PATTERN),
    m_thumbnailPath(IMAGE_OWNER_PATTERN),
    m_fanartPath(IMAGE_OWNER_PATTERN)
{
  Reset();
}

CPVRRecording::CPVRRecording(const PVR_RECORDING& recording, unsigned int iClientId)
  : m_iconPath(recording.strIconPath, IMAGE_OWNER_PATTERN),
    m_thumbnailPath(recording.strThumbnailPath, IMAGE_OWNER_PATTERN),
    m_fanartPath(recording.strFanartPath, IMAGE_OWNER_PATTERN)
{
  Reset();

  m_strRecordingId = recording.strRecordingId;
  m_strTitle = recording.strTitle;
  m_strShowTitle = recording.strEpisodeName;
  m_iSeason = recording.iSeriesNumber;
  m_iEpisode = recording.iEpisodeNumber;
  if (recording.iYear > 0)
    SetYear(recording.iYear);
  m_iClientId = iClientId;
  m_recordingTime =
      recording.recordingTime +
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  m_iPriority = recording.iPriority;
  m_iLifetime = recording.iLifetime;
  // Deleted recording is placed at the root of the deleted view
  m_strDirectory = recording.bIsDeleted ? "" : recording.strDirectory;
  m_strPlot = recording.strPlot;
  m_strPlotOutline = recording.strPlotOutline;
  m_strChannelName = recording.strChannelName;
  m_bIsDeleted = recording.bIsDeleted;
  m_iEpgEventId = recording.iEpgEventId;
  m_iChannelUid = recording.iChannelUid;
  if (strlen(recording.strFirstAired) > 0)
    m_firstAired.SetFromW3CDateTime(recording.strFirstAired);
  m_iFlags = recording.iFlags;
  if (recording.sizeInBytes >= 0)
    m_sizeInBytes = recording.sizeInBytes;
  m_strProviderName = recording.strProviderName;
  m_iClientProviderUniqueId = recording.iClientProviderUid;

  SetGenre(recording.iGenreType, recording.iGenreSubType, recording.strGenreDescription);
  CVideoInfoTag::SetPlayCount(recording.iPlayCount);
  if (recording.iLastPlayedPosition > 0 && recording.iDuration > recording.iLastPlayedPosition)
    CVideoInfoTag::SetResumePoint(recording.iLastPlayedPosition, recording.iDuration, "");
  SetDuration(recording.iDuration);

  //  As the channel a recording was done on (probably long time ago) might no longer be
  //  available today prefer addon-supplied channel type (tv/radio) over channel attribute.
  if (recording.channelType != PVR_RECORDING_CHANNEL_TYPE_UNKNOWN)
  {
    m_bRadio = recording.channelType == PVR_RECORDING_CHANNEL_TYPE_RADIO;
  }
  else
  {
    const std::shared_ptr<const CPVRChannel> channel(Channel());
    if (channel)
    {
      m_bRadio = channel->IsRadio();
    }
    else
    {
      const std::shared_ptr<const CPVRClient> client =
          CServiceBroker::GetPVRManager().GetClient(m_iClientId);
      bool bSupportsRadio = client && client->GetClientCapabilities().SupportsRadio();
      if (bSupportsRadio && client && client->GetClientCapabilities().SupportsTV())
      {
        CLog::Log(LOGWARNING, "Unable to determine channel type. Defaulting to TV.");
        m_bRadio = false; // Assume TV.
      }
      else
      {
        m_bRadio = bSupportsRadio;
      }
    }
  }

  UpdatePath();
}

bool CPVRRecording::operator==(const CPVRRecording& right) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return (this == &right) ||
         (m_strRecordingId == right.m_strRecordingId && m_iClientId == right.m_iClientId &&
          m_strChannelName == right.m_strChannelName && m_recordingTime == right.m_recordingTime &&
          GetDuration() == right.GetDuration() && m_strPlotOutline == right.m_strPlotOutline &&
          m_strPlot == right.m_strPlot && m_iPriority == right.m_iPriority &&
          m_iLifetime == right.m_iLifetime && m_strDirectory == right.m_strDirectory &&
          m_strFileNameAndPath == right.m_strFileNameAndPath && m_strTitle == right.m_strTitle &&
          m_strShowTitle == right.m_strShowTitle && m_iSeason == right.m_iSeason &&
          m_iEpisode == right.m_iEpisode && GetPremiered() == right.GetPremiered() &&
          m_iconPath == right.m_iconPath && m_thumbnailPath == right.m_thumbnailPath &&
          m_fanartPath == right.m_fanartPath && m_iRecordingId == right.m_iRecordingId &&
          m_bIsDeleted == right.m_bIsDeleted && m_iEpgEventId == right.m_iEpgEventId &&
          m_iChannelUid == right.m_iChannelUid && m_bRadio == right.m_bRadio &&
          m_genre == right.m_genre && m_iGenreType == right.m_iGenreType &&
          m_iGenreSubType == right.m_iGenreSubType && m_firstAired == right.m_firstAired &&
          m_iFlags == right.m_iFlags && m_sizeInBytes == right.m_sizeInBytes &&
          m_strProviderName == right.m_strProviderName &&
          m_iClientProviderUniqueId == right.m_iClientProviderUniqueId);
}

bool CPVRRecording::operator!=(const CPVRRecording& right) const
{
  return !(*this == right);
}

void CPVRRecording::FillAddonData(PVR_RECORDING& recording) const
{
  time_t recTime;
  RecordingTimeAsUTC().GetAsTime(recTime);

  recording = {};
  strncpy(recording.strRecordingId, ClientRecordingID().c_str(),
          sizeof(recording.strRecordingId) - 1);
  strncpy(recording.strTitle, m_strTitle.c_str(), sizeof(recording.strTitle) - 1);
  strncpy(recording.strEpisodeName, m_strShowTitle.c_str(), sizeof(recording.strEpisodeName) - 1);
  recording.iSeriesNumber = m_iSeason;
  recording.iEpisodeNumber = m_iEpisode;
  recording.iYear = GetYear();
  strncpy(recording.strDirectory, Directory().c_str(), sizeof(recording.strDirectory) - 1);
  strncpy(recording.strPlotOutline, m_strPlotOutline.c_str(), sizeof(recording.strPlotOutline) - 1);
  strncpy(recording.strPlot, m_strPlot.c_str(), sizeof(recording.strPlot) - 1);
  strncpy(recording.strGenreDescription, GetGenresLabel().c_str(),
          sizeof(recording.strGenreDescription) - 1);
  strncpy(recording.strChannelName, ChannelName().c_str(), sizeof(recording.strChannelName) - 1);
  strncpy(recording.strIconPath, ClientIconPath().c_str(), sizeof(recording.strIconPath) - 1);
  strncpy(recording.strThumbnailPath, ClientThumbnailPath().c_str(),
          sizeof(recording.strThumbnailPath) - 1);
  strncpy(recording.strFanartPath, ClientFanartPath().c_str(), sizeof(recording.strFanartPath) - 1);
  recording.recordingTime =
      recTime - CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  recording.iDuration = GetDuration();
  recording.iPriority = Priority();
  recording.iLifetime = LifeTime();
  recording.iGenreType = GenreType();
  recording.iGenreSubType = GenreSubType();
  recording.iPlayCount = GetLocalPlayCount();
  recording.iLastPlayedPosition = std::lrint(GetLocalResumePoint().timeInSeconds);
  recording.bIsDeleted = IsDeleted();
  recording.iEpgEventId = m_iEpgEventId;
  recording.iChannelUid = ChannelUid();
  recording.channelType =
      IsRadio() ? PVR_RECORDING_CHANNEL_TYPE_RADIO : PVR_RECORDING_CHANNEL_TYPE_TV;
  if (FirstAired().IsValid())
    strncpy(recording.strFirstAired, FirstAired().GetAsW3CDate().c_str(),
            sizeof(recording.strFirstAired) - 1);
  recording.iFlags = Flags();
  recording.sizeInBytes = GetSizeInBytes();
  strncpy(recording.strProviderName, ProviderName().c_str(), sizeof(recording.strProviderName) - 1);
  recording.iClientProviderUid = ClientProviderUniqueId();
}

void CPVRRecording::Serialize(CVariant& value) const
{
  CVideoInfoTag::Serialize(value);

  value["channel"] = m_strChannelName;
  value["lifetime"] = m_iLifetime;
  value["directory"] = m_strDirectory;
  value["icon"] = ClientIconPath();
  value["starttime"] = m_recordingTime.IsValid() ? m_recordingTime.GetAsDBDateTime() : "";
  value["endtime"] = m_recordingTime.IsValid() ? EndTimeAsUTC().GetAsDBDateTime() : "";
  value["recordingid"] = m_iRecordingId;
  value["isdeleted"] = m_bIsDeleted;
  value["epgeventid"] = m_iEpgEventId;
  value["channeluid"] = m_iChannelUid;
  value["radio"] = m_bRadio;
  value["genre"] = m_genre;

  if (!value.isMember("art"))
    value["art"] = CVariant(CVariant::VariantTypeObject);
  if (!ClientThumbnailPath().empty())
    value["art"]["thumb"] = ClientThumbnailPath();
  if (!ClientFanartPath().empty())
    value["art"]["fanart"] = ClientFanartPath();

  value["clientid"] = m_iClientId;
}

void CPVRRecording::ToSortable(SortItem& sortable, Field field) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (field == FieldSize)
    sortable[FieldSize] = m_sizeInBytes;
  else if (field == FieldProvider)
    sortable[FieldProvider] = StringUtils::Format("{} {}", m_iClientId, m_iClientProviderUniqueId);
  else
    CVideoInfoTag::ToSortable(sortable, field);
}

void CPVRRecording::Reset()
{
  m_strRecordingId.clear();
  m_iClientId = -1;
  m_strChannelName.clear();
  m_strDirectory.clear();
  m_iPriority = -1;
  m_iLifetime = -1;
  m_strFileNameAndPath.clear();
  m_bGotMetaData = false;
  m_iRecordingId = 0;
  m_bIsDeleted = false;
  m_bInProgress = true;
  m_iEpgEventId = EPG_TAG_INVALID_UID;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iChannelUid = PVR_CHANNEL_INVALID_UID;
  m_bRadio = false;
  m_iFlags = PVR_RECORDING_FLAG_UNDEFINED;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_sizeInBytes = 0;
  }
  m_strProviderName.clear();
  m_iClientProviderUniqueId = PVR_PROVIDER_INVALID_UID;

  m_recordingTime.Reset();
  CVideoInfoTag::Reset();
}

bool CPVRRecording::Delete()
{
  std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->DeleteRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::Undelete()
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->UndeleteRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::Rename(const std::string& strNewName)
{
  m_strTitle = strNewName;
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->RenameRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::SetPlayCount(int count)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsPlayCount())
  {
    if (client->SetRecordingPlayCount(*this, count) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetPlayCount(count);
}

bool CPVRRecording::IncrementPlayCount()
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsPlayCount())
  {
    if (client->SetRecordingPlayCount(*this, CVideoInfoTag::GetPlayCount() + 1) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::IncrementPlayCount();
}

bool CPVRRecording::SetResumePoint(const CBookmark& resumePoint)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    if (client->SetRecordingLastPlayedPosition(*this, lrint(resumePoint.timeInSeconds)) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(resumePoint);
}

bool CPVRRecording::SetResumePoint(double timeInSeconds,
                                   double totalTimeInSeconds,
                                   const std::string& playerState /* = "" */)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    if (client->SetRecordingLastPlayedPosition(*this, lrint(timeInSeconds)) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(timeInSeconds, totalTimeInSeconds, playerState);
}

CBookmark CPVRRecording::GetResumePoint() const
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition() &&
      m_resumePointRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_resumePointRefetchTimeout.Set(10s); // update resume point from backend at most every 10 secs

    int pos = -1;
    client->GetRecordingLastPlayedPosition(*this, pos);

    if (pos >= 0)
    {
      CBookmark resumePoint(CVideoInfoTag::GetResumePoint());
      resumePoint.timeInSeconds = pos;
      resumePoint.totalTimeInSeconds = (pos == 0) ? 0 : m_duration;
      CPVRRecording* pThis = const_cast<CPVRRecording*>(this);
      pThis->CVideoInfoTag::SetResumePoint(resumePoint);
    }
  }
  return CVideoInfoTag::GetResumePoint();
}

bool CPVRRecording::UpdateRecordingSize()
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsSize() &&
      m_recordingSizeRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_recordingSizeRefetchTimeout.Set(10s); // update size from backend at most every 10 secs

    int64_t sizeInBytes = -1;
    client->GetRecordingSize(*this, sizeInBytes);

    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (sizeInBytes >= 0 && sizeInBytes != m_sizeInBytes)
    {
      m_sizeInBytes = sizeInBytes;
      return true;
    }
  }

  return false;
}

void CPVRRecording::UpdateMetadata(CVideoDatabase& db, const CPVRClient& client)
{
  if (m_bGotMetaData || !db.IsOpen())
    return;

  if (!client.GetClientCapabilities().SupportsRecordingsPlayCount())
    CVideoInfoTag::SetPlayCount(db.GetPlayCount(m_strFileNameAndPath));

  if (!client.GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    CBookmark resumePoint;
    if (db.GetResumeBookMark(m_strFileNameAndPath, resumePoint))
      CVideoInfoTag::SetResumePoint(resumePoint);
  }

  m_lastPlayed = db.GetLastPlayed(m_strFileNameAndPath);

  m_bGotMetaData = true;
}

std::vector<PVR_EDL_ENTRY> CPVRRecording::GetEdl() const
{
  std::vector<PVR_EDL_ENTRY> edls;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsEdl())
    client->GetRecordingEdl(*this, edls);

  return edls;
}

void CPVRRecording::Update(const CPVRRecording& tag, const CPVRClient& client)
{
  m_strRecordingId = tag.m_strRecordingId;
  m_iClientId = tag.m_iClientId;
  m_strTitle = tag.m_strTitle;
  m_strShowTitle = tag.m_strShowTitle;
  m_iSeason = tag.m_iSeason;
  m_iEpisode = tag.m_iEpisode;
  SetPremiered(tag.GetPremiered());
  m_recordingTime = tag.m_recordingTime;
  m_iPriority = tag.m_iPriority;
  m_iLifetime = tag.m_iLifetime;
  m_strDirectory = tag.m_strDirectory;
  m_strPlot = tag.m_strPlot;
  m_strPlotOutline = tag.m_strPlotOutline;
  m_strChannelName = tag.m_strChannelName;
  m_genre = tag.m_genre;
  m_iconPath = tag.m_iconPath;
  m_thumbnailPath = tag.m_thumbnailPath;
  m_fanartPath = tag.m_fanartPath;
  m_bIsDeleted = tag.m_bIsDeleted;
  m_iEpgEventId = tag.m_iEpgEventId;
  m_iChannelUid = tag.m_iChannelUid;
  m_bRadio = tag.m_bRadio;
  m_firstAired = tag.m_firstAired;
  m_iFlags = tag.m_iFlags;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_sizeInBytes = tag.m_sizeInBytes;
    m_strProviderName = tag.m_strProviderName;
    m_iClientProviderUniqueId = tag.m_iClientProviderUniqueId;
  }

  if (client.GetClientCapabilities().SupportsRecordingsPlayCount())
    CVideoInfoTag::SetPlayCount(tag.GetLocalPlayCount());

  if (client.GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
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

void CPVRRecording::UpdatePath()
{
  m_strFileNameAndPath = CPVRRecordingsPath(m_bIsDeleted, m_bRadio, m_strDirectory, m_strTitle,
                                            m_iSeason, m_iEpisode, GetYear(), m_strShowTitle,
                                            m_strChannelName, m_recordingTime, m_strRecordingId);
}

const CDateTime& CPVRRecording::RecordingTimeAsLocalTime() const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_recordingTime);

  return tmp;
}

CDateTime CPVRRecording::EndTimeAsUTC() const
{
  unsigned int duration = GetDuration();
  return m_recordingTime + CDateTimeSpan(0, 0, duration / 60, duration % 60);
}

CDateTime CPVRRecording::EndTimeAsLocalTime() const
{
  CDateTime ret;
  ret.SetFromUTCDateTime(EndTimeAsUTC());
  return ret;
}

bool CPVRRecording::WillBeExpiredWithNewLifetime(int iLifetime) const
{
  if (iLifetime > 0)
    return (EndTimeAsUTC() + CDateTimeSpan(iLifetime, 0, 0, 0)) <= CDateTime::GetUTCDateTime();

  return false;
}

CDateTime CPVRRecording::ExpirationTimeAsLocalTime() const
{
  CDateTime ret;
  if (m_iLifetime > 0)
    ret = EndTimeAsLocalTime() + CDateTimeSpan(m_iLifetime, 0, 0, 0);

  return ret;
}

std::string CPVRRecording::GetTitleFromURL(const std::string& url)
{
  return CPVRRecordingsPath(url).GetTitle();
}

std::shared_ptr<CPVRChannel> CPVRRecording::Channel() const
{
  if (m_iChannelUid != PVR_CHANNEL_INVALID_UID)
    return CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_iChannelUid,
                                                                          m_iClientId);

  return std::shared_ptr<CPVRChannel>();
}

int CPVRRecording::ChannelUid() const
{
  return m_iChannelUid;
}

int CPVRRecording::ClientID() const
{
  return m_iClientId;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRRecording::GetRecordingTimer() const
{
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> recordingTimers =
      CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();

  for (const auto& timer : recordingTimers)
  {
    if (timer->ClientID() == ClientID() && timer->ClientChannelUID() == ChannelUid())
    {
      // first, match epg event uids, if available
      if (timer->UniqueBroadcastID() == BroadcastUid() &&
          timer->UniqueBroadcastID() != EPG_TAG_INVALID_UID)
        return timer;

      // alternatively, match start and end times
      const CDateTime timerStart =
          timer->StartAsUTC() - CDateTimeSpan(0, 0, timer->MarginStart(), 0);
      const CDateTime timerEnd = timer->EndAsUTC() + CDateTimeSpan(0, 0, timer->MarginEnd(), 0);
      if (timerStart <= RecordingTimeAsUTC() && timerEnd >= EndTimeAsUTC())
        return timer;
    }
  }
  return {};
}

bool CPVRRecording::IsInProgress() const
{
  // Note: It is not enough to only check recording time and duration against 'now'.
  //       Only the state of the related timer is a safe indicator that the backend
  //       actually is recording this.
  // Once the recording is known to not be in progress that will never change.
  if (m_bInProgress)
    m_bInProgress = GetRecordingTimer() != nullptr;
  return m_bInProgress;
}

void CPVRRecording::SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre)
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

const std::string CPVRRecording::GetGenresLabel() const
{
  return StringUtils::Join(
      m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}

CDateTime CPVRRecording::FirstAired() const
{
  return m_firstAired;
}

void CPVRRecording::SetYear(int year)
{
  if (year > 0)
    m_premiered = CDateTime(year, 1, 1, 0, 0, 0);
}

int CPVRRecording::GetYear() const
{
  return m_premiered.IsValid() ? m_premiered.GetYear() : 0;
}

bool CPVRRecording::HasYear() const
{
  return m_premiered.IsValid();
}

bool CPVRRecording::IsNew() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_NEW) > 0;
}

bool CPVRRecording::IsPremiere() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_PREMIERE) > 0;
}

bool CPVRRecording::IsLive() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_LIVE) > 0;
}

bool CPVRRecording::IsFinale() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_FINALE) > 0;
}

int64_t CPVRRecording::GetSizeInBytes() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_sizeInBytes;
}

int CPVRRecording::ClientProviderUniqueId() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iClientProviderUniqueId;
}

std::string CPVRRecording::ProviderName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strProviderName;
}

std::shared_ptr<CPVRProvider> CPVRRecording::GetDefaultProvider() const
{
  return CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId,
                                                                  PVR_PROVIDER_INVALID_UID);
}

bool CPVRRecording::HasClientProvider() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iClientProviderUniqueId != PVR_PROVIDER_INVALID_UID;
}

std::shared_ptr<CPVRProvider> CPVRRecording::GetProvider() const
{
  auto provider = CServiceBroker::GetPVRManager().Providers()->GetByClient(
      m_iClientId, m_iClientProviderUniqueId);

  if (!provider)
    provider = GetDefaultProvider();

  return provider;
}
