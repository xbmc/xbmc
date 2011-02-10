/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "PVRRecordings.h"
#include "PVRRecording.h"

/**
 * Create a blank unmodified recording tag
 */
CPVRRecordingInfoTag::CPVRRecordingInfoTag()
{
  Reset();
}

bool CPVRRecordingInfoTag::operator ==(const CPVRRecordingInfoTag& right) const
{

  if (this == &right) return true;

  return (m_clientIndex         == right.m_clientIndex &&
          m_clientID            == right.m_clientID &&
          m_strChannel          == right.m_strChannel &&
          m_recordingTime       == right.m_recordingTime &&
          m_duration            == right.m_duration &&
          m_strPlotOutline      == right.m_strPlotOutline &&
          m_strPlot             == right.m_strPlot &&
          m_strStreamURL        == right.m_strStreamURL &&
          m_Priority            == right.m_Priority &&
          m_Lifetime            == right.m_Lifetime &&
          m_strDirectory        == right.m_strDirectory &&
          m_strFileNameAndPath  == right.m_strFileNameAndPath &&
          m_strTitle            == right.m_strTitle);
}

bool CPVRRecordingInfoTag::operator !=(const CPVRRecordingInfoTag& right) const
{

  if (this == &right) return false;
  return !(*this == right);
}

/**
 * Initialize blank CPVRRecordingInfoTag
 */
void CPVRRecordingInfoTag::Reset(void)
{
  m_clientIndex           = -1;
  m_clientID              = CPVRManager::Get()->GetFirstClientID(); // Temporary until we support multiple backends
  m_strChannel            = "";
  m_strDirectory          = "";
  m_recordingTime         = NULL;
  m_strStreamURL          = "";
  m_Priority              = -1;
  m_Lifetime              = -1;
  m_strFileNameAndPath    = "";

  CVideoInfoTag::Reset();
}

int CPVRRecordingInfoTag::GetDuration() const
{
  int duration;
  duration =  m_duration.GetDays()*60*60*24;
  duration += m_duration.GetHours()*60*60;
  duration += m_duration.GetMinutes()*60;
  duration += m_duration.GetSeconds();
  duration /= 60;
  return duration;
}

bool CPVRRecordingInfoTag::Delete(void) const
{
  try
  {
    CLIENTMAP *clients = CPVRManager::Get()->Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->DeleteRecording(*this);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    g_PVRRecordings.Update();
    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

bool CPVRRecordingInfoTag::Rename(const CStdString &newName) const
{
  try
  {
    CLIENTMAP *clients = CPVRManager::Get()->Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->RenameRecording(*this, newName);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    g_PVRRecordings.Update();
    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

void CPVRRecordingInfoTag::DisplayError(PVR_ERROR err) const
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
