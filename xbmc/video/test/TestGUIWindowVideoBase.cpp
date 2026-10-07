/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/WindowIDs.h"
#include "video/windows/GUIWindowVideoBase.h"

#include <gtest/gtest.h>

namespace
{
// The video library windows' own view states need a GUI, so this takes the general one
class CTestGUIWindowVideoBase : public CGUIWindowVideoBase
{
public:
  CTestGUIWindowVideoBase() : CGUIWindowVideoBase(WINDOW_INVALID, "") {}

  using CGUIWindowVideoBase::CanContainFilter;
};
} // namespace

TEST(TestGUIWindowVideoBase, AVideoLibraryPathCanCarryAFilter)
{
  CTestGUIWindowVideoBase window;
  EXPECT_TRUE(window.CanContainFilter("videodb://movies/titles/"));
  EXPECT_TRUE(window.CanContainFilter("videodb://tvshows/genres/1/"));
  EXPECT_FALSE(window.CanContainFilter("musicdb://albums/"));
  EXPECT_FALSE(window.CanContainFilter("smb://server/films/"));
}
