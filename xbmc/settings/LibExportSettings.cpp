/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

bool CLibExportSettings::IsArtists() const
{
  return (m_itemstoexport & ELIBEXPORT_ALBUMARTISTS) ||
         (m_itemstoexport & ELIBEXPORT_SONGARTISTS) ||
         (m_itemstoexport & ELIBEXPORT_OTHERARTISTS);
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
  if (IsItemExported(ELIBEXPORT_SONGS))
    values.emplace_back(ELIBEXPORT_SONGS);
  return values;
}

std::vector<int> CLibExportSettings::GetLimitedItems(int items) const
{
  std::vector<int> values;
  if (IsItemExported(ELIBEXPORT_ALBUMS) && (items & ELIBEXPORT_ALBUMS))
    values.emplace_back(ELIBEXPORT_ALBUMS);
  if (IsItemExported(ELIBEXPORT_ALBUMARTISTS) && (items & ELIBEXPORT_ALBUMARTISTS))
    values.emplace_back(ELIBEXPORT_ALBUMARTISTS);
  if (IsItemExported(ELIBEXPORT_SONGARTISTS) && (items & ELIBEXPORT_SONGARTISTS))
    values.emplace_back(ELIBEXPORT_SONGARTISTS);
  if (IsItemExported(ELIBEXPORT_OTHERARTISTS) && (items & ELIBEXPORT_OTHERARTISTS))
    values.emplace_back(ELIBEXPORT_OTHERARTISTS);
  if (IsItemExported(ELIBEXPORT_ACTORTHUMBS) && (items & ELIBEXPORT_ACTORTHUMBS))
    values.emplace_back(ELIBEXPORT_ACTORTHUMBS);
  if (IsItemExported(ELIBEXPORT_SONGS) && (items & ELIBEXPORT_SONGS))
    values.emplace_back(ELIBEXPORT_SONGS);
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

bool CLibExportSettings::IsArtistFoldersOnly() const
{
  return (m_exporttype == ELIBEXPORT_ARTISTFOLDERS);
}
