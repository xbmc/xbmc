/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgSearchPath.h"

#include "pvr/epg/EpgSearchFilter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <cstdlib>
#include <string>
#include <vector>

using namespace PVR;

const std::string CPVREpgSearchPath::PATH_SEARCH_DIALOG = "pvr://search/search_dialog";
const std::string CPVREpgSearchPath::PATH_TV_SEARCH = "pvr://search/tv/";
const std::string CPVREpgSearchPath::PATH_TV_SAVEDSEARCHES = "pvr://search/tv/savedsearches/";
const std::string CPVREpgSearchPath::PATH_RADIO_SEARCH = "pvr://search/radio/";
const std::string CPVREpgSearchPath::PATH_RADIO_SAVEDSEARCHES = "pvr://search/radio/savedsearches/";

CPVREpgSearchPath::CPVREpgSearchPath(const std::string& strPath)
{
  Init(strPath);
}

CPVREpgSearchPath::CPVREpgSearchPath(const CPVREpgSearchFilter& search)
  : m_path(StringUtils::Format("pvr://search/{}/savedsearches/{}",
                               search.IsRadio() ? "radio" : "tv",
                               search.GetDatabaseId())),
    m_bValid(true),
    m_bRadio(search.IsRadio())
{
}

bool CPVREpgSearchPath::Init(const std::string& strPath)
{
  std::string strVarPath(strPath);
  URIUtils::RemoveSlashAtEnd(strVarPath);

  m_path = strVarPath;
  const std::vector<std::string> segments = URIUtils::SplitPath(m_path);

  m_bValid =
      ((segments.size() >= 3) && (segments.size() <= 5) && (segments.at(1) == "search") &&
       ((segments.at(2) == "radio") || (segments.at(2) == "tv") || (segments.at(2) == "search")) &&
       ((segments.size() == 3) || (segments.at(3) == "savedsearches")));
  m_bRoot = (m_bValid && (segments.size() == 3) && (segments.at(2) != "search"));
  m_bRadio = (m_bValid && (segments.at(2) == "radio"));
  m_bSavedSearchesRoot =
      (m_bValid && (segments.size() == 4) && (segments.at(3) == "savedsearches"));
  m_bSavedSearch = (m_bValid && (segments.size() == 5));
  if (m_bSavedSearch)
    m_iId = std::stoi(segments.at(4));

  return m_bValid;
}
