/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NfoUtils.h"

#include "utils/StringUtils.h"
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
constexpr int VERSION_MOVIE = 1;
constexpr int VERSION_TVSHOW = 1;
constexpr int VERSION_EPISODEDETAILS = 1;
constexpr int VERSION_MUSICVIDEOS = 1;

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

// When there are no uniqueid tags, convert <id>xxx</id> tags to <uniqueid>xxx</uniqueid>
// All <id> tags are removed regardless.
bool ConvertIdToUniqueId(TiXmlElement* root)
{
  const bool hasUniqueId = (nullptr != root->FirstChildElement("uniqueid"));

  TiXmlNode* id = root->FirstChildElement("id");

  // The original code read only the first <id> value. Convert the first, remove the rest
  if (!hasUniqueId && id != nullptr)
  {
    if (const TiXmlNode* idChild = id->FirstChild(); idChild != nullptr)
    {
      if (std::string value = idChild->ValueStr(); !value.empty())
      {
        if (nullptr == XMLUtils::SetString(root, "uniqueid", value))
        {
          CLog::LogF(LOGERROR, "unable to add uniqueid tag, value {}", value);
          return false;
        }
      }
    }
  }

  for (TiXmlNode* node = id; node != nullptr;)
    node = XMLUtils::RemoveAndReturnNextSibling(node, "id");

  return true;
}

// When there are no "ratings" tags, convert <rating max="zzz">xxx</rating><votes>yyy</votes> tags to
// <ratings><rating max="zzz"><value>xxx</value><votes>yyy</votes></rating></ratings>
// All <rating> and <votes> tags are removed regardless.
bool ConvertRating(TiXmlElement* root)
{
  const bool hasRatings = (nullptr != root->FirstChildElement("ratings"));

  TiXmlElement* ratingElement = root->FirstChildElement("rating");

  // Convert only when <rating> exists and has a value
  if (!hasRatings && ratingElement != nullptr)
  {
    if (TiXmlNode* rating = ratingElement->FirstChild(); rating != nullptr)
    {
      // Extract the information
      const std::string ratingValue = rating->Value();

      std::optional<int> votes;
      std::string value;
      if (XMLUtils::GetString(root, "votes", value))
        votes = StringUtils::ReturnDigits(value);

      std::string maxValue;
      ratingElement->QueryStringAttribute("max", &maxValue);

      // Create new node
      TiXmlElement newRating("rating");
      if (!maxValue.empty())
        newRating.SetAttribute("max", maxValue);
      XMLUtils::SetString(&newRating, "value", ratingValue);
      if (votes.has_value())
        XMLUtils::SetInt(&newRating, "votes", votes.value());

      TiXmlElement newRatings("ratings");
      newRatings.InsertEndChild(newRating);

      root->InsertEndChild(newRatings);
    }
  }

  for (TiXmlNode* node = ratingElement; node != nullptr;)
    node = XMLUtils::RemoveAndReturnNextSibling(node, "rating");

  for (TiXmlNode* node = root->FirstChildElement("votes"); node != nullptr;)
    node = XMLUtils::RemoveAndReturnNextSibling(node, "votes");

  return true;
}

bool UpgradeMovie(TiXmlElement* root, int currentVersion)
{
  if (currentVersion < 1)
  {
    if (!ConvertIdToUniqueId(root))
      return false;

    if (!ConvertRating(root))
      return false;
  }
  return true;
}

bool UpgradeTvShow(TiXmlElement* root, int currentVersion)
{
  if (currentVersion < 1)
  {
    if (!ConvertIdToUniqueId(root))
      return false;

    if (!ConvertRating(root))
      return false;
  }
  return true;
}

bool UpgradeEpisodeDetails(TiXmlElement* root, int currentVersion)
{
  if (currentVersion < 1)
  {
    if (!ConvertIdToUniqueId(root))
      return false;

    if (!ConvertRating(root))
      return false;
  }
  return true;
}

bool UpgradeMusicVideos(TiXmlElement* root, int currentVersion)
{
  if (currentVersion < 1)
  {
    if (!ConvertIdToUniqueId(root))
      return false;

    if (!ConvertRating(root))
      return false;
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
