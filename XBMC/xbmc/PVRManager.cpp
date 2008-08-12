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

#include "stdafx.h"
#include "PVRManager.h"
#include "utils/PVRClient-mythtv.h"
#include "GUISettings.h"
#include "Application.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/EPGInfoTag.h"
#include "Playlist.h"

using namespace PLAYLIST;

#define XBMC_PVRMANAGER_VERSION "0.1"

CPVRManager* CPVRManager::m_instance=NULL;
bool CPVRManager::m_hasRecordings=true; /// fix this


CPVRManager::CPVRManager()
{
  m_hasRecordings = true;
}

CPVRManager::~CPVRManager()
{
  CLog::Log(LOGINFO,"pvrmanager destroyed");
}

void CPVRManager::RemoveInstance()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance=NULL;
  }
}

void CPVRManager::Start()
{
  if (g_guiSettings.GetString("pvrmanager.serverip").IsEmpty() ||
      g_guiSettings.GetString("pvrmanager.username").IsEmpty() ||
      g_guiSettings.GetString("pvrmanager.password").IsEmpty() ||
      g_guiSettings.GetString("pvrmanager.username").IsEmpty())
  {
    return;
  }

  // determine which plugins are enabled
  // init and add each IPVRClient to a vector
  m_client = new PVRClientMythTv(0, this);
  CFileItemList recordings;
  m_client->GetRecordingSchedules(recordings);
  recordings.Clear();
  m_client->GetUpcomingRecordings(recordings);
  recordings.Clear();
  m_client->GetConflicting(recordings);
  int breakpoint = 1;
}

void CPVRManager::Stop()
{

}

CPVRManager* CPVRManager::GetInstance()
{
  if (!m_instance)
    m_instance = new CPVRManager();

  return m_instance;
}

void CPVRManager::ReleaseInstance()
{
  m_instance = NULL; /// check is this enough?
}

void CPVRManager::OnMessage(int event, const std::string& data)
{
  switch (event) {
    case PVR_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVRClient unknown event (error?)", __FUNCTION__);
      break;
    case PVR_EVENT_SCHEDULE_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVRClient schedule change: %s", __FUNCTION__, data);
      break;
  }
}