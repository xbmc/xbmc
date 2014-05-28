/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef UTILS_INFOLOADER_H_INCLUDED
#define UTILS_INFOLOADER_H_INCLUDED
#include "InfoLoader.h"
#endif

#ifndef UTILS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define UTILS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif

#ifndef UTILS_JOBMANAGER_H_INCLUDED
#define UTILS_JOBMANAGER_H_INCLUDED
#include "JobManager.h"
#endif

#ifndef UTILS_TIMEUTILS_H_INCLUDED
#define UTILS_TIMEUTILS_H_INCLUDED
#include "TimeUtils.h"
#endif


CInfoLoader::CInfoLoader(unsigned int timeToRefresh)
{
  m_refreshTime = 0;
  m_timeToRefresh = timeToRefresh;
  m_busy = false;
}

CInfoLoader::~CInfoLoader()
{
}

void CInfoLoader::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_refreshTime = CTimeUtils::GetFrameTime() + m_timeToRefresh;
  m_busy = false;
}

CStdString CInfoLoader::GetInfo(int info)
{
  // Refresh if need be
  if (m_refreshTime < CTimeUtils::GetFrameTime() && !m_busy)
  { // queue up the job
    m_busy = true;
    CJobManager::GetInstance().AddJob(GetJob(), this);
  }
  if (m_busy && CTimeUtils::GetFrameTime() - m_refreshTime > 1000)
  {
    return BusyInfo(info);
  }
  return TranslateInfo(info);
}

CStdString CInfoLoader::BusyInfo(int info) const
{
  return g_localizeStrings.Get(503);
}

CStdString CInfoLoader::TranslateInfo(int info) const
{
  return "";
}

void CInfoLoader::Refresh()
{
  m_refreshTime = CTimeUtils::GetFrameTime();
}

