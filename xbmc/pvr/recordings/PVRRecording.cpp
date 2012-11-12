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

#include "dialogs/GUIDialogOK.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "PVRRecordings.h"
#include "pvr/addons/PVRClients.h"
#include "utils/StringUtils.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"

#include "epg/Epg.h"

using namespace PVR;
using namespace EPG;

CPVRRecording::CPVRRecording()
{
  Reset();
}

CPVRRecording::CPVRRecording(const PVR_RECORDING &recording, unsigned int iClientId)
{
  Reset();

  m_strRecordingId    = recording.strRecordingId;
  m_strTitle          = recording.strTitle;
  m_iClientId         = iClientId;
  m_recordingTime     = recording.recordingTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_duration          = CDateTimeSpan(0, 0, recording.iDuration / 60, recording.iDuration % 60);
  m_iPriority         = recording.iPriority;
  m_iLifetime         = recording.iLifetime;
  m_strDirectory      = recording.strDirectory;
  m_strPlot           = recording.strPlot;
  m_strPlotOutline    = recording.strPlotOutline;
  m_strStreamURL      = recording.strStreamURL;
  m_strChannelName    = recording.strChannelName;
  m_genre             = StringUtils::Split(CEpg::ConvertGenreIdToString(recording.iGenreType, recording.iGenreSubType), g_advancedSettings.m_videoItemSeparator);
  m_iRecPlayCount     = recording.iPlayCount;
  m_strIconPath       = recording.strIconPath;
  m_strThumbnailPath  = recording.strThumbnailPath;
  m_strFanartPath     = recording.strFanartPath;
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
       m_iRecPlayCount      == right.m_iRecPlayCount &&
       m_strIconPath        == right.m_strIconPath &&
       m_strThumbnailPath   == right.m_strThumbnailPath &&
       m_strFanartPath      == right.m_strFanartPath);
}

bool CPVRRecording::operator !=(const CPVRRecording& right) const
{
  return !(*this == right);
}

void CPVRRecording::Reset(void)
{
  m_strRecordingId     = StringUtils::EmptyString;
  m_iClientId          = 0;
  m_strChannelName     = StringUtils::EmptyString;
  m_strDirectory       = StringUtils::EmptyString;
  m_strStreamURL       = StringUtils::EmptyString;
  m_iPriority          = -1;
  m_iLifetime          = -1;
  m_strFileNameAndPath = StringUtils::EmptyString;
  m_iRecPlayCount      = 0;
  m_strIconPath        = StringUtils::EmptyString;
  m_strThumbnailPath   = StringUtils::EmptyString;
  m_strFanartPath      = StringUtils::EmptyString;

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

  return true;
}

bool CPVRRecording::Rename(const CStdString &strNewName)
{
  m_strTitle.Format("%s", strNewName);
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
  m_iRecPlayCount = count;
  if (g_PVRClients->SupportsRecordingPlayCount(m_iClientId) &&
      !g_PVRClients->SetRecordingPlayCount(*this, count, &error))
  {
    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRRecording::IncrementPlayCount()
{
  return SetPlayCount(m_iRecPlayCount + 1);
}

bool CPVRRecording::SetLastPlayedPosition(int lastplayedposition)
{
  PVR_ERROR error;
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

void CPVRRecording::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(19033,19111,19110,0); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_REJECTED)
    CGUIDialogOK::ShowAndGetInput(19033,19068,19110,0); /* print info dialog "Couldn't delete recording!" */
  else
    CGUIDialogOK::ShowAndGetInput(19033,19147,19110,0); /* print info dialog "Unknown error!" */

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
  m_iRecPlayCount     = tag.m_iRecPlayCount;
  m_strIconPath       = tag.m_strIconPath;
  m_strThumbnailPath  = tag.m_strThumbnailPath;
  m_strFanartPath     = tag.m_strFanartPath;

  CStdString strShow;
  strShow.Format("%s - ", g_localizeStrings.Get(20364).c_str());
  if (m_strPlotOutline.Left(strShow.size()).Equals(strShow))
  {
    CStdString strEpisode = m_strPlotOutline;
    CStdString strTitle = m_strDirectory;
    
    int pos = strTitle.ReverseFind('/');
    strTitle.erase(0, pos + 1);
    strEpisode.erase(0, strShow.size());
    m_strTitle.Format("%s - %s", strTitle.c_str(), strEpisode);
    pos = strEpisode.Find('-');
    strEpisode.erase(0, pos + 2);
    m_strPlotOutline = strEpisode;
  }
  UpdatePath();
}

void CPVRRecording::UpdatePath(void)
{
  if (!m_strStreamURL.IsEmpty())
  {
    m_strFileNameAndPath = m_strStreamURL;
  }
  else
  {
    CStdString strTitle(m_strTitle);
    CStdString strDatetime(m_recordingTime.GetAsSaveString());
    CStdString strDirectory;
    CStdString strChannel;
    strTitle.Replace('/',' ');

    if (!m_strDirectory.IsEmpty())
      strDirectory.Format("%s/", m_strDirectory.c_str());
    if (!m_strChannelName.IsEmpty())
      strChannel.Format(" (%s)", m_strChannelName.c_str());
    m_strFileNameAndPath.Format("pvr://recordings/%s%s, TV%s, %s.pvr", strDirectory.c_str(), strTitle.c_str(), strChannel.c_str(), strDatetime.c_str());
  }
}

const CDateTime &CPVRRecording::RecordingTimeAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_recordingTime);

  return tmp;
}

CStdString CPVRRecording::GetTitleFromURL(const CStdString &url)
{
  CRegExp reg(true);
  if (reg.RegComp("pvr://recordings/(.*/)*(.*), TV( \\(.*\\))?, "
      "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9].pvr"))
  {
    if (reg.RegFind(url.c_str()) >= 0)
      return reg.GetReplaceString("\\2");
  }
  return StringUtils::EmptyString;
}
