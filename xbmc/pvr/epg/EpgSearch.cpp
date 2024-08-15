/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgSearch.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgSearchFilter.h"

#include <algorithm>
#include <mutex>

using namespace PVR;

void CPVREpgSearch::Execute()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags{
      CServiceBroker::GetPVRManager().EpgContainer().GetTags(m_filter.GetEpgSearchData())};
  m_filter.SetEpgSearchDataFiltered();

  // Tags can still contain false positives, for search criteria that cannot be handled via
  // database. So, run extended search filters on what we got from the database.
  for (auto it = tags.cbegin(); it != tags.cend();)
  {
    it = tags.erase(std::remove_if(tags.begin(), tags.end(),
                                   [this](const std::shared_ptr<const CPVREpgInfoTag>& entry)
                                   { return !m_filter.FilterEntry(entry); }),
                    tags.cend());
  }

  if (m_filter.ShouldRemoveDuplicates())
    m_filter.RemoveDuplicates(tags);

  m_filter.SetLastExecutedDateTime(CDateTime::GetUTCDateTime());

  m_results = tags;
}

const std::vector<std::shared_ptr<CPVREpgInfoTag>>& CPVREpgSearch::GetResults() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_results;
}
