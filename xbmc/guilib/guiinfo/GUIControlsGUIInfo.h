/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/WindowIDs.h"
#include "guilib/guiinfo/GUIInfoProvider.h"

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

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; }
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; }

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
