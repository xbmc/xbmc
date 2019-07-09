/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoLoader.h"

#include "JobManager.h"
#include "TimeUtils.h"
#include "guilib/LocalizeStrings.h"

CInfoLoader::CInfoLoader(unsigned int timeToRefresh)
{
  m_refreshTime = 0;
  m_timeToRefresh = timeToRefresh;
  m_busy = false;
}

CInfoLoader::~CInfoLoader() = default;

void CInfoLoader::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_refreshTime = CTimeUtils::GetFrameTime() + m_timeToRefresh;
  m_busy = false;
}

std::string CInfoLoader::GetInfo(int info)
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

std::string CInfoLoader::BusyInfo(int info) const
{
  return g_localizeStrings.Get(503);
}

std::string CInfoLoader::TranslateInfo(int info) const
{
  return "";
}

void CInfoLoader::Refresh()
{
  m_refreshTime = CTimeUtils::GetFrameTime();
}

