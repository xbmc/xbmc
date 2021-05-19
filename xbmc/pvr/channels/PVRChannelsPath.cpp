/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelsPath.h"

#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>

using namespace PVR;

const std::string CPVRChannelsPath::PATH_TV_CHANNELS = "pvr://channels/tv/";
const std::string CPVRChannelsPath::PATH_RADIO_CHANNELS = "pvr://channels/radio/";


CPVRChannelsPath::CPVRChannelsPath(const std::string& strPath)
{
  std::string strVarPath = TrimSlashes(strPath);
  const std::vector<std::string> segments = URIUtils::SplitPath(strVarPath);

  for (const std::string& segment : segments)
  {
    switch (m_kind)
    {
      case Kind::INVALID:
        if (segment == "pvr://")
          m_kind = Kind::PROTO; // pvr:// followed by something => go on
        else if (segment == "pvr:" && segments.size() == 1) // just pvr:// => invalid
          strVarPath = "pvr:/";
        break;

      case Kind::PROTO:
        if (segment == "channels")
          m_kind = Kind::EMPTY; // pvr://channels
        else
          m_kind = Kind::INVALID;
        break;

      case Kind::EMPTY:
        if (segment == "tv" || segment == "radio")
        {
          m_kind = Kind::ROOT; // pvr://channels/(tv|radio)
          m_bRadio = (segment == "radio");
        }
        else
        {
          CLog::LogF(LOGERROR, "Invalid channels path '{}' - channel root segment syntax error.",
                     strPath);
          m_kind = Kind::INVALID;
        }
        break;

      case Kind::ROOT:
        m_kind = Kind::GROUP; // pvr://channels/(tv|radio)/<groupname>
        m_group = CURL::Decode(segment);
        break;

      case Kind::GROUP:
      {
        std::vector<std::string> tokens = StringUtils::Split(segment, "_");
        if (tokens.size() == 2)
        {
          m_clientID = tokens[0];
          tokens = StringUtils::Split(tokens[1], ".");
          if (tokens.size() == 2 && tokens[1] == "pvr")
          {
            std::string channelUID = tokens[0];
            if (!channelUID.empty() && channelUID.find_first_not_of("0123456789") == std::string::npos)
              m_iChannelUID = std::atoi(channelUID.c_str());
          }
        }

        if (!m_clientID.empty() && m_iChannelUID >= 0)
        {
          m_kind = Kind::CHANNEL; // pvr://channels/(tv|radio)/<groupname>/<addonid>_<channeluid>.pvr
        }
        else
        {
          CLog::LogF(LOGERROR, "Invalid channels path '{}' - channel segment syntax error.",
                     strPath);
          m_kind = Kind::INVALID;
        }
        break;
      }

      case Kind::CHANNEL:
        CLog::LogF(LOGERROR, "Invalid channels path '{}' - too many path segments.", strPath);
        m_kind = Kind::INVALID; // too many segments
        break;
    }

    if (m_kind == Kind::INVALID)
      break;
  }

  // append slash to all folders
  if (m_kind < Kind::CHANNEL)
    strVarPath.append("/");

  m_path = strVarPath;
}

CPVRChannelsPath::CPVRChannelsPath(bool bRadio, bool bHidden, const std::string& strGroupName)
  : m_bRadio(bRadio)
{
  if (!bHidden && strGroupName.empty())
    m_kind = Kind::EMPTY;
  else
    m_kind = Kind::GROUP;

  m_group = bHidden ? ".hidden" : strGroupName;
  m_path =
      StringUtils::Format("pvr://channels/{}/{}", bRadio ? "radio" : "tv", CURL::Encode(m_group));

  if (!m_group.empty())
    m_path.append("/");
}

CPVRChannelsPath::CPVRChannelsPath(bool bRadio, const std::string& strGroupName)
  : m_bRadio(bRadio)
{
  if (strGroupName.empty())
    m_kind = Kind::EMPTY;
  else
    m_kind = Kind::GROUP;

  m_group = strGroupName;
  m_path =
      StringUtils::Format("pvr://channels/{}/{}", bRadio ? "radio" : "tv", CURL::Encode(m_group));

  if (!m_group.empty())
    m_path.append("/");
}

CPVRChannelsPath::CPVRChannelsPath(bool bRadio, const std::string& strGroupName, const std::string& strClientID, int iChannelUID)
  : m_bRadio(bRadio)
{
  if (!strGroupName.empty() && !strClientID.empty() && iChannelUID >= 0)
  {
    m_kind = Kind::CHANNEL;
    m_group = strGroupName;
    m_clientID = strClientID;
    m_iChannelUID = iChannelUID;
    m_path = StringUtils::Format("pvr://channels/{}/{}/{}_{}.pvr", bRadio ? "radio" : "tv",
                                 CURL::Encode(m_group), m_clientID, m_iChannelUID);
  }
}

bool CPVRChannelsPath::IsHiddenChannelGroup() const
{
  return m_kind == Kind::GROUP && m_group == ".hidden";
}

std::string CPVRChannelsPath::TrimSlashes(const std::string& strString)
{
  std::string strTrimmed = strString;
  while (!strTrimmed.empty() && strTrimmed.front() == '/')
    strTrimmed.erase(0, 1);

  while (!strTrimmed.empty() && strTrimmed.back() == '/')
    strTrimmed.pop_back();

  return strTrimmed;
}
