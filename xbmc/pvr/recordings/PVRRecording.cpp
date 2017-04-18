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

#include "dialogs/GUIDialogOK.h"
#include "epg/Epg.h"
#include "epg/EpgContainer.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"

#include "PVRRecording.h"

using namespace PVR;
using namespace EPG;

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

CPVRRecording::CPVRRecording(const PVR_RECORDING &recording, unsigned int iClientId)
{
  Reset();

  m_strRecordingId                 = recording.strRecordingId;
  m_strTitle                       = recording.strTitle;
  m_strShowTitle                   = recording.strEpisodeName;
  m_iSeason                        = recording.iSeriesNumber;
  m_iEpisode                       = recording.iEpisodeNumber;
  if (recording.iYear > 0)
    SetYear(recording.iYear);
  m_iClientId                      = iClientId;
  m_recordingTime                  = recording.recordingTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_iPriority                      = recording.iPriority;
  m_iLifetime                      = recording.iLifetime;
  // Deleted recording is placed at the root of the deleted view
  m_strDirectory                   = recording.bIsDeleted ? "" : recording.strDirectory;
  m_strPlot                        = recording.strPlot;
  m_strPlotOutline                 = recording.strPlotOutline;
  m_strStreamURL                   = recording.strStreamURL;
  m_strChannelName                 = recording.strChannelName;
  m_genre                          = StringUtils::Split(CEpg::ConvertGenreIdToString(recording.iGenreType, recording.iGenreSubType), g_advancedSettings.m_videoItemSeparator);
  m_strIconPath                    = recording.strIconPath;
  m_strThumbnailPath               = recording.strThumbnailPath;
  m_strFanartPath                  = recording.strFanartPath;
  m_bIsDeleted                     = recording.bIsDeleted;
  m_iEpgEventId                    = recording.iEpgEventId;
  m_iChannelUid                    = recording.iChannelUid;

  CVideoInfoTag::SetPlayCount(recording.iPlayCount);
  CVideoInfoTag::SetResumePoint(recording.iLastPlayedPosition, recording.iDuration);
  SetDuration(recording.iDuration);

  //  As the channel a recording was done on (probably long time ago) might no longer be
  //  available today prefer addon-supplied channel type (tv/radio) over channel attribute.
  if (recording.channelType != PVR_RECORDING_CHANNEL_TYPE_UNKNOWN)
  {
    m_bRadio = recording.channelType == PVR_RECORDING_CHANNEL_TYPE_RADIO;
  }
  else
  {
    const CPVRChannelPtr channel(Channel());
    if (channel)
    {
      m_bRadio = channel->IsRadio();
    }
    else
    {
      bool bSupportsRadio(CServiceBroker::GetPVRManager().Clients()->SupportsRadio(m_iClientId));
      if (bSupportsRadio && CServiceBroker::GetPVRManager().Clients()->SupportsTV(m_iClientId))
      {
        CLog::Log(LOGWARNING,"CPVRRecording::CPVRRecording - unable to determine channel type. Defaulting to TV.");
        m_bRadio = false; // Assume TV.
      }
      else
      {
        m_bRadio = bSupportsRadio;
      }
    }
  }
}

bool CPVRRecording::operator ==(const CPVRRecording& right) const
{
  return (this == &right) ||
      (m_strRecordingId     == right.m_strRecordingId &&
       m_iClientId          == right.m_iClientId &&
       m_strChannelName     == right.m_strChannelName &&
       m_recordingTime      == right.m_recordingTime &&
       GetDuration()        == right.GetDuration() &&
       m_strPlotOutline     == right.m_strPlotOutline &&
       m_strPlot            == right.m_strPlot &&
       m_strStreamURL       == right.m_strStreamURL &&
       m_iPriority          == right.m_iPriority &&
       m_iLifetime          == right.m_iLifetime &&
       m_strDirectory       == right.m_strDirectory &&
       m_strFileNameAndPath == right.m_strFileNameAndPath &&
       m_strTitle           == right.m_strTitle &&
       m_strShowTitle       == right.m_strShowTitle &&
       m_iSeason            == right.m_iSeason &&
       m_iEpisode           == right.m_iEpisode &&
       GetPremiered()       == right.GetPremiered() &&
       m_strIconPath        == right.m_strIconPath &&
       m_strThumbnailPath   == right.m_strThumbnailPath &&
       m_strFanartPath      == right.m_strFanartPath &&
       m_iRecordingId       == right.m_iRecordingId &&
       m_bIsDeleted         == right.m_bIsDeleted &&
       m_iEpgEventId        == right.m_iEpgEventId &&
       m_iChannelUid        == right.m_iChannelUid &&
       m_bRadio             == right.m_bRadio);
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
  value["streamurl"] = m_strStreamURL;
  value["directory"] = m_strDirectory;
  value["icon"] = m_strIconPath;
  value["starttime"] = m_recordingTime.IsValid() ? m_recordingTime.GetAsDBDateTime() : "";
  value["endtime"] = m_recordingTime.IsValid() ? EndTimeAsUTC().GetAsDBDateTime() : "";
  value["recordingid"] = m_iRecordingId;
  value["isdeleted"] = m_bIsDeleted;
  value["epgeventid"] = m_iEpgEventId;
  value["channeluid"] = m_iChannelUid;
  value["radio"] = m_bRadio;

  if (!value.isMember("art"))
    value["art"] = CVariant(CVariant::VariantTypeObject);
  if (!m_strThumbnailPath.empty())
    value["art"]["thumb"] = m_strThumbnailPath;
  if (!m_strFanartPath.empty())
    value["art"]["fanart"] = m_strFanartPath;
}

void CPVRRecording::Reset(void)
{
  m_strRecordingId     .clear();
  m_iClientId          = 0;
  m_strChannelName     .clear();
  m_strDirectory       .clear();
  m_strStreamURL       .clear();
  m_iPriority          = -1;
  m_iLifetime          = -1;
  m_strFileNameAndPath .clear();
  m_strIconPath        .clear();
  m_strThumbnailPath   .clear();
  m_strFanartPath      .clear();
  m_bGotMetaData       = false;
  m_iRecordingId       = 0;
  m_bIsDeleted         = false;
  m_iEpgEventId        = EPG_TAG_INVALID_UID;
  m_iSeason            = -1;
  m_iEpisode           = -1;
  m_iChannelUid        = PVR_CHANNEL_INVALID_UID;
  m_bRadio             = false;

  m_recordingTime.Reset();
  CVideoInfoTag::Reset();
}

bool CPVRRecording::Delete(void)
{
  PVR_ERROR error = CServiceBroker::GetPVRManager().Clients()->DeleteRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
    return false;

  OnDelete();
  return true;
}

void CPVRRecording::OnDelete(void)
{
  if (m_iEpgEventId != EPG_TAG_INVALID_UID)
  {
    const CPVRChannelPtr channel(Channel());
    if (channel)
    {
      const EPG::CEpgInfoTagPtr epgTag(EPG::CEpgContainer::GetInstance().GetTagById(channel, m_iEpgEventId));
      if (epgTag)
        epgTag->ClearRecording();
    }
  }
}

bool CPVRRecording::Undelete(void)
{
  PVR_ERROR error = CServiceBroker::GetPVRManager().Clients()->UndeleteRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
    return false;

  return true;
}

bool CPVRRecording::Rename(const std::string &strNewName)
{
  m_strTitle = StringUtils::Format("%s", strNewName.c_str());
  PVR_ERROR error = CServiceBroker::GetPVRManager().Clients()->RenameRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
    return false;

  return true;
}

bool CPVRRecording::SetPlayCount(int count)
{
  PVR_ERROR error;
  if (CServiceBroker::GetPVRManager().Clients()->SupportsRecordingPlayCount(m_iClientId) &&
      !CServiceBroker::GetPVRManager().Clients()->SetRecordingPlayCount(*this, count, &error))
    return false;

  return CVideoInfoTag::SetPlayCount(count);
}

bool CPVRRecording::IncrementPlayCount()
{
  PVR_ERROR error;
  if (CServiceBroker::GetPVRManager().Clients()->SupportsRecordingPlayCount(m_iClientId) &&
      !CServiceBroker::GetPVRManager().Clients()->SetRecordingPlayCount(*this, CVideoInfoTag::GetPlayCount(), &error))
    return false;

  return CVideoInfoTag::IncrementPlayCount();
}

bool CPVRRecording::SetResumePoint(const CBookmark &resumePoint)
{
  PVR_ERROR error;
  if (CServiceBroker::GetPVRManager().Clients()->SupportsLastPlayedPosition(m_iClientId) &&
      !CServiceBroker::GetPVRManager().Clients()->SetRecordingLastPlayedPosition(*this, lrint(resumePoint.timeInSeconds), &error))
    return false;

  return CVideoInfoTag::SetResumePoint(resumePoint);
}

bool CPVRRecording::SetResumePoint(double timeInSeconds, double totalTimeInSeconds, const std::string &playerState /* = "" */)
{
  PVR_ERROR error;
  if (CServiceBroker::GetPVRManager().Clients()->SupportsLastPlayedPosition(m_iClientId) &&
      !CServiceBroker::GetPVRManager().Clients()->SetRecordingLastPlayedPosition(*this, lrint(timeInSeconds), &error))
    return false;

  return CVideoInfoTag::SetResumePoint(timeInSeconds, totalTimeInSeconds, playerState);
}

CBookmark CPVRRecording::GetResumePoint() const
{
  if (CServiceBroker::GetPVRManager().Clients()->SupportsLastPlayedPosition(m_iClientId))
  {
    int pos = CServiceBroker::GetPVRManager().Clients()->GetRecordingLastPlayedPosition(*this);
    if (pos >= 0)
    {
      CBookmark resumePoint(CVideoInfoTag::GetResumePoint());
      resumePoint.timeInSeconds = pos;
      CPVRRecording *pThis = const_cast<CPVRRecording*>(this);
      pThis->CVideoInfoTag::SetResumePoint(resumePoint);
    }
  }
  return CVideoInfoTag::GetResumePoint();
}

void CPVRRecording::UpdateMetadata(CVideoDatabase &db)
{
  if (m_bGotMetaData)
    return;

  if (!CServiceBroker::GetPVRManager().Clients()->SupportsRecordingPlayCount(m_iClientId))
  {
    CVideoInfoTag::SetPlayCount(db.GetPlayCount(m_strFileNameAndPath));
  }

  if (!CServiceBroker::GetPVRManager().Clients()->SupportsLastPlayedPosition(m_iClientId))
  {
    CBookmark resumePoint;
    if (db.GetResumeBookMark(m_strFileNameAndPath, resumePoint))
      CVideoInfoTag::SetResumePoint(resumePoint);
  }

  m_bGotMetaData = true;
}

std::vector<PVR_EDL_ENTRY> CPVRRecording::GetEdl() const
{
  if (CServiceBroker::GetPVRManager().Clients()->SupportsRecordingEdl(m_iClientId))
  {
    return CServiceBroker::GetPVRManager().Clients()->GetRecordingEdl(*this);
  }
  return std::vector<PVR_EDL_ENTRY>();
}

void CPVRRecording::Update(const CPVRRecording &tag)
{
  m_strRecordingId    = tag.m_strRecordingId;
  m_iClientId         = tag.m_iClientId;
  m_strTitle          = tag.m_strTitle;
  m_strShowTitle      = tag.m_strShowTitle;
  m_iSeason           = tag.m_iSeason;
  m_iEpisode          = tag.m_iEpisode;
  SetPremiered(tag.GetPremiered());
  m_recordingTime     = tag.m_recordingTime;
  m_iPriority         = tag.m_iPriority;
  m_iLifetime         = tag.m_iLifetime;
  m_strDirectory      = tag.m_strDirectory;
  m_strPlot           = tag.m_strPlot;
  m_strPlotOutline    = tag.m_strPlotOutline;
  m_strStreamURL      = tag.m_strStreamURL;
  m_strChannelName    = tag.m_strChannelName;
  m_genre             = tag.m_genre;
  m_strIconPath       = tag.m_strIconPath;
  m_strThumbnailPath  = tag.m_strThumbnailPath;
  m_strFanartPath     = tag.m_strFanartPath;
  m_bIsDeleted        = tag.m_bIsDeleted;
  m_iEpgEventId       = tag.m_iEpgEventId;
  m_iChannelUid       = tag.m_iChannelUid;
  m_bRadio            = tag.m_bRadio;

  CVideoInfoTag::SetPlayCount(tag.GetLocalPlayCount());
  CVideoInfoTag::SetResumePoint(tag.GetLocalResumePoint());
  SetDuration(tag.GetDuration());

  //Old Method of identifying TV show title and subtitle using m_strDirectory and strPlotOutline (deprecated)
  std::string strShow = StringUtils::Format("%s - ", g_localizeStrings.Get(20364).c_str());
  if (StringUtils::StartsWithNoCase(m_strPlotOutline, strShow))
  {
    CLog::Log(LOGPVR,"CPVRRecording::Update - PVR addon provides episode name in strPlotOutline which is deprecated");
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

  if (m_bIsDeleted)
    OnDelete();

  UpdatePath();
}

void CPVRRecording::UpdatePath(void)
{
  if (!m_strStreamURL.empty())
  {
    m_strFileNameAndPath = m_strStreamURL;
  }
  else
  {
    m_strFileNameAndPath = CPVRRecordingsPath(
      m_bIsDeleted, m_bRadio, m_strDirectory, m_strTitle, m_iSeason, m_iEpisode, GetYear(), m_strShowTitle, m_strChannelName, m_recordingTime, m_strRecordingId);
  }
}

const CDateTime &CPVRRecording::RecordingTimeAsLocalTime(void) const
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

std::string CPVRRecording::GetTitleFromURL(const std::string &url)
{
  return CPVRRecordingsPath(url).GetTitle();
}

CPVRChannelPtr CPVRRecording::Channel(void) const
{
  if (m_iChannelUid != PVR_CHANNEL_INVALID_UID)
    return CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_iChannelUid, m_iClientId);

  return CPVRChannelPtr();
}

int CPVRRecording::ChannelUid(void) const
{
  return m_iChannelUid;
}

int CPVRRecording::ClientID(void) const
{
  return m_iClientId;
}

bool CPVRRecording::IsInProgress() const
{
  // Note: It is not enough to only check recording time and duration against 'now'.
  //       Only the state of the related timer is a safe indicator that the backend
  //       actually is recording this.

  return CServiceBroker::GetPVRManager().Timers()->HasRecordingTimerForRecording(*this);
}
