/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameVersions.h"

#include "GameNameParser.h"
#include "ReleasePolicy.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <utility>

using namespace KODI;
using namespace GAME;

GameRelease CGameVersions::Release(const ParsedGameName& name)
{
  GameRelease release;
  release.regions = name.regions;
  release.revision = name.revision;
  release.status = name.status;
  release.licence = name.licence;
  release.alternate = name.alternate;
  release.modified = name.hack || name.translated;
  if (name.bad)
    release.dump = DumpStatus::BAD;
  else if (name.verified)
    release.dump = DumpStatus::VERIFIED;
  return release;
}

std::vector<std::vector<size_t>> CGameVersions::Group(const std::vector<ParsedGameName>& names,
                                                      const CReleasePolicy& policy)
{
  std::vector<GameRelease> releases;
  releases.reserve(names.size());
  for (const ParsedGameName& name : names)
    releases.emplace_back(Release(name));

  // Two platforms can each have a game of the same name, so the file's format counts too
  std::map<std::tuple<std::string, int, int, std::string>, std::vector<size_t>> games;
  for (size_t i = 0; i < names.size(); ++i)
  {
    const ParsedGameName& name = names[i];
    if (name.displayTitle.empty() || name.dataDisk)
      continue;
    games[{StringUtils::ToLower(name.displayTitle), name.disc, name.side, name.extension}]
        .emplace_back(i);
  }

  std::vector<std::vector<size_t>> groups;
  for (auto& [game, versions] : games)
  {
    if (versions.size() < 2)
      continue;
    std::ranges::stable_sort(versions, [&policy, &releases](size_t a, size_t b)
                             { return policy.Prefers(releases[a], releases[b]); });
    groups.emplace_back(std::move(versions));
  }
  return groups;
}
