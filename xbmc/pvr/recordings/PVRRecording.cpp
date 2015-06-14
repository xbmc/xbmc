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
#include "epg/EpgContainer.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "pvr/addons/PVRClients.h"
#include "utils/StringUtils.h"
#include "utils/RegExp.h"
#include "video/VideoDatabase.h"

#include "epg/Epg.h"

using namespace PVR;
using namespace EPG;

CPVRRecordingUid::CPVRRecordingUid() :
    m_iClientId(PVR_INVALID_CLIENT_ID)
{
}

CPVRRecordingUid::CPVRRecordingUid(const CPVRRecordingUid &recordingId) :
  m_iClientId(recordingId.m_iClientId),
  m_strRecordingId(recordingId.m_strRecordingId)
{
}

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
  m_iClientId                      = iClientId;
  m_recordingTime                  = recording.recordingTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_duration                       = CDateTimeSpan(0, 0, recording.iDuration / 60, recording.iDuration % 60);
  m_iPriority                      = recording.iPriority;
  m_iLifetime                      = recording.iLifetime;
  // Deleted recording is placed at the root of the deleted view
  m_strDirectory                   = recording.bIsDeleted ? "" : recording.strDirectory;
  m_strPlot                        = recording.strPlot;
  m_strPlotOutline                 = recording.strPlotOutline;
  m_strStreamURL                   = recording.strStreamURL;
  m_strChannelName                 = recording.strChannelName;
  m_genre                          = StringUtils::Split(CEpg::ConvertGenreIdToString(recording.iGenreType, recording.iGenreSubType), g_advancedSettings.m_videoItemSeparator);
  m_playCount                      = recording.iPlayCount;
  m_resumePoint.timeInSeconds      = recording.iLastPlayedPosition;
  m_resumePoint.totalTimeInSeconds = recording.iDuration;
  m_strIconPath                    = recording.strIconPath;
  m_strThumbnailPath               = recording.strThumbnailPath;
  m_strFanartPath                  = recording.strFanartPath;
  m_bIsDeleted                     = recording.bIsDeleted;
  m_iEpgEventId                    = recording.iEpgEventId;
}

bool CPVRRecording::operator ==(const CPVRRecording& right) const
{
  return (this == &right) ||
      (m_strRecordingId     == right.m_strRecordingId &&
       m_iClientId          == right.m_iClientId &&
       m_strChannelName     == right.m_strChannelName &&
       m_recordingTime      == right.m_recordingTime &&
       m_duration           == right.m_duration &&
       m_strPlotOutline     == right.m_strPlotOutline &&
       m_strPlot            == right.m_strPlot &&
       m_strStreamURL       == right.m_strStreamURL &&
       m_iPriority          == right.m_iPriority &&
       m_iLifetime          == right.m_iLifetime &&
       m_strDirectory       == right.m_strDirectory &&
       m_strFileNameAndPath == right.m_strFileNameAndPath &&
       m_strTitle           == right.m_strTitle &&
       m_strIconPath        == right.m_strIconPath &&
       m_strThumbnailPath   == right.m_strThumbnailPath &&
       m_strFanartPath      == right.m_strFanartPath &&
       m_iRecordingId       == right.m_iRecordingId &&
       m_bIsDeleted         == right.m_bIsDeleted &&
       m_iEpgEventId        == right.m_iEpgEventId);
}

bool CPVRRecording::operator !=(const CPVRRecording& right) const
{
  return !(*this == right);
}

void CPVRRecording::Serialize(CVariant& value) const
{
  CVideoInfoTag::Serialize(value);

  value["channel"] = m_strChannelName;
  value["runtime"] = m_duration.GetSecondsTotal();
  value["lifetime"] = m_iLifetime;
  value["streamurl"] = m_strStreamURL;
  value["directory"] = m_strDirectory;
  value["icon"] = m_strIconPath;
  value["starttime"] = m_recordingTime.IsValid() ? m_recordingTime.GetAsDBDateTime() : "";
  value["endtime"] = m_recordingTime.IsValid() ? (m_recordingTime + m_duration).GetAsDBDateTime() : "";
  value["recordingid"] = m_iRecordingId;
  value["deleted"] = m_bIsDeleted;
  value["epgevent"] = m_iEpgEventId;

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
  m_iEpgEventId        = -1;

  m_recordingTime.Reset();
  CVideoInfoTag::Reset();
}

int CPVRRecording::GetDuration() const
{
  return (m_duration.GetDays() * 60*60*24 +
      m_duration.GetHours() * 60*60 +
      m_duration.GetMinutes() * 60 +
      m_duration.GetSeconds());
}

bool CPVRRecording::Delete(void)
{
  PVR_ERROR error = g_PVRClients->DeleteRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }
  OnDelete();
  return true;
}

void CPVRRecording::OnDelete(void)
{
  EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(EpgEvent());
  if (epgTag)
    epgTag->ClearRecording();
}

bool CPVRRecording::Undelete(void)
{
  PVR_ERROR error = g_PVRClients->UndeleteRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRRecording::Rename(const std::string &strNewName)
{
  m_strTitle = StringUtils::Format("%s", strNewName.c_str());
  PVR_ERROR error = g_PVRClients->RenameRecording(*this);
  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRRecording::SetPlayCount(int count)
{
  PVR_ERROR error;
  m_playCount = count;
  if (g_PVRClients->SupportsRecordingPlayCount(m_iClientId) &&
      !g_PVRClients->SetRecordingPlayCount(*this, count, &error))
  {
    DisplayError(error);
    return false;
  }

  return true;
}

void CPVRRecording::UpdateMetadata(CVideoDatabase &db)
{
  if (m_bGotMetaData)
    return;

  bool supportsPlayCount  = g_PVRClients->SupportsRecordingPlayCount(m_iClientId);
  bool supportsLastPlayed = g_PVRClients->SupportsLastPlayedPosition(m_iClientId);

  if (!supportsPlayCount || !supportsLastPlayed)
  {
    if (!supportsPlayCount)
      m_playCount = db.GetPlayCount(m_strFileNameAndPath);

    if (!supportsLastPlayed)
      db.GetResumeBookMark(m_strFileNameAndPath, m_resumePoint);
  }

  m_bGotMetaData = true;
}

bool CPVRRecording::IncrementPlayCount()
{
  return SetPlayCount(m_playCount + 1);
}

bool CPVRRecording::SetLastPlayedPosition(int lastplayedposition)
{
  PVR_ERROR error;

  CBookmark bookmark;
  bookmark.timeInSeconds = lastplayedposition;
  bookmark.totalTimeInSeconds = (double)GetDuration();
  m_resumePoint = bookmark;

  if (g_PVRClients->SupportsLastPlayedPosition(m_iClientId) &&
      !g_PVRClients->SetRecordingLastPlayedPosition(*this, lastplayedposition, &error))
  {
    DisplayError(error);
    return false;
  }
  return true;
}

int CPVRRecording::GetLastPlayedPosition() const
{
  int rc = -1;
  if (g_PVRClients->SupportsLastPlayedPosition(m_iClientId))
  {
    rc = g_PVRClients->GetRecordingLastPlayedPosition(*this);
    if (rc < 0)
      DisplayError(PVR_ERROR_SERVER_ERROR);
  }
  return rc;
}

std::vector<PVR_EDL_ENTRY> CPVRRecording::GetEdl() const
{
  if (g_PVRClients->SupportsRecordingEdl(m_iClientId))
  {
    return g_PVRClients->GetRecordingEdl(*this);
  }
  return std::vector<PVR_EDL_ENTRY>();
}

void CPVRRecording::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(19033, 19111); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_REJECTED)
    CGUIDialogOK::ShowAndGetInput(19033, 19068); /* print info dialog "Couldn't delete recording!" */
  else
    CGUIDialogOK::ShowAndGetInput(19033, 19147); /* print info dialog "Unknown error!" */

  return;
}

void CPVRRecording::Update(const CPVRRecording &tag)
{
  m_strRecordingId    = tag.m_strRecordingId;
  m_iClientId         = tag.m_iClientId;
  m_strTitle          = tag.m_strTitle;
  m_recordingTime     = tag.m_recordingTime;
  m_duration          = tag.m_duration;
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

  if (g_PVRClients->SupportsRecordingPlayCount(m_iClientId))
    m_playCount       = tag.m_playCount;

  if (g_PVRClients->SupportsLastPlayedPosition(m_iClientId))
  {
    m_resumePoint.timeInSeconds = tag.m_resumePoint.timeInSeconds;
    m_resumePoint.totalTimeInSeconds = tag.m_resumePoint.totalTimeInSeconds;
  }

  std::string strShow = StringUtils::Format("%s - ", g_localizeStrings.Get(20364).c_str());
  if (StringUtils::StartsWithNoCase(m_strPlotOutline, strShow))
  {
    std::string strEpisode = m_strPlotOutline;
    std::string strTitle = m_strDirectory;

    size_t pos = strTitle.rfind('/');
    strTitle.erase(0, pos + 1);
    strEpisode.erase(0, strShow.size());
    m_strTitle = StringUtils::Format("%s - %s", strTitle.c_str(), strEpisode.c_str());
    pos = strEpisode.find('-');
    strEpisode.erase(0, pos + 2);
    m_strPlotOutline = strEpisode;
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
    std::string strTitle(m_strTitle);
    std::string strDatetime(m_recordingTime.GetAsSaveString());
    std::string strDirectory;
    std::string strChannel;
    StringUtils::Replace(strTitle, '/',' ');

    if (!m_strDirectory.empty())
      strDirectory = StringUtils::Format("%s/", m_strDirectory.c_str());
    if (!m_strChannelName.empty())
    {
      strChannel = StringUtils::Format(" (%s)", m_strChannelName.c_str());
      StringUtils::Replace(strChannel, '/',' ');
    }
    m_strFileNameAndPath = StringUtils::Format("pvr://" PVR_RECORDING_BASE_PATH "/%s/%s%s, TV%s, %s.pvr", m_bIsDeleted ? PVR_RECORDING_DELETED_PATH : PVR_RECORDING_ACTIVE_PATH,  strDirectory.c_str(), strTitle.c_str(), strChannel.c_str(), strDatetime.c_str());
  }
}

const CDateTime &CPVRRecording::RecordingTimeAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_recordingTime);

  return tmp;
}

std::string CPVRRecording::GetTitleFromURL(const std::string &url)
{
  CRegExp reg(true);
  if (reg.RegComp("pvr://" PVR_RECORDING_BASE_PATH "/(.*/)*(.*), TV( \\(.*\\))?, "
      "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9].pvr"))
  {
    if (reg.RegFind(url.c_str()) >= 0)
      return reg.GetMatch(2);
  }
  return "";
}

void CPVRRecording::CopyClientInfo(CVideoInfoTag *target) const
{
  if (!target)
    return;

  target->m_playCount   = m_playCount;
  target->m_resumePoint = m_resumePoint;
}

CPVRChannelPtr CPVRRecording::Channel(void) const
{
  if (m_iEpgEventId)
  {
    EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(m_iEpgEventId);
    if (epgTag)
      return epgTag->ChannelTag();
  }
  return CPVRChannelPtr();
}

bool CPVRRecording::IsBeingRecorded(void) const
{
  if (m_iEpgEventId)
  {
    EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(m_iEpgEventId);
    return epgTag ? epgTag->HasRecording() : false;
  }
  return false;
}
