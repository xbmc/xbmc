/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRProvidersPath.h"

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

using namespace PVR;

const std::string CPVRProvidersPath::PATH_TV_PROVIDERS = "pvr://providers/tv/";
const std::string CPVRProvidersPath::PATH_RADIO_PROVIDERS = "pvr://providers/radio/";
const std::string CPVRProvidersPath::CHANNELS = "channels";
const std::string CPVRProvidersPath::RECORDINGS = "recordings";

//pvr://providers/(tv|radio)/<provider-uid@client-id>/(channels|recordings)/

CPVRProvidersPath::CPVRProvidersPath(const std::string& path)
{
  Init(path);
}

CPVRProvidersPath::CPVRProvidersPath(CPVRProvidersPath::Kind kind,
                                     int clientId,
                                     int providerUid,
                                     const std::string& lastSegment /* = "" */)
{
  m_kind = kind;
  m_isValid = ((m_kind == Kind::RADIO) || (m_kind == Kind::TV));
  if (m_isValid)
  {
    if (lastSegment.empty())
    {
      m_path = StringUtils::Format("pvr://providers/{}/{}@{}/",
                                   (m_kind == Kind::RADIO) ? "radio" : "tv", providerUid, clientId);
      m_isProvider = true;
      m_isChannels = false;
      m_isRecordings = false;
    }
    else
    {
      m_path = StringUtils::Format("pvr://providers/{}/{}@{}/{}/",
                                   (m_kind == Kind::RADIO) ? "radio" : "tv", providerUid, clientId,
                                   lastSegment);
      m_isProvider = false;
      m_isChannels = (lastSegment == CHANNELS);
      m_isRecordings = (lastSegment == RECORDINGS);
    }

    m_clientId = clientId;
    m_providerUid = providerUid;
    m_isRoot = false;
  }
}

bool CPVRProvidersPath::Init(const std::string& path)
{
  std::string varPath(path);
  URIUtils::RemoveSlashAtEnd(varPath);

  m_path = varPath;
  const std::vector<std::string> segments{URIUtils::SplitPath(m_path)};

  m_isValid =
      ((segments.size() >= 3) && (segments.size() <= 5) && (segments.at(1) == "providers") &&
       ((segments.at(2) == "radio") || (segments.at(2) == "tv")));
  m_isRoot = (m_isValid && (segments.size() == 3));
  if (m_isValid)
    m_kind = (segments.at(2) == "radio") ? Kind::RADIO : Kind::TV;

  if (!m_isValid || m_isRoot)
  {
    m_providerUid = PVR_PROVIDER_INVALID_UID;
    m_clientId = PVR_CLIENT_INVALID_UID;
    m_isProvider = false;
    m_isChannels = false;
    m_isRecordings = false;
  }
  else
  {
    std::vector<std::string> tokens{StringUtils::Split(segments.at(3), "@")};
    if (tokens.size() == 2 && !tokens[0].empty() && !tokens[1].empty())
    {
      m_providerUid = std::stoi(tokens[0]);
      m_clientId = std::stoi(tokens[1]);
      m_isProvider = (segments.size() == 4);

      if (segments.size() == 5)
      {
        m_isChannels = (segments.at(4) == CHANNELS);
        m_isRecordings = !m_isChannels && (segments.at(4) == RECORDINGS);
        m_isValid = (m_isChannels || m_isRecordings);
      }
    }
    else
    {
      m_isValid = false;
    }
  }

  return m_isValid;
}
