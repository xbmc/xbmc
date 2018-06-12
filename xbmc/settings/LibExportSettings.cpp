/*
 *      Copyright (C) 2017 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
// LibExportSettings.cpp: implementation of the CLibExportSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "LibExportSettings.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLibExportSettings::CLibExportSettings()
{
  m_exporttype = ELIBEXPORT_SINGLEFILE;
  m_itemstoexport = ELIBEXPORT_ALBUMS + ELIBEXPORT_ALBUMARTISTS;
  m_overwrite = false;
  m_artwork = false;
  m_unscraped  = false;
  m_skipnfo = false;
}

bool CLibExportSettings::operator!=(const CLibExportSettings &right) const
{
  if (m_exporttype != right.m_exporttype)
    return true;
  if (m_strPath != right.m_strPath)
    return true;
  if (m_overwrite != right.m_overwrite)
    return true;
  if (m_itemstoexport != right.m_itemstoexport)
    return true;

  if (m_artwork != right.m_artwork)
    return true;
  if (m_unscraped != right.m_unscraped)
    return true;
  if (m_skipnfo != right.m_skipnfo)
    return true;

  return false;
}

bool CLibExportSettings::IsItemExported(ELIBEXPORTOPTIONS item) const
{
  return (m_itemstoexport & item);
}

std::vector<int> CLibExportSettings::GetExportItems() const
{
  std::vector<int> values;
  if (IsItemExported(ELIBEXPORT_ALBUMS))
    values.emplace_back(ELIBEXPORT_ALBUMS);
  if (IsItemExported(ELIBEXPORT_ALBUMARTISTS))
    values.emplace_back(ELIBEXPORT_ALBUMARTISTS);
  if (IsItemExported(ELIBEXPORT_SONGARTISTS))
    values.emplace_back(ELIBEXPORT_SONGARTISTS);
  if (IsItemExported(ELIBEXPORT_OTHERARTISTS))
    values.emplace_back(ELIBEXPORT_OTHERARTISTS);
  if (IsItemExported(ELIBEXPORT_ACTORTHUMBS))
    values.emplace_back(ELIBEXPORT_ACTORTHUMBS);

  return values;
}

bool CLibExportSettings::IsSingleFile() const
{
  return (m_exporttype == ELIBEXPORT_SINGLEFILE);
}

bool CLibExportSettings::IsSeparateFiles() const
{
  return (m_exporttype == ELIBEXPORT_SEPARATEFILES);
}

bool CLibExportSettings::IsToLibFolders() const
{
  return (m_exporttype == ELIBEXPORT_TOLIBRARYFOLDER);
}
