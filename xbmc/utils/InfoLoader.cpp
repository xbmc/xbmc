/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoLoader.h"

#include "ServiceBroker.h"
#include "TimeUtils.h"
#include "guilib/LocalizeStrings.h"
#include "jobs/JobManager.h"

CInfoLoader::CInfoLoader(unsigned int timeToRefresh)
{
  m_refreshTime = 0;
  m_timeToRefresh = timeToRefresh;
}

CInfoLoader::~CInfoLoader() = default;

void CInfoLoader::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_refreshTime = CTimeUtils::GetFrameTime() + m_timeToRefresh;
  m_busy = false;
}

bool CInfoLoader::RefreshIfNeeded()
{
  if (m_refreshTime < CTimeUtils::GetFrameTime() && !m_busy)
  {
    // queue up data refresh job
    m_busy = true;
    CServiceBroker::GetJobManager()->AddJob(GetJob(), this);
    return true;
  }
  return false;
}

std::string CInfoLoader::GetInfo(int info)
{
  RefreshIfNeeded();

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

