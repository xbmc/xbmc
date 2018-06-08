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

#pragma once

// LibExportSettings.h: interface for the CLibExportSettings class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include "settings/lib/Setting.h"

// Enumeration of library export options (possibly OR'd together)
enum ELIBEXPORTOPTIONS
{
  ELIBEXPORT_SINGLEFILE = 0x0000,
  ELIBEXPORT_SEPARATEFILES = 0x0001,
  ELIBEXPORT_TOLIBRARYFOLDER = 0x0002,
  ELIBEXPORT_OVERWRITE = 0x0004,
  ELIBEXPORT_UNSCRAPED = 0x0008,
  ELIBEXPORT_ALBUMS = 0x0010,
  ELIBEXPORT_ALBUMARTISTS = 0x0020,
  ELIBEXPORT_SONGARTISTS = 0x0040,
  ELIBEXPORT_OTHERARTISTS = 0x0080,
  ELIBEXPORT_ARTWORK = 0x0100,
  ELIBEXPORT_NFOFILES = 0x0200,
  ELIBEXPORT_ACTORTHUMBS = 0x0400
};

class CLibExportSettings
{
public:
  CLibExportSettings();
  ~CLibExportSettings() = default;

  bool operator!=(const CLibExportSettings &right) const;
  bool IsItemExported(ELIBEXPORTOPTIONS item) const;
  std::vector<int> GetExportItems() const;
  void ClearItems() { m_itemstoexport = 0; }
  void AddItem(ELIBEXPORTOPTIONS item) { m_itemstoexport += item; }
  unsigned int GetItemsToExport() { return m_itemstoexport; }
  void SetItemsToExport(int itemstoexport) { m_itemstoexport = static_cast<unsigned int>(itemstoexport); }
  unsigned int GetExportType() { return m_exporttype; }
  void SetExportType(int exporttype) { m_exporttype = static_cast<unsigned int>(exporttype); }
  bool IsSingleFile() const;
  bool IsSeparateFiles() const;
  bool IsToLibFolders() const;

  std::string m_strPath;
  bool m_overwrite;
  bool m_artwork;
  bool m_unscraped;
  bool m_skipnfo;
private:
  unsigned int m_exporttype; //singlefile, separate files, to library folder
  unsigned int m_itemstoexport;
};
