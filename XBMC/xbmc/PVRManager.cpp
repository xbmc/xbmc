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
#include "Application.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "VideoInfoTag.h"
#include "Playlist.h"

using namespace PLAYLIST;

#define XBMC_PVRMANAGER_VERSION "0.1"

CPVRManager* CPVRManager::m_pInstance=NULL;

CPVRManager::CPVRManager()
{
  m_hWorkerEvent = CreateEvent(NULL, false, false, NULL);
  m_LiveTVQueue = new CPlayList;
}

CPVRManager::~CPVRManager()
{
  CloseHandle(m_hWorkerEvent);
  StopThread();
  CLog::Log(LOGINFO,"pvrmanager destroyed");
  delete m_LiveTVQueue;
}

void CPVRManager::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance=NULL;
  }
}

CPVRManager* CPVRManager::GetInstance()
{
  if (!m_pInstance)
    m_pInstance=new CPVRManager;

  return m_pInstance;
}

//void CPVRManager::Parameter(const CStdString& key, const CStdString& data, CStdString& value)
//{
//  value = "";
//  vector<CStdString> params;
//  StringUtils::SplitString(data, "\n", params);
//  for (int i = 0; i < (int)params.size(); i++)
//  {
//    CStdString tmp = params[i];
//    if (int pos = tmp.Find(key) >= 0)
//    {
//      tmp.Delete(pos - 1, key.GetLength() + 1);
//      value = tmp;
//      break;
//    }
//  }
//  CLog::Log(LOGDEBUG, "Parameter %s -> %s", key.c_str(), value.c_str());
//}

void CPVRManager::OnStartup()
{
  SetPriority(THREAD_PRIORITY_NORMAL);
}

//void CPVRManager::FillGuideGrid(EPGGuideGrid* grid, int numDaysOffset)
//{
//
//}

void CPVRManager::Process()
{
 
}