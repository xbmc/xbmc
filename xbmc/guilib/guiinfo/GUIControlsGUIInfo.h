/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
#include "guilib/WindowIDs.h"

#include <map>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CGUIControlsGUIInfo : public CGUIInfoProvider
{
public:
  ~CGUIControlsGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const override;
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  /*! \brief containers call this to specify that the focus is changing
   \param id control id
   \param next true if we're moving to the next item, false if previous
   \param scrolling true if the container is scrolling, false if the movement requires no scroll
   */
  void SetContainerMoving(int id, bool next, bool scrolling);
  void ResetContainerMovingCache();

private:
  int m_nextWindowID = WINDOW_INVALID;
  int m_prevWindowID = WINDOW_INVALID;

  std::map<int, int> m_containerMoves;  // direction of list moving
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
