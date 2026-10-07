/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SpecialProtocol.h"
#include "playlists/PlayListPLS.h"
#include "utils/URIUtils.h"

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListPLS, AnUnnamedPlayListIsNamedAfterItsFile)
{
  const std::string path{
      URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "unnamed.pls")};
  {
    std::ofstream file(path);
    file << "[playlist]\nFile1=http://example.com/stream\nNumberOfEntries=1\n";
  }

  PLAYLIST::CPlayListPLS playlist;
  EXPECT_TRUE(playlist.Load(path));
  std::remove(path.c_str());

  EXPECT_EQ(playlist.size(), 1);
  EXPECT_EQ(playlist.GetName(), "unnamed.pls");
}
