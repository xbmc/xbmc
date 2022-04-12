/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgSearchData.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "utils/log.h"

using namespace PVR;

void PVREpgSearchData::Reset()
{
  m_strSearchTerm.clear();
  m_bSearchInDescription = false;
  m_bIncludeUnknownGenres = false;
  m_iGenreType = EPG_SEARCH_UNSET;

  m_startDateTime.SetFromUTCDateTime(
      CServiceBroker::GetPVRManager().EpgContainer().GetFirstEPGDate());
  if (!m_startDateTime.IsValid())
  {
    CLog::Log(LOGWARNING, "No valid epg start time. Defaulting search start time to 'now'");
    m_startDateTime.SetFromUTCDateTime(CDateTime::GetUTCDateTime()); // default to 'now'
  }

  m_endDateTime.SetFromUTCDateTime(CServiceBroker::GetPVRManager().EpgContainer().GetLastEPGDate());
  if (!m_endDateTime.IsValid())
  {
    CLog::Log(
        LOGWARNING,
        "No valid epg end time. Defaulting search end time to search start time plus 10 days");
    m_endDateTime.SetFromUTCDateTime(m_startDateTime +
                                     CDateTimeSpan(10, 0, 0, 0)); // default to start + 10 days
  }

  m_iUniqueBroadcastId = EPG_TAG_INVALID_UID;
}
