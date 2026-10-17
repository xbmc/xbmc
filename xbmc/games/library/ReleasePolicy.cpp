/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ReleasePolicy.h"

#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

using namespace KODI;
using namespace GAME;

namespace
{
//! The regions tried after the player's own, in order
constexpr std::array<std::string_view, 3> FALLBACK_REGIONS{"Europe", "USA", "Japan"};

//! The wider region whose releases a country gets
std::string_view WiderRegion(std::string_view region)
{
  if (region == "United Kingdom" || region == "Germany" || region == "France" ||
      region == "Spain" || region == "Italy" || region == "Australia")
    return "Europe";
  if (region == "Canada" || region == "Brazil")
    return "USA";
  if (region == "Korea")
    return "Asia";
  return "";
}
} // namespace

CReleasePolicy::CReleasePolicy(std::vector<std::string> regionPriority,
                               bool preferRetail,
                               bool preferNewestRevision,
                               bool preferVerified)
  : m_regionPriority(std::move(regionPriority)),
    m_preferRetail(preferRetail),
    m_preferNewestRevision(preferNewestRevision),
    m_preferVerified(preferVerified)
{
}

std::vector<std::string> CReleasePolicy::RegionPriority(const std::string& region)
{
  std::vector<std::string> regions;
  const auto add = [&regions](std::string_view name)
  {
    if (!name.empty() && std::ranges::find(regions, name) == regions.end())
      regions.emplace_back(name);
  };
  add(region);
  add(WiderRegion(region));
  add("World");
  for (std::string_view fallback : FALLBACK_REGIONS)
    add(fallback);
  return regions;
}

int CReleasePolicy::RegionRank(const GameRelease& release) const
{
  // The best-placed region a release covers; a release with no region sits
  // behind every listed one, and an unlisted region behind that
  int best = static_cast<int>(m_regionPriority.size()) + 1;
  for (const std::string& region : release.regions)
  {
    const auto it = std::ranges::find(m_regionPriority, region);
    if (it != m_regionPriority.end())
      best = std::min(best, static_cast<int>(it - m_regionPriority.begin()));
    else if (region == "World")
      best = std::min(best, static_cast<int>(m_regionPriority.size()));
  }
  if (release.regions.empty())
    best = static_cast<int>(m_regionPriority.size());
  return best;
}

int CReleasePolicy::StatusRank(const GameRelease& release)
{
  int rank = 0;
  if (release.status != ReleaseStatus::RETAIL || !release.edition.empty())
    rank += 2;
  if (release.licence == Licence::PIRATE)
    rank += 4;
  if (release.alternate)
    rank += 1;
  return rank;
}

int CReleasePolicy::RevisionRank(const GameRelease& release)
{
  // "Rev 2" > "Rev 1" > none; "v1.1" > "v1.0"; "Rev B" > "Rev A"
  std::string_view rev = release.revision;
  if (rev.empty())
    return 0;

  for (std::string_view word : {"revision", "version", "rev", "v"})
  {
    if (StringUtils::StartsWithNoCase(rev, word))
    {
      rev.remove_prefix(word.size());
      break;
    }
  }

  int value = 0;
  bool seenDigit = false;
  for (const char c : rev)
  {
    const auto u = static_cast<unsigned char>(c);
    if (std::isdigit(u))
    {
      value = value * 10 + (c - '0');
      seenDigit = true;
    }
    else if (std::isalpha(u) && !seenDigit)
      value = std::tolower(u) - 'a' + 1;
  }
  return value + 1;
}

int CReleasePolicy::DumpRank(const GameRelease& release)
{
  switch (release.dump)
  {
    case DumpStatus::VERIFIED:
      return 0;
    case DumpStatus::UNKNOWN:
    case DumpStatus::GOOD:
      return 1;
    case DumpStatus::BAD:
      return 3;
  }
  return 1;
}

bool CReleasePolicy::Prefers(const GameRelease& a, const GameRelease& b) const
{
  // A bad dump never plays when another exists
  if (DumpRank(a) != DumpRank(b) && (a.dump == DumpStatus::BAD || b.dump == DumpStatus::BAD))
    return DumpRank(a) < DumpRank(b);

  if (m_preferRetail && StatusRank(a) != StatusRank(b))
    return StatusRank(a) < StatusRank(b);

  if (RegionRank(a) != RegionRank(b))
    return RegionRank(a) < RegionRank(b);

  if (m_preferNewestRevision && RevisionRank(a) != RevisionRank(b))
    return RevisionRank(a) > RevisionRank(b);

  if (m_preferVerified && DumpRank(a) != DumpRank(b))
    return DumpRank(a) < DumpRank(b);

  return false;
}

int CReleasePolicy::PickDefault(const std::vector<GameRelease>& releases) const
{
  int best = -1;
  for (size_t i = 0; i < releases.size(); ++i)
  {
    if (releases[i].files.empty())
      continue;
    if (best < 0 || Prefers(releases[i], releases[static_cast<size_t>(best)]))
      best = static_cast<int>(i);
  }
  return best;
}
