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

#include <algorithm>
#include <array>
#include <optional>
#include <string>

namespace
{
constexpr int VERSION_LEGACY = 0; // nfo has no version attribute and predates version support

// Current nfo versions
constexpr int VERSION_MOVIE = 0;
constexpr int VERSION_TVSHOW = 0;
constexpr int VERSION_EPISODEDETAILS = 0;
constexpr int VERSION_MUSICVIDEOS = 0;

struct nfoDetails
{
  std::string_view m_tagName;
  int m_version;
  bool (*m_upgradeFn)(TiXmlElement*, int); //! @todo C++23 convert to std::function
};

bool UpgradeMovie(TiXmlElement* root, int currentVersion);
bool UpgradeTvShow(TiXmlElement* root, int currentVersion);
bool UpgradeEpisodeDetails(TiXmlElement* root, int currentVersion);
bool UpgradeMusicVideos(TiXmlElement* root, int currentVersion);

using namespace std::literals::string_view_literals;

// clang-format off
constexpr std::array<struct nfoDetails, 4> nfos{{
    {"movie"sv,                   VERSION_MOVIE,                   UpgradeMovie},
    {"tvshow"sv,                  VERSION_TVSHOW,                  UpgradeTvShow},
    {"episodedetails"sv,          VERSION_EPISODEDETAILS,          UpgradeEpisodeDetails},
    {"musicvideos"sv,             VERSION_MUSICVIDEOS,             UpgradeMusicVideos},
}};
// clang-format on

constexpr std::optional<nfoDetails> FindNfoParameters(std::string_view tag)
{
  if (const auto it{std::ranges::find(nfos, tag, &nfoDetails::m_tagName)}; it != nfos.end())
    return *it;
  else
    return std::nullopt;
}

constexpr int CurrentNfoVersion(std::string_view tag)
{
  const auto& param{FindNfoParameters(tag)};
  if (param)
    return param->m_version;

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
  const std::string type{root->ValueStr()};
  int version;
  if (root->QueryIntAttribute("version", &version) != TIXML_SUCCESS)
    version = VERSION_LEGACY;

  const int targetVersion = CurrentNfoVersion(type);

  if (targetVersion > VERSION_LEGACY && version < targetVersion)
  {
    CLog::LogF(LOGDEBUG, "upgrading {} from version {} to {}", type, version, targetVersion);

    root->SetAttribute("version", targetVersion);

    const auto& param{FindNfoParameters(type)};
    if (param)
      return param->m_upgradeFn(root, version);
  }

  return true;
}
