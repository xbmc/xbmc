/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgGuidePath.h"

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <cstdlib>
#include <string>
#include <vector>

using namespace PVR;

CPVREpgGuidePath::CPVREpgGuidePath(int epgId, const CDateTime& startDateTime)
  : m_path(StringUtils::Format("pvr://guide/{:04}/{}.epg", epgId, startDateTime.GetAsDBDateTime())),
    m_isValid(startDateTime.IsValid()),
    m_epgId(epgId),
    m_startDateTime(startDateTime)
{
}

CPVREpgGuidePath::CPVREpgGuidePath(const std::string& path) : m_path(path)
{
  const std::vector<std::string> segments{URIUtils::SplitPath(m_path)};

  m_isValid = (segments.size() == 4 && segments.at(1) == "guide" && segments.at(2).size() == 4 &&
               StringUtils::IsNaturalNumber(segments.at(2)) && segments.at(3).ends_with(".epg"));
  m_epgId = std::atoi(segments.at(2).c_str());
  m_startDateTime = CDateTime::FromDBDateTime(segments.at(3).substr(0, segments.at(3).size() - 4));

  m_isValid &= m_startDateTime.IsValid();
}
