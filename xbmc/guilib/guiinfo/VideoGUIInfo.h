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

class CVideoInfoTag;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CVideoGUIInfo : public CGUIInfoProvider
{
public:
  CVideoGUIInfo() = default;
  ~CVideoGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const override;
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;

private:
  int GetPercentPlayed(const CVideoInfoTag* tag) const;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
