/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GameInfoTag.h"
#include "utils/Variant.h"

using namespace GAME_INFO;

void CGameInfoTag::Reset()
{
  m_bLoaded = false;
  m_strURL.clear();
  m_strName.clear();
  m_strPlatform.clear();
  m_strID.clear();
  m_strRegion.clear();
  m_strPublisher.clear();
  m_strFormat.clear();
  m_strCartridgeType.clear();
}

const CGameInfoTag &CGameInfoTag::operator=(const CGameInfoTag &tag)
{
  if (this != &tag)
  {
    m_bLoaded          = tag.m_bLoaded;
    m_strURL           = tag.m_strURL;
    m_strName          = tag.m_strName;
    m_strPlatform      = tag.m_strPlatform;
    m_strID            = tag.m_strID;
    m_strRegion        = tag.m_strRegion;
    m_strPublisher     = tag.m_strPublisher;
    m_strFormat        = tag.m_strFormat;
    m_strCartridgeType = tag.m_strCartridgeType;
  }
  return *this;
}

bool CGameInfoTag::operator==(const CGameInfoTag &tag) const
{
  if (this != &tag)
  {
    // Two tags can't be equal if they aren't loaded
    if (!m_bLoaded || !tag.m_bLoaded) return false;
    if (m_strURL != tag.m_strURL) return false;
    if (m_strName != tag.m_strName) return false;
    if (m_strPlatform != tag.m_strPlatform) return false;
    if (m_strID != tag.m_strID) return false;
    if (m_strRegion != tag.m_strRegion) return false;
    if (m_strPublisher != tag.m_strPublisher) return false;
    if (m_strFormat != tag.m_strFormat) return false;
    if (m_strCartridgeType != tag.m_strCartridgeType) return false;
  }
  return true;
}

void CGameInfoTag::Archive(CArchive &ar)
{
  if (ar.IsStoring())
  {
    ar << m_bLoaded;
    ar << m_strURL;
    ar << m_strName;
    ar << m_strPlatform;
    ar << m_strID;
    ar << m_strRegion;
    ar << m_strPublisher;
    ar << m_strFormat;
    ar << m_strCartridgeType;
  }
  else
  {
    ar >> m_bLoaded;
    ar >> m_strURL;
    ar >> m_strName;
    ar >> m_strPlatform;
    ar >> m_strID;
    ar >> m_strRegion;
    ar >> m_strPublisher;
    ar >> m_strFormat;
    ar >> m_strCartridgeType;
  }
}

void CGameInfoTag::Serialize(CVariant &value) const
{
  value["loaded"]        = m_bLoaded;
  value["url"]           = m_strURL;
  value["name"]          = m_strName;
  value["platform"]      = m_strPlatform;
  value["id"]            = m_strID;
  value["region"]        = m_strRegion;
  value["publisher"]     = m_strPublisher;
  value["format"]        = m_strFormat;
  value["cartridgetype"] = m_strCartridgeType;
}

void CGameInfoTag::ToSortable(SortItem &sortable)
{
  // No database entries for games (...yet)
}
