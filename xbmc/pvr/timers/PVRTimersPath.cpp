/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimersPath.h"

#include <cstdlib>
#include <vector>

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace PVR;

const std::string CPVRTimersPath::PATH_ADDTIMER = "pvr://timers/addtimer/";
const std::string CPVRTimersPath::PATH_NEW      = "pvr://timers/new/";

CPVRTimersPath::CPVRTimersPath(const std::string& strPath)
{
  Init(strPath);
}

CPVRTimersPath::CPVRTimersPath(const std::string& strPath, int iClientId, unsigned int iParentId)
{
  if (Init(strPath))
  {
    // set/replace client and parent id.
    m_path = StringUtils::Format("pvr://timers/%s/%s/%d/%d",
                                 m_bRadio      ? "radio" : "tv",
                                 m_bTimerRules ? "rules" : "timers",
                                 iClientId,
                                 iParentId);
    m_iClientId = iClientId;
    m_iParentId = iParentId;
    m_bRoot = false;
  }
}

CPVRTimersPath::CPVRTimersPath(bool bRadio, bool bTimerRules) :
  m_path(StringUtils::Format(
    "pvr://timers/%s/%s", bRadio ? "radio" : "tv", bTimerRules ? "rules" : "timers")),
  m_bValid(true),
  m_bRoot(true),
  m_bRadio(bRadio),
  m_bTimerRules(bTimerRules),
  m_iClientId(-1),
  m_iParentId(0)
{
}

bool CPVRTimersPath::Init(const std::string& strPath)
{
  std::string strVarPath(strPath);
  URIUtils::RemoveSlashAtEnd(strVarPath);

  m_path = strVarPath;
  const std::vector<std::string> segments = URIUtils::SplitPath(m_path);

  m_bValid   = (((segments.size() == 4) || (segments.size() == 6)) &&
                (segments.at(1) == "timers") &&
                ((segments.at(2) == "radio") || (segments.at(2) == "tv"))&&
                ((segments.at(3) == "rules") || (segments.at(3) == "timers")));
  m_bRoot    = (m_bValid && (segments.size() == 4));
  m_bRadio   = (m_bValid && (segments.at(2) == "radio"));
  m_bTimerRules = (m_bValid && (segments.at(3) == "rules"));

  if (!m_bValid || m_bRoot)
  {
    m_iClientId = -1;
    m_iParentId = 0;
  }
  else
  {
    m_iClientId = std::stoi(segments.at(4));
    m_iParentId = std::stoul(segments.at(5));
  }

  return m_bValid;
}
