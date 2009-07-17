/*
 *      Copyright (C) 2005-2008 Team XBMC
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

/*
 * DESCRIPTION:
 *
 * cPVRRecordingInfoTag is part of the PVRManager to support recording entrys.
 *
 * The recording information tag holds data about name, length, recording time
 * and so on of recorded stream stored on the backend.
 *
 * The filename string is used to by the PVRManager and passed to DVDPlayer
 * to stream data from the backend to XBMC.
 *
 * It is a also CVideoInfoTag and some of his variables must be set!
 *
 * TODO:
 * Nothing in the moment. Any ideas?
 *
 */

#include "stdafx.h"
#include "PVRRecordings.h"
#include "PVRManager.h"


/**
 * Create a blank unmodified recording tag
 */
cPVRRecordingInfoTag::cPVRRecordingInfoTag()
{
  Reset();
}

bool cPVRRecordingInfoTag::operator ==(const cPVRRecordingInfoTag& right) const
{

  if (this == &right) return true;

  return (m_clientIndex         == right.m_clientIndex &&
          m_clientID            == right.m_clientID &&
          m_strChannel          == right.m_strChannel &&
          m_recordingTime       == right.m_recordingTime &&
          m_duration            == right.m_duration &&
          m_strPlotOutline      == right.m_strPlotOutline &&
          m_strPlot             == right.m_strPlot &&
          m_strFileNameAndPath  == right.m_strFileNameAndPath &&
          m_Priority            == right.m_Priority &&
          m_Lifetime            == right.m_Lifetime &&
          m_strTitle            == right.m_strTitle);
}

bool cPVRRecordingInfoTag::operator !=(const cPVRRecordingInfoTag& right) const
{

  if (this == &right) return false;

  if (m_clientIndex             != right.m_clientIndex) return true;
  if (m_clientID                != right.m_clientID) return true;
  if (m_strChannel              != right.m_strChannel) return true;
  if (m_recordingTime           != right.m_recordingTime) return true;
  if (m_duration                != right.m_duration) return true;
  if (m_strPlotOutline          != right.m_strPlotOutline) return true;
  if (m_strPlot                 != right.m_strPlot) return true;
  if (m_strFileNameAndPath      != right.m_strFileNameAndPath) return true;
  if (m_Priority                != right.m_Priority) return true;
  if (m_Lifetime                != right.m_Lifetime) return true;
  if (m_strTitle                != right.m_strTitle) return true;

  return false;
}

/**
 * Initialize blank cPVRRecordingInfoTag
 */
void cPVRRecordingInfoTag::Reset(void)
{
  m_clientIndex           = -1;
  m_clientID              = CPVRManager::GetInstance()->GetFirstClientID(); // Temporary until we support multiple backends
  m_strChannel            = "";
  m_recordingTime         = NULL;
  m_strFileNameAndPath    = "";
  m_Priority              = -1;
  m_Lifetime              = -1;

  CVideoInfoTag::Reset();
}

int cPVRRecordingInfoTag::DurationSeconds() const
{
  int duration;
  duration =  m_duration.GetDays()*60*60*24;
  duration += m_duration.GetHours()*60*60;
  duration += m_duration.GetMinutes()*60;
  duration += m_duration.GetSeconds();
  duration /= 60;
  return duration;
}
















// --- cPVRRecordings ---------------------------------------------------------------

cPVRRecordings PVRRecordings;

cPVRRecordings::cPVRRecordings(void)
{

}

bool cPVRRecordings::Load()
{
  CPVRManager *manager  = CPVRManager::GetInstance();
  CLIENTMAP   *clients  = manager->Clients();
  
  Clear();

  /* Go thru all clients and receive there Recordings */
  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    /* Load only if the client have Recordings */
    IPVRClient* client = (*itr).second;
    if (client->GetNumRecordings() > 0)
    {
      client->GetAllRecordings(this);
    }
    itr++;
  }

  for (unsigned int i = 0; i < size(); i++)
  {
    CStdString Path;
    Path.Format("record://%i", i+1);
    at(i).SetPath(Path);
  }

  return true;
}

bool cPVRRecordings::Update(bool Wait)
{
  Load();
}

int cPVRRecordings::GetNumRecordings()
{
  return size();
}

void cPVRRecordings::Clear()
{
  /* Clear all current present Recordings inside list */
  erase(begin(), end());
  return;
}
