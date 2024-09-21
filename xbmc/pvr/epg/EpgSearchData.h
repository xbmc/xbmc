/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <string>

namespace PVR
{

static constexpr int EPG_SEARCH_UNSET = -1;

struct PVREpgSearchData
{
  std::string m_strSearchTerm; /*!< The term to search for */
  bool m_bSearchInDescription = false; /*!< Search for strSearchTerm in the description too */
  bool m_bIncludeUnknownGenres = false; /*!< Whether to include unknown genres */
  int m_iGenreType = EPG_SEARCH_UNSET; /*!< The genre type for an entry */
  bool m_bIgnoreFinishedBroadcasts; /*!< True to ignore finished broadcasts, false if not */
  bool m_bIgnoreFutureBroadcasts; /*!< True to ignore future broadcasts, false if not */
  CDateTime m_startDateTime; /*!< The minimum start date and time for an entry */
  CDateTime m_endDateTime; /*!< The maximum end date and time for an entry */
  bool m_startAnyTime{true}; /*!< Match any start time */
  bool m_endAnyTime{true}; /*!< Match any end time */

  void Reset()
  {
    m_strSearchTerm.clear();
    m_bSearchInDescription = false;
    m_bIncludeUnknownGenres = false;
    m_iGenreType = EPG_SEARCH_UNSET;
    m_bIgnoreFinishedBroadcasts = true;
    m_bIgnoreFutureBroadcasts = false;
    m_startDateTime.SetValid(false);
    m_endDateTime.SetValid(false);
    m_startAnyTime = true;
    m_endAnyTime = true;
  }
};

} // namespace PVR
