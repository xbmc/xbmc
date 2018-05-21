#pragma once
/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
