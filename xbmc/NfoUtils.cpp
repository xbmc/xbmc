/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NfoUtils.h"

#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <string>

namespace
{
constexpr int VERSION_LEGACY = 0; // nfo has no version attribute and predates version support

// Current nfo versions
constexpr int VERSION_MOVIE = 0;
constexpr int VERSION_TVSHOW = 0;
constexpr int VERSION_EPISODEDETAILS = 0;
constexpr int VERSION_MUSICVIDEOS = 0;

constexpr int CurrentNfoVersion(std::string_view tag)
{
  if (tag == "movie")
    return VERSION_MOVIE;
  if (tag == "tvshow")
    return VERSION_TVSHOW;
  if (tag == "episodedetails")
    return VERSION_EPISODEDETAILS;
  if (tag == "musicvideos")
    return VERSION_EPISODEDETAILS;

  return VERSION_LEGACY;
}

bool UpgradeMovie(TiXmlElement* root, int currentVersion)
{
  return true;
}

bool UpgradeTvShow(TiXmlElement* root, int currentVersion)
{
  return true;
}

bool UpgradeEpisodeDetails(TiXmlElement* root, int currentVersion)
{
  return true;
}

bool UpgradeMusicVideos(TiXmlElement* root, int currentVersion)
{
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
  if (targetVersion > VERSION_LEGACY && version < targetVersion)
  {
    CLog::LogF(LOGDEBUG, "upgrading {} from version {} to {}", type, version, targetVersion);

    root->SetAttribute("version", targetVersion);

    if (type == "movie")
      return UpgradeMovie(root, version);
    if (type == "tvshow")
      return UpgradeTvShow(root, version);
    if (type == "episodedetails")
      return UpgradeEpisodeDetails(root, version);
    if (type == "musicvideos")
      return UpgradeMusicVideos(root, version);
  }

  return true;
}
