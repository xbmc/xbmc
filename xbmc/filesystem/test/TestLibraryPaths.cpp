/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/LibraryPaths.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
//! Whether \p path names a node shipped in system/library, as a folder or an XML file
bool IsShippedNode(const std::string& path)
{
  if (!StringUtils::StartsWith(path, LIBRARY::ROOT))
    return false;
  std::string node{"special://xbmc/system/library/" +
                   path.substr(std::string{LIBRARY::ROOT}.size())};
  if (XFILE::CDirectory::Exists(node))
    return true;
  URIUtils::RemoveSlashAtEnd(node);
  return XFILE::CFile::Exists(node);
}
} // namespace

TEST(TestLibraryPaths, EveryPathIsAShippedNode)
{
  for (const char* path :
       {LIBRARY::VIDEO, LIBRARY::VIDEO_FLAT, LIBRARY::VIDEO_FILES, LIBRARY::MOVIE_TITLES,
        LIBRARY::TVSHOW_TITLES, LIBRARY::MUSICVIDEOS, LIBRARY::MUSICVIDEO_TITLES, LIBRARY::MUSIC,
        LIBRARY::MUSIC_FILES, LIBRARY::MUSIC_PLAYLISTS})
    EXPECT_TRUE(IsShippedNode(path)) << path;
}
