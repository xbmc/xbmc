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
  if (!StringUtils::StartsWith(path, MEDIA::LIBRARY_PATH::ROOT))
    return false;
  std::string node{"special://xbmc/system/library/" +
                   path.substr(std::string{MEDIA::LIBRARY_PATH::ROOT}.size())};
  if (XFILE::CDirectory::Exists(node))
    return true;
  URIUtils::RemoveSlashAtEnd(node);
  return XFILE::CFile::Exists(node);
}
} // namespace

TEST(TestLibraryPaths, EveryPathIsAShippedNode)
{
  for (const char* path : {MEDIA::LIBRARY_PATH::VIDEO, MEDIA::LIBRARY_PATH::VIDEO_FLAT,
                           MEDIA::LIBRARY_PATH::VIDEO_FILES, MEDIA::LIBRARY_PATH::MOVIE_TITLES,
                           MEDIA::LIBRARY_PATH::TVSHOW_TITLES, MEDIA::LIBRARY_PATH::MUSICVIDEOS,
                           MEDIA::LIBRARY_PATH::MUSICVIDEO_TITLES, MEDIA::LIBRARY_PATH::MUSIC,
                           MEDIA::LIBRARY_PATH::MUSIC_FILES, MEDIA::LIBRARY_PATH::MUSIC_PLAYLISTS})
    EXPECT_TRUE(IsShippedNode(path)) << path;
}
