/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NfoUtils.h"

#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <string>

constexpr int VERSION_LEGACY = 0; // nfo has no version attribute and predates version support
constexpr int VERSION_TVSHOW = 1;

namespace
{
int CurrentNfoVersion(std::string_view tag)
{
  //! @todo refactor in a generic manner once more than one tag type has to be handled
  if (tag == "tvshow")
    return VERSION_TVSHOW;

  return VERSION_LEGACY;
}

bool UpgradeTvShow(TiXmlElement* root, int currentVersion)
{
  if (currentVersion < 1)
  {
    // Convert from <namedseason number="1">Season Name</namedseason>
    // to <seasondetails number="1"><name>SeasonName</name></seasondetails>

    TiXmlElement* namedSeason = root->FirstChildElement("namedseason");
    while (namedSeason != nullptr)
    {
      if (namedSeason->FirstChild() != nullptr)
      {
        int number;
        std::string name = namedSeason->FirstChild()->ValueStr();
        if (!name.empty() && namedSeason->Attribute("number", &number) != nullptr)
        {
          TiXmlElement season("seasondetails");
          season.SetAttribute("number", number);
          TiXmlNode* node = root->InsertEndChild(season);
          if (nullptr == node)
          {
            CLog::LogF(LOGERROR, "unable to add seasondetails for season {}", number);
            return false;
          }
          if (nullptr == XMLUtils::SetString(node, "name", name))
          {
            CLog::LogF(LOGERROR, "unable to set name {} for season {}", name, number);
            return false;
          }
        }
      }

      TiXmlElement* next = namedSeason->NextSiblingElement("namedseason");

      if (!root->RemoveChild(namedSeason))
        return false;

      namedSeason = next;
    }
  }

  return true;
}
} // namespace

void CNfoUtils::SetVersion(TiXmlElement& elem, std::string_view tag)
{
  const int version{CurrentNfoVersion(tag)};

  if (version > VERSION_LEGACY)
    elem.SetAttribute("version", version);
}

bool CNfoUtils::Upgrade(TiXmlElement* root)
{
  const std::string type = root->ValueStr();
  int version;
  if (root->QueryIntAttribute("version", &version) != TIXML_SUCCESS)
    version = VERSION_LEGACY;

  const int targetVersion = CurrentNfoVersion(type);

  //! @todo refactor in a generic manner once more than one tag type has to be handled
  if (targetVersion > VERSION_LEGACY && version < targetVersion && type == "tvshow")
  {
    CLog::LogF(LOGDEBUG, "upgrading {} from version {} to {}", type, version, targetVersion);
    return UpgradeTvShow(root, version);
  }

  return true;
}
