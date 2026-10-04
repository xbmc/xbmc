/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "music/windows/GUIWindowMusicNav.h"

#include <string>

#include <gtest/gtest.h>

using XFILE::CMusicDatabaseDirectory;
using XFILE::MUSICDATABASEDIRECTORY::NodeType;

namespace
{
class CTestGUIWindowMusicNav : public CGUIWindowMusicNav
{
public:
  using CGUIWindowMusicNav::GetStartFolder;
};
} // namespace

TEST(TestGUIWindowMusicNav, EveryNamedStartFolderIsALibraryNode)
{
  CTestGUIWindowMusicNav window;
  for (const char* name : {"albums", "artists", "boxsets", "compilations", "genres",
                           "recentlyaddedalbums", "recentlyplayedalbums", "singles", "songs",
                           "top100", "top100albums", "top100songs", "years"})
  {
    const std::string folder{window.GetStartFolder(name)};
    EXPECT_NE(NodeType::NONE, CMusicDatabaseDirectory::GetDirectoryChildType(folder))
        << name << " -> " << folder;
  }
}
