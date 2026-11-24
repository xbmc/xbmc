/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/LibraryGUIInfo.h"

#include <gtest/gtest.h>

using namespace KODI::GUILIB::GUIINFO;

// What a scan or a clean has found out is answered without asking the database
TEST(TestLibraryGUIInfo, AKnownAnswerIsGivenWithoutAskingTheDatabase)
{
  CLibraryGUIInfo info;
  info.SetLibraryBool(LIBRARY_HAS_MOVIES, true);
  info.SetLibraryBool(LIBRARY_HAS_BOXSETS, false);

  EXPECT_TRUE(info.GetLibraryBool(LIBRARY_HAS_MOVIES));
  EXPECT_FALSE(info.GetLibraryBool(LIBRARY_HAS_BOXSETS));
  EXPECT_TRUE(info.GetLibraryBool(LIBRARY_HAS_VIDEO));
}
