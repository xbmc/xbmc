/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "dialogs/GUIDialogOK.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "PVRRecordings.h"
#include "pvr/addons/PVRClients.h"

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

  m_strRecordingId = recording.strRecordingId;
  m_strTitle       = recording.strTitle;
  m_iClientId      = iClientId;
  m_recordingTime  = recording.recordingTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_duration       = CDateTimeSpan(0, 0, recording.iDuration / 60, recording.iDuration % 60);
  m_iPriority      = recording.iPriority;
  m_iLifetime      = recording.iLifetime;
  m_strDirectory   = recording.strDirectory;
  m_strPlot        = recording.strPlot;
  m_strPlotOutline = recording.strPlotOutline;
  m_strStreamURL   = recording.strStreamURL;
  m_strChannelName = recording.strChannelName;
  m_strGenre       = CEpg::ConvertGenreIdToString(recording.iGenreType, recording.iGenreSubType);
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
       m_strTitle           == right.m_strTitle);
}

bool CPVRRecording::operator !=(const CPVRRecording& right) const
{
  return !(*this == right);
}

void CPVRRecording::Reset(void)
{
  m_strRecordingId     = "";
  m_iClientId          = g_PVRClients->GetFirstConnectedClientID(); // Temporary until we support multiple backends
  m_strChannelName     = "";
  m_strDirectory       = "";
  m_strStreamURL       = "";
  m_iPriority          = -1;
  m_iLifetime          = -1;
  m_strFileNameAndPath = "";

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
  PVR_ERROR error;
  if (!g_PVRClients->DeleteRecording(*this, &error))
  {
    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRRecording::Rename(const CStdString &strNewName)
{
  PVR_ERROR error;
  m_strTitle.Format("%s", strNewName);
  if (!g_PVRClients->RenameRecording(*this, &error))
  {
    DisplayError(error);
    return false;
  }

  return true;
}

void CPVRRecording::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(19033,19111,19110,0); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_NOT_SYNC)
    CGUIDialogOK::ShowAndGetInput(19033,19108,19110,0); /* print info dialog "Recordings not in sync!" */
  else if (err == PVR_ERROR_NOT_DELETED)
    CGUIDialogOK::ShowAndGetInput(19033,19068,19110,0); /* print info dialog "Couldn't delete recording!" */
  else
    CGUIDialogOK::ShowAndGetInput(19033,19147,19110,0); /* print info dialog "Unknown error!" */

  return;
}

void CPVRRecording::Update(const CPVRRecording &tag)
{
  m_strRecordingId = tag.m_strRecordingId;
  m_iClientId      = tag.m_iClientId;
  m_strTitle       = tag.m_strTitle;
  m_recordingTime  = tag.m_recordingTime;
  m_duration       = tag.m_duration;
  m_iPriority      = tag.m_iPriority;
  m_iLifetime      = tag.m_iLifetime;
  m_strDirectory   = tag.m_strDirectory;
  m_strPlot        = tag.m_strPlot;
  m_strPlotOutline = tag.m_strPlotOutline;
  m_strStreamURL   = tag.m_strStreamURL;
  m_strChannelName = tag.m_strChannelName;
  m_strGenre       = tag.m_strGenre;

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
  CStdString strTitle = m_strTitle;
  CStdString strDatetime = m_recordingTime.GetAsSaveString();
  strTitle.Replace('/','-');
  strTitle.Remove('?');

  if (m_strDirectory != "")
    m_strFileNameAndPath.Format("pvr://recordings/%s/%s/%s.pvr",
        m_strDirectory.c_str(), strDatetime.c_str(), strTitle.c_str());
  else
    m_strFileNameAndPath.Format("pvr://recordings/%s/%s.pvr",
        strDatetime.c_str(), strTitle.c_str());
}

const CDateTime &CPVRRecording::RecordingTimeAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_recordingTime);

  return tmp;
}
