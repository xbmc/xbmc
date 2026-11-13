/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameNameInfo.h"

#include "GameNameParser.h"
#include "ServiceBroker.h"
#include "games/tags/GameInfoTag.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"

#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
int StatusLabel(ReleaseStatus status)
{
  switch (status)
  {
    case ReleaseStatus::BETA:
      return 35739; // "Beta"
    case ReleaseStatus::PROTOTYPE:
      return 35740; // "Prototype"
    case ReleaseStatus::SAMPLE:
      return 35741; // "Sample"
    case ReleaseStatus::DEMO:
      return 35742; // "Demo"
    case ReleaseStatus::ALPHA:
      return 35743; // "Alpha"
    case ReleaseStatus::DEBUG_BUILD:
      return 35744; // "Debug"
    case ReleaseStatus::PROGRAM:
      return 35745; // "Program"
    default:
      return 0;
  }
}

int LicenceLabel(Licence licence)
{
  switch (licence)
  {
    case Licence::UNLICENSED:
      return 35746; // "Unlicensed"
    case Licence::AFTERMARKET:
      return 35747; // "Aftermarket"
    case Licence::HOMEBREW:
      return 35748; // "Homebrew"
    case Licence::PIRATE:
      return 35749; // "Pirate"
    default:
      return 0;
  }
}
} // namespace

void CGameNameInfo::SetInfoTag(CGameInfoTag& tag, const ParsedGameName& name)
{
  if (name.displayTitle.empty())
    return;

  tag.SetTitle(name.displayTitle);
  if (name.year > 0)
    tag.SetYear(static_cast<unsigned int>(name.year));
  tag.SetPublisher(name.publisher);

  std::vector<std::string> regions;
  for (const GameRegion& region : name.regions)
    regions.emplace_back(CGameNameParser::RegionName(region));
  tag.SetRegion(StringUtils::Join(regions, ", "));
}

std::string CGameNameInfo::VersionLabel(const ParsedGameName& name)
{
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  std::vector<std::string> parts;
  if (name.hack)
    parts.emplace_back(strings.Get(35737)); // "Hack"
  if (name.translated)
    parts.emplace_back(strings.Get(35738)); // "Fan translation"
  if (const int status = StatusLabel(name.status); status > 0)
    parts.emplace_back(strings.Get(status));
  if (const int licence = LicenceLabel(name.licence); licence > 0)
    parts.emplace_back(strings.Get(licence));
  if (!name.revision.empty())
    parts.emplace_back(name.revision);
  if (!name.regions.empty())
  {
    std::vector<std::string> regions;
    for (const GameRegion& region : name.regions)
      regions.emplace_back(CGameNameParser::RegionName(region));
    parts.emplace_back(StringUtils::Join(regions, ", "));
  }
  if (!name.languages.empty())
    parts.emplace_back(StringUtils::ToUpper(StringUtils::Join(name.languages, ", ")));
  if (name.alternate)
    parts.emplace_back(strings.Get(35750)); // "Alternate"
  if (name.bad)
    parts.emplace_back(strings.Get(35751)); // "Bad dump"

  if (parts.empty())
    return name.displayTitle;
  return StringUtils::Join(parts, " \u00b7 ");
}
