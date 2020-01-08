/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecording.h"

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
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
#include <string>
#include <vector>

using namespace PVR;

CPVRRecordingUid::CPVRRecordingUid(int iClientId, const std::string& strRecordingId) :
  m_iClientId(iClientId),
  m_strRecordingId(strRecordingId)
{
}

bool CPVRRecordingUid::operator >(const CPVRRecordingUid& right) const
{
  return (m_iClientId == right.m_iClientId) ?
            m_strRecordingId > right.m_strRecordingId :
            m_iClientId > right.m_iClientId;
}

bool CPVRRecordingUid::operator <(const CPVRRecordingUid& right) const
{
  return (m_iClientId == right.m_iClientId) ?
            m_strRecordingId < right.m_strRecordingId :
            m_iClientId < right.m_iClientId;
}

bool CPVRRecordingUid::operator ==(const CPVRRecordingUid& right) const
{
  return m_iClientId == right.m_iClientId && m_strRecordingId == right.m_strRecordingId;
}

bool CPVRRecordingUid::operator !=(const CPVRRecordingUid& right) const
{
  return m_iClientId != right.m_iClientId || m_strRecordingId != right.m_strRecordingId;
}


CPVRRecording::CPVRRecording()
{
  Reset();
}

CPVRRecording::CPVRRecording(const PVR_RECORDING& recording, unsigned int iClientId)
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
  m_recordingTime = recording.recordingTime + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  m_iPriority = recording.iPriority;
  m_iLifetime = recording.iLifetime;
  // Deleted recording is placed at the root of the deleted view
  m_strDirectory = recording.bIsDeleted ? "" : recording.strDirectory;
  m_strPlot = recording.strPlot;
  m_strPlotOutline = recording.strPlotOutline;
  m_strChannelName = recording.strChannelName;
  m_strIconPath = recording.strIconPath;
  m_strThumbnailPath = recording.strThumbnailPath;
  m_strFanartPath = recording.strFanartPath;
  m_bIsDeleted = recording.bIsDeleted;
  m_iEpgEventId = recording.iEpgEventId;
  m_iChannelUid = recording.iChannelUid;

  SetGenre(recording.iGenreType, recording.iGenreSubType, recording.strGenreDescription);
  CVideoInfoTag::SetPlayCount(recording.iPlayCount);
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
    const std::shared_ptr<CPVRChannel> channel(Channel());
    if (channel)
    {
      m_bRadio = channel->IsRadio();
    }
    else
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
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

bool CPVRRecording::operator ==(const CPVRRecording& right) const
{
  return (this == &right) ||
      (m_strRecordingId == right.m_strRecordingId &&
       m_iClientId == right.m_iClientId &&
       m_strChannelName == right.m_strChannelName &&
       m_recordingTime == right.m_recordingTime &&
       GetDuration() == right.GetDuration() &&
       m_strPlotOutline == right.m_strPlotOutline &&
       m_strPlot == right.m_strPlot &&
       m_iPriority == right.m_iPriority &&
       m_iLifetime == right.m_iLifetime &&
       m_strDirectory == right.m_strDirectory &&
       m_strFileNameAndPath == right.m_strFileNameAndPath &&
       m_strTitle == right.m_strTitle &&
       m_strShowTitle == right.m_strShowTitle &&
       m_iSeason == right.m_iSeason &&
       m_iEpisode == right.m_iEpisode &&
       GetPremiered() == right.GetPremiered() &&
       m_strIconPath == right.m_strIconPath &&
       m_strThumbnailPath == right.m_strThumbnailPath &&
       m_strFanartPath == right.m_strFanartPath &&
       m_iRecordingId == right.m_iRecordingId &&
       m_bIsDeleted == right.m_bIsDeleted &&
       m_iEpgEventId == right.m_iEpgEventId &&
       m_iChannelUid == right.m_iChannelUid &&
       m_bRadio == right.m_bRadio &&
       m_genre == right.m_genre &&
       m_iGenreType == right.m_iGenreType &&
       m_iGenreSubType == right.m_iGenreSubType);
}

bool CPVRRecording::operator !=(const CPVRRecording& right) const
{
  return !(*this == right);
}

void CPVRRecording::Serialize(CVariant& value) const
{
  CVideoInfoTag::Serialize(value);

  value["channel"] = m_strChannelName;
  value["lifetime"] = m_iLifetime;
  value["directory"] = m_strDirectory;
  value["icon"] = m_strIconPath;
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
  if (!m_strThumbnailPath.empty())
    value["art"]["thumb"] = m_strThumbnailPath;
  if (!m_strFanartPath.empty())
    value["art"]["fanart"] = m_strFanartPath;
}

void CPVRRecording::Reset()
{
  m_strRecordingId     .clear();
  m_iClientId = -1;
  m_strChannelName     .clear();
  m_strDirectory       .clear();
  m_iPriority = -1;
  m_iLifetime = -1;
  m_strFileNameAndPath .clear();
  m_strIconPath        .clear();
  m_strThumbnailPath   .clear();
  m_strFanartPath      .clear();
  m_bGotMetaData = false;
  m_iRecordingId = 0;
  m_bIsDeleted = false;
  m_iEpgEventId = EPG_TAG_INVALID_UID;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iChannelUid = PVR_CHANNEL_INVALID_UID;
  m_bRadio = false;

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
    if (client->SetRecordingPlayCount(*this, CVideoInfoTag::GetPlayCount() + 1) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::IncrementPlayCount();
}

bool CPVRRecording::SetResumePoint(const CBookmark& resumePoint)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    if (client->SetRecordingLastPlayedPosition(*this, lrint(resumePoint.timeInSeconds)) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(resumePoint);
}

bool CPVRRecording::SetResumePoint(double timeInSeconds, double totalTimeInSeconds, const std::string& playerState /* = "" */)
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
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition() &&
      m_resumePointRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_resumePointRefetchTimeout.Set(10000); // update resume point from backend at most every 10 secs

    int pos = -1;
    client->GetRecordingLastPlayedPosition(*this, pos);

    if (pos >= 0)
    {
      CBookmark resumePoint(CVideoInfoTag::GetResumePoint());
      resumePoint.timeInSeconds = pos;
      CPVRRecording* pThis = const_cast<CPVRRecording*>(this);
      pThis->CVideoInfoTag::SetResumePoint(resumePoint);
    }
  }
  return CVideoInfoTag::GetResumePoint();
}

void CPVRRecording::UpdateMetadata(CVideoDatabase& db)
{
  if (m_bGotMetaData || !db.IsOpen())
    return;

  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);

  if (!client || !client->GetClientCapabilities().SupportsRecordingsPlayCount())
    CVideoInfoTag::SetPlayCount(db.GetPlayCount(m_strFileNameAndPath));

  if (!client || !client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    CBookmark resumePoint;
    if (db.GetResumeBookMark(m_strFileNameAndPath, resumePoint))
      CVideoInfoTag::SetResumePoint(resumePoint);
  }

  m_bGotMetaData = true;
}

std::vector<PVR_EDL_ENTRY> CPVRRecording::GetEdl() const
{
  std::vector<PVR_EDL_ENTRY> edls;

  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsEdl())
    client->GetRecordingEdl(*this, edls);

  return edls;
}

void CPVRRecording::Update(const CPVRRecording& tag)
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
  m_strIconPath = tag.m_strIconPath;
  m_strThumbnailPath = tag.m_strThumbnailPath;
  m_strFanartPath = tag.m_strFanartPath;
  m_bIsDeleted = tag.m_bIsDeleted;
  m_iEpgEventId = tag.m_iEpgEventId;
  m_iChannelUid = tag.m_iChannelUid;
  m_bRadio = tag.m_bRadio;

  CVideoInfoTag::SetPlayCount(tag.GetLocalPlayCount());
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
    m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }

  //Old Method of identifying TV show title and subtitle using m_strDirectory and strPlotOutline (deprecated)
  std::string strShow = StringUtils::Format("%s - ", g_localizeStrings.Get(20364).c_str());
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
  m_strFileNameAndPath = CPVRRecordingsPath(
    m_bIsDeleted, m_bRadio, m_strDirectory, m_strTitle, m_iSeason, m_iEpisode, GetYear(), m_strShowTitle, m_strChannelName, m_recordingTime, m_strRecordingId);
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
    return CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_iChannelUid, m_iClientId);

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
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> recordingTimers = CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();

  for (const auto& timer : recordingTimers)
  {
    if (timer->m_iClientId == ClientID() &&
        timer->m_iClientChannelUid == ChannelUid())
    {
      // first, match epg event uids, if available
      if (timer->UniqueBroadcastID() == BroadcastUid() &&
          timer->UniqueBroadcastID() != EPG_TAG_INVALID_UID)
        return timer;

      // alternatively, match start and end times
      const CDateTime timerStart = timer->StartAsUTC() - CDateTimeSpan(0, 0, timer->m_iMarginStart, 0);
      const CDateTime timerEnd = timer->EndAsUTC() + CDateTimeSpan(0, 0, timer->m_iMarginEnd, 0);
      if (timerStart <= RecordingTimeAsUTC() &&
          timerEnd >= EndTimeAsUTC())
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

  return GetRecordingTimer() != nullptr;
}

void CPVRRecording::SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre)
{
  m_iGenreType = iGenreType;
  m_iGenreSubType = iGenreSubType;

  if ((iGenreType == EPG_GENRE_USE_STRING || iGenreSubType == EPG_GENRE_USE_STRING) && !strGenre.empty())
  {
    /* Type and sub type are not given. Use the provided genre description if available. */
    m_genre = StringUtils::Split(strGenre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
  else
  {
    /* Determine the genre description from the type and subtype IDs */
    m_genre = StringUtils::Split(CPVREpg::ConvertGenreIdToString(iGenreType, iGenreSubType), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
}

const std::string CPVRRecording::GetGenresLabel() const
{
  return StringUtils::Join(m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}
