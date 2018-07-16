/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

#include <string>
#include <utility>
#include <vector>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CLibraryGUIInfo : public CGUIInfoProvider
{
public:
  CLibraryGUIInfo();
  ~CLibraryGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const override;
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;

  bool GetLibraryBool(int condition) const;
  void SetLibraryBool(int condition, bool value);
  void ResetLibraryBools();

private:
  mutable int m_libraryHasMusic;
  mutable int m_libraryHasMovies;
  mutable int m_libraryHasTVShows;
  mutable int m_libraryHasMusicVideos;
  mutable int m_libraryHasMovieSets;
  mutable int m_libraryHasSingles;
  mutable int m_libraryHasCompilations;

  //Count of artists in music library contributing to song by role e.g. composers, conductors etc.
  //For checking visibility of custom nodes for a role.
  mutable std::vector<std::pair<std::string, int>> m_libraryRoleCounts;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
