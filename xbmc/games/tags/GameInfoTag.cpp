/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
}

CGameInfoTag& CGameInfoTag::operator=(const CGameInfoTag& tag)
{
  if (this != &tag)
  {
    m_bLoaded          = tag.m_bLoaded;
    m_strURL           = tag.m_strURL;
    m_strTitle         = tag.m_strTitle;
    m_strPlatform      = tag.m_strPlatform;
    m_genres           = tag.m_genres;
    m_strDeveloper     = tag.m_strDeveloper;
    m_strOverview      = tag.m_strOverview;
    m_year             = tag.m_year;
    m_strID            = tag.m_strID;
    m_strRegion        = tag.m_strRegion;
    m_strPublisher     = tag.m_strPublisher;
    m_strFormat        = tag.m_strFormat;
    m_strCartridgeType = tag.m_strCartridgeType;
    m_strGameClient    = tag.m_strGameClient;
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
      if (m_strURL           != tag.m_strURL)           return false;
      if (m_strTitle         != tag.m_strTitle)         return false;
      if (m_strPlatform      != tag.m_strPlatform)      return false;
      if (m_genres           != tag.m_genres)           return false;
      if (m_strDeveloper     != tag.m_strDeveloper)     return false;
      if (m_strOverview      != tag.m_strOverview)      return false;
      if (m_year             != tag.m_year)             return false;
      if (m_strID            != tag.m_strID)            return false;
      if (m_strRegion        != tag.m_strRegion)        return false;
      if (m_strPublisher     != tag.m_strPublisher)     return false;
      if (m_strFormat        != tag.m_strFormat)        return false;
      if (m_strCartridgeType != tag.m_strCartridgeType) return false;
      if (m_strGameClient    != tag.m_strGameClient)    return false;
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
  }
}

void CGameInfoTag::Serialize(CVariant& value) const
{
  value["loaded"]        = m_bLoaded;
  value["url"]           = m_strURL;
  value["name"]          = m_strTitle;
  value["platform"]      = m_strPlatform;
  value["genres"]        = m_genres;
  value["developer"]     = m_strDeveloper;
  value["overview"]      = m_strOverview;
  value["year"]          = m_year;
  value["id"]            = m_strID;
  value["region"]        = m_strRegion;
  value["publisher"]     = m_strPublisher;
  value["format"]        = m_strFormat;
  value["cartridgetype"] = m_strCartridgeType;
  value["gameclient"]    = m_strGameClient;
}

void CGameInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  // No database entries for games (...yet)
}
