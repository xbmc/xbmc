/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"

#include <string>

namespace PVR
{

static constexpr int EPG_SEARCH_UNSET = -1;

struct PVREpgSearchData
{
  std::string m_strSearchTerm; /*!< The term to search for */
  bool m_bSearchInDescription = false; /*!< Search for strSearchTerm in the description too */
  int m_iGenreType = EPG_SEARCH_UNSET; /*!< The genre type for an entry */
  CDateTime m_startDateTime; /*!< The minimum start time for an entry */
  CDateTime m_endDateTime; /*!< The maximum end time for an entry */
  unsigned int m_iUniqueBroadcastId = EPG_TAG_INVALID_UID; /*!< The broadcastid to search for */

  void Reset();
};

} // namespace PVR
