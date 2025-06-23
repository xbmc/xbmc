/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// LibExportSettings.h: interface for the CLibExportSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "settings/lib/Setting.h"

#include <string>
#include <string_view>

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
  ELIBEXPORT_ACTORTHUMBS = 0x0400,
  ELIBEXPORT_ARTISTFOLDERS = 0x0800,
  ELIBEXPORT_SONGS = 0x1000
};

class CLibExportSettings
{
public:
  CLibExportSettings() = default;
  ~CLibExportSettings() = default;

  bool operator!=(const CLibExportSettings &right) const;

  const std::string& GetPath() const { return m_strPath; }
  void SetPath(std::string_view path) { m_strPath = path; }
  bool IsOverwrite() const { return m_overwrite; }
  void SetOverwrite(bool set) { m_overwrite = set; }
  bool IsArtwork() const { return m_artwork; }
  void SetArtwork(bool set) { m_artwork = set; }
  bool IsUnscraped() const { return m_unscraped; }
  void SetUnscraped(bool set) { m_unscraped = set; }
  bool IsSkipNfo() const { return m_skipnfo; }
  void SetSkipNfo(bool set) { m_skipnfo = set; }
  bool IsItemExported(ELIBEXPORTOPTIONS item) const;
  bool IsArtists() const;
  std::vector<int> GetExportItems() const;
  std::vector<int> GetLimitedItems(int items) const;
  void ClearItems() { m_itemstoexport = 0; }
  void AddItem(ELIBEXPORTOPTIONS item) { m_itemstoexport += item; }
  unsigned int GetItemsToExport() const { return m_itemstoexport; }
  void SetItemsToExport(int itemstoexport) { m_itemstoexport = static_cast<unsigned int>(itemstoexport); }
  unsigned int GetExportType() const { return m_exporttype; }
  void SetExportType(int exporttype) { m_exporttype = static_cast<unsigned int>(exporttype); }
  bool IsSingleFile() const;
  bool IsSeparateFiles() const;
  bool IsToLibFolders() const;
  bool IsArtistFoldersOnly() const;

private:
  std::string m_strPath;
  bool m_overwrite{false};
  bool m_artwork{false};
  bool m_unscraped{false};
  bool m_skipnfo{false};
  unsigned int m_exporttype{ELIBEXPORT_SINGLEFILE}; //singlefile, separate files, to library folder
  unsigned int m_itemstoexport{ELIBEXPORT_ALBUMS + ELIBEXPORT_ALBUMARTISTS};
};
