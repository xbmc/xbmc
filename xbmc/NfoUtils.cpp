/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NfoUtils.h"

#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <array>
#include <optional>
#include <string>

namespace
{
using namespace std::literals::string_view_literals;

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

constexpr int VERSION_LEGACY = 0; // nfo has no version attribute and predates version support

// Current nfo versions
constexpr int VERSION_MOVIE = 1;
constexpr int VERSION_TVSHOW = 1;
constexpr int VERSION_EPISODEDETAILS = 1;
constexpr int VERSION_MUSICVIDEOS = 1;

// clang-format off
constexpr std::array<struct nfoDetails, 4> nfos{{
    {"movie"sv,                   VERSION_MOVIE,                   UpgradeMovie},
    {"tvshow"sv,                  VERSION_TVSHOW,                  UpgradeTvShow},
    {"episodedetails"sv,          VERSION_EPISODEDETAILS,          UpgradeEpisodeDetails},
    {"musicvideos"sv,             VERSION_MUSICVIDEOS,             UpgradeMusicVideos},
}};
// clang-format on

std::optional<nfoDetails> FindNfoParameters(std::string_view tag)
{
  auto it{std::find_if(nfos.begin(), nfos.end(),
                       [tag](const auto& nfo) { return nfo.m_tagName == tag; })};

  if (it != nfos.end())
    return *it;
  else
    return std::nullopt;
}

int CurrentNfoVersion(std::string_view tag)
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

  while (id != nullptr)
  {
    if (!hasUniqueId)
    {
      const std::string value = id->FirstChild()->ValueStr();
      if (!value.empty())
      {
        if (nullptr == XMLUtils::SetString(root, "uniqueid", value))
        {
          CLog::LogF(LOGERROR, "unable to add uniqueid tag, value {}", value);
          return false;
        }
      }
    }
    id = XMLUtils::RemoveAndReturnNextSibling(id, "id");
  }
  return true;
}

// When there are no "ratings" tags, convert <rating max="zzz">xxx</rating><votes>yyy</votes> tags to
// <ratings><rating max="zzz"><value>xxx</value><votes>yyy</votes></rating></ratings>
// All <rating> and <votes> tags are removed regardless.
bool ConvertRating(TiXmlElement* root)
{
  const bool hasRatings = (nullptr != root->FirstChildElement("ratings"));

  TiXmlElement* ratingElement = root->FirstChildElement("rating");

  if (!hasRatings && ratingElement != nullptr)
  {
    // Extract the information
    const char* rating = ratingElement->FirstChild()->Value();

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
    XMLUtils::SetString(&newRating, "value", rating);
    if (votes.has_value())
      XMLUtils::SetInt(&newRating, "votes", votes.value());

    TiXmlElement newRatings("ratings");
    newRatings.InsertEndChild(newRating);

    root->InsertEndChild(newRatings);
  }

  for (TiXmlNode* node = ratingElement; node != nullptr;)
    node = XMLUtils::RemoveAndReturnNextSibling(node, "rating");

  for (TiXmlNode* node = root->FirstChildElement("votes"); node != nullptr;)
    node = XMLUtils::RemoveAndReturnNextSibling(node, "votes");

  return true;
}

// Convert <set>Set Name</set> to <set><name>Set Name</name></set>
// New tags (if any) are preserved and prevent the generation of a new style tag from a legacy tag.
bool ConvertMovieSet(TiXmlElement* root)
{
  bool hasNewStyle{false};
  std::string legacyName;
  std::string dummy;

  TiXmlNode* node = root->FirstChildElement("set");
  while (node != nullptr)
  {
    // Identify new style <set><name>xxx</name></set>?
    if (XMLUtils::GetString(node, "name", dummy) && !dummy.empty())
    {
      hasNewStyle = true;
      // Keep and skip to next tag
      node = node->NextSibling("set");
      continue;
    }

    // No new style tag idenfied yet? Look for legacy <set>xxx</set> tag
    if (!hasNewStyle)
    {
      const TiXmlNode* child = node->FirstChild();
      if (child != nullptr && child->Type() == TiXmlNode::TINYXML_TEXT)
      {
        const std::string name = child->ValueStr();
        if (!name.empty() && legacyName.empty())
          legacyName = name;
      }
    }
    node = XMLUtils::RemoveAndReturnNextSibling(node, "set");
  }

  // No new style tag and a legacy tag found? Create a new style tag from the legacy info.
  if (!hasNewStyle && !legacyName.empty())
  {
    TiXmlElement set("set");
    XMLUtils::SetString(&set, "name", legacyName);
    root->InsertEndChild(set);
  }

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

    if (!ConvertMovieSet(root))
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
