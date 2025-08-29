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

#include <string>

namespace
{
constexpr int VERSION_LEGACY = 0; // nfo has no version attribute and predates version support

// Current nfo versions
constexpr int VERSION_MOVIE = 1;
constexpr int VERSION_TVSHOW = 1;
constexpr int VERSION_EPISODEDETAILS = 1;
constexpr int VERSION_MUSICVIDEOS = 1;

int CurrentNfoVersion(std::string_view tag)
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
