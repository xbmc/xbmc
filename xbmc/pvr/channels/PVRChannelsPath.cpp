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
      {
        // special case denoting local all channels group
        if (segment == "*")
        {
          m_kind = Kind::GROUP; // pvr://channels/(tv|radio)/<all-channels-wildcard>@-1
          m_groupName = segment;
          m_groupClientID = -1; // local
          break;
        }

        std::vector<std::string> tokens = StringUtils::Split(segment, "@");
        if (tokens.size() == 2 && !tokens[0].empty() && !tokens[1].empty())
        {
          m_groupName = CURL::Decode(tokens[0]);

          std::string groupClientID = tokens[1];
          if (groupClientID.find_first_not_of("-0123456789") == std::string::npos)
          {
            m_groupClientID = std::atoi(groupClientID.c_str());
            if (m_groupClientID >= -1)
            {
              m_kind = Kind::GROUP; // pvr://channels/(tv|radio)/<groupname>@<clientid>
              break;
            }
          }
        }

        CLog::LogF(LOGERROR, "Invalid channels path '{}' - channel group segment syntax error.",
                   strPath);
        m_kind = Kind::INVALID;
        break;
      }

      case Kind::GROUP:
      {
        std::vector<std::string> tokens = StringUtils::Split(segment, "_");
        if (tokens.size() == 2)
        {
          std::vector<std::string> instance = StringUtils::Split(tokens[0], "@");
          if (instance.size() == 2)
          {
            m_instanceID = std::atoi(instance[0].c_str());
            m_addonID = instance[1];
          }
          else
          {
            m_instanceID = ADDON::ADDON_SINGLETON_INSTANCE_ID;
            m_addonID = tokens[0];
          }

          tokens = StringUtils::Split(tokens[1], ".");
          if (tokens.size() == 2 && tokens[1] == "pvr")
          {
            std::string channelUID = tokens[0];
            if (!channelUID.empty() && channelUID.find_first_not_of("0123456789") == std::string::npos)
              m_iChannelUID = std::atoi(channelUID.c_str());
          }
        }

        if (!m_addonID.empty() && m_iChannelUID >= 0)
        {
          m_kind = Kind::
              CHANNEL; // pvr://channels/(tv|radio)/<groupname>@<clientid>/<instanceid>@<addonid>_<channeluid>.pvr
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

CPVRChannelsPath::CPVRChannelsPath(bool bRadio,
                                   bool bHidden,
                                   const std::string& strGroupName,
                                   int iGroupClientID)
  : m_bRadio(bRadio), m_groupName(bHidden ? ".hidden" : strGroupName)
{
  if (m_groupName.empty())
  {
    m_kind = Kind::EMPTY;
    m_path = StringUtils::Format("pvr://channels/{}", bRadio ? "radio" : "tv");
  }
  else
  {
    m_kind = Kind::GROUP;
    m_groupClientID = iGroupClientID;
    m_path = StringUtils::Format("pvr://channels/{}/{}@{}/", bRadio ? "radio" : "tv",
                                 CURL::Encode(m_groupName), m_groupClientID);
  }
}

CPVRChannelsPath::CPVRChannelsPath(bool bRadio, const std::string& strGroupName, int iGroupClientID)
  : m_bRadio(bRadio), m_groupName(strGroupName)
{
  if (m_groupName.empty())
  {
    m_kind = Kind::EMPTY;
    m_path = StringUtils::Format("pvr://channels/{}", bRadio ? "radio" : "tv");
  }
  else
  {
    m_kind = Kind::GROUP;
    m_groupClientID = iGroupClientID;
    m_path = StringUtils::Format("pvr://channels/{}/{}@{}/", bRadio ? "radio" : "tv",
                                 CURL::Encode(m_groupName), m_groupClientID);
  }
}

CPVRChannelsPath::CPVRChannelsPath(bool bRadio,
                                   const std::string& strGroupName,
                                   int iGroupClientID,
                                   const std::string& strAddonID,
                                   ADDON::AddonInstanceId instanceID,
                                   int iChannelUID)
  : m_bRadio(bRadio)
{
  if (!strGroupName.empty() && iGroupClientID >= -1 && !strAddonID.empty() && iChannelUID >= 0)
  {
    m_kind = Kind::CHANNEL;
    m_groupName = strGroupName;
    m_groupClientID = iGroupClientID;
    m_addonID = strAddonID;
    m_instanceID = instanceID;
    m_iChannelUID = iChannelUID;
    m_path = StringUtils::Format("pvr://channels/{}/{}@{}/{}@{}_{}.pvr", bRadio ? "radio" : "tv",
                                 CURL::Encode(m_groupName), m_groupClientID, m_instanceID,
                                 m_addonID, m_iChannelUID);
  }
}

bool CPVRChannelsPath::IsHiddenChannelGroup() const
{
  return m_kind == Kind::GROUP && m_groupName == ".hidden";
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
