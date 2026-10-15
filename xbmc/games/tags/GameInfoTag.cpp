/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameInfoTag.h"

#include "utils/Archive.h"
#include "utils/Variant.h"

#include <string>

using namespace KODI;
using namespace GAME;

void CGameInfoTag::Reset()
{
  m_bLoaded = false;
  m_strURL.clear();
  m_strTitle.clear();
  m_strPlatform.clear();
  m_genres.clear();
  m_strDeveloper.clear();
  m_strOverview.clear();
  m_year = 0;
  m_strID.clear();
  m_strRegion.clear();
  m_strPublisher.clear();
  m_strFormat.clear();
  m_strCartridgeType.clear();
  m_strGameClient.clear();
  m_strOriginalTitle.clear();
  m_rating = 0.0f;
  m_players = 0;
  m_strAgeRating.clear();
  m_strTrailer.clear();
  m_tags.clear();
}

CGameInfoTag& CGameInfoTag::operator=(const CGameInfoTag& tag)
{
  if (this != &tag)
  {
    m_bLoaded = tag.m_bLoaded;
    m_strURL = tag.m_strURL;
    m_strTitle = tag.m_strTitle;
    m_strPlatform = tag.m_strPlatform;
    m_genres = tag.m_genres;
    m_strDeveloper = tag.m_strDeveloper;
    m_strOverview = tag.m_strOverview;
    m_year = tag.m_year;
    m_strID = tag.m_strID;
    m_strRegion = tag.m_strRegion;
    m_strPublisher = tag.m_strPublisher;
    m_strFormat = tag.m_strFormat;
    m_strCartridgeType = tag.m_strCartridgeType;
    m_strGameClient = tag.m_strGameClient;
    m_strOriginalTitle = tag.m_strOriginalTitle;
    m_rating = tag.m_rating;
    m_players = tag.m_players;
    m_strAgeRating = tag.m_strAgeRating;
    m_strTrailer = tag.m_strTrailer;
    m_tags = tag.m_tags;
  }
  return *this;
}

bool CGameInfoTag::operator==(const CGameInfoTag& tag) const
{
  if (this != &tag)
  {
    if (m_bLoaded != tag.m_bLoaded)
      return false;

    if (m_bLoaded)
    {
      if (m_strURL != tag.m_strURL)
        return false;
      if (m_strTitle != tag.m_strTitle)
        return false;
      if (m_strPlatform != tag.m_strPlatform)
        return false;
      if (m_genres != tag.m_genres)
        return false;
      if (m_strDeveloper != tag.m_strDeveloper)
        return false;
      if (m_strOverview != tag.m_strOverview)
        return false;
      if (m_year != tag.m_year)
        return false;
      if (m_strID != tag.m_strID)
        return false;
      if (m_strRegion != tag.m_strRegion)
        return false;
      if (m_strPublisher != tag.m_strPublisher)
        return false;
      if (m_strFormat != tag.m_strFormat)
        return false;
      if (m_strCartridgeType != tag.m_strCartridgeType)
        return false;
      if (m_strGameClient != tag.m_strGameClient)
        return false;
      if (m_strOriginalTitle != tag.m_strOriginalTitle)
        return false;
      if (m_rating != tag.m_rating)
        return false;
      if (m_players != tag.m_players)
        return false;
      if (m_strAgeRating != tag.m_strAgeRating)
        return false;
      if (m_strTrailer != tag.m_strTrailer)
        return false;
      if (m_tags != tag.m_tags)
        return false;
    }
  }
  return true;
}

void CGameInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_bLoaded;
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_strPlatform;
    ar << m_genres;
    ar << m_strDeveloper;
    ar << m_strOverview;
    ar << m_year;
    ar << m_strID;
    ar << m_strRegion;
    ar << m_strPublisher;
    ar << m_strFormat;
    ar << m_strCartridgeType;
    ar << m_strGameClient;
    ar << m_strOriginalTitle;
    ar << m_rating;
    ar << m_players;
    ar << m_strAgeRating;
    ar << m_strTrailer;
    ar << m_tags;
  }
  else
  {
    ar >> m_bLoaded;
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_strPlatform;
    ar >> m_genres;
    ar >> m_strDeveloper;
    ar >> m_strOverview;
    ar >> m_year;
    ar >> m_strID;
    ar >> m_strRegion;
    ar >> m_strPublisher;
    ar >> m_strFormat;
    ar >> m_strCartridgeType;
    ar >> m_strGameClient;
    ar >> m_strOriginalTitle;
    ar >> m_rating;
    ar >> m_players;
    ar >> m_strAgeRating;
    ar >> m_strTrailer;
    ar >> m_tags;
  }
}

void CGameInfoTag::Serialize(CVariant& value) const
{
  value["loaded"] = m_bLoaded;
  value["url"] = m_strURL;
  value["name"] = m_strTitle;
  value["platform"] = m_strPlatform;
  value["genres"] = m_genres;
  value["developer"] = m_strDeveloper;
  value["overview"] = m_strOverview;
  value["year"] = m_year;
  value["id"] = m_strID;
  value["region"] = m_strRegion;
  value["publisher"] = m_strPublisher;
  value["format"] = m_strFormat;
  value["cartridgetype"] = m_strCartridgeType;
  value["gameclient"] = m_strGameClient;
  value["originaltitle"] = m_strOriginalTitle;
  value["rating"] = m_rating;
  value["players"] = m_players;
  value["agerating"] = m_strAgeRating;
  value["trailer"] = m_strTrailer;
  value["tags"] = m_tags;
}

void CGameInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  switch (field)
  {
    case Field::TITLE:
      if (!m_strTitle.empty())
        sortable[Field::TITLE] = m_strTitle;
      break;
    case Field::YEAR:
      sortable[Field::YEAR] = m_year;
      break;
    case Field::GENRE:
      sortable[Field::GENRE] = m_genres;
      break;
    case Field::ORIGINAL_TITLE:
      sortable[Field::ORIGINAL_TITLE] = m_strOriginalTitle;
      break;
    case Field::RATING:
      sortable[Field::RATING] = m_rating;
      break;
    default:
      break;
  }
}
