/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/LibraryPaths.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryType.h"
#include "media/MediaSection.h"
#include "music/MusicLibraryQuery.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "video/VideoLibraryQuery.h"

#include <optional>
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

TEST(TestLibraryPaths, EveryShippedFilterNodeIsAQueryItsLibraryCanList)
{
  const std::string root{"special://xbmc/system/library/"};
  CFileItemList files;
  CUtil::GetRecursiveListing(root, files, ".xml", XFILE::DIR_FLAG_NO_FILE_DIRS);

  int filters{0};
  for (const auto& file : files)
  {
    const std::string& path{file->GetPath()};
    CXBMCTinyXML2 doc;
    ASSERT_TRUE(doc.LoadFile(path)) << path;
    const tinyxml2::XMLElement* node{doc.RootElement()};
    ASSERT_NE(nullptr, node) << path;
    if (XMLUtils::GetAttribute(node, "type") != "filter")
    {
      continue;
    }
    ++filters;

    std::string content;
    XMLUtils::GetString(node, "content", content);
    const std::optional<MEDIA::LIBRARY_QUERY::TYPE> type{
        MEDIA::LIBRARY_QUERY::TypeFromName(content)};
    ASSERT_TRUE(type) << path << " lists \"" << content << "\"";

    MEDIA::CLibraryQuery query;
    query.SetType(*type);
    ASSERT_TRUE(query.LoadFromXML(node)) << path;

    for (const auto& rule : query.GetRules())
    {
      EXPECT_TRUE(MEDIA::CLibraryQueryRule::HasField(*type, rule->m_field))
          << path << " has a rule on " << MEDIA::CLibraryQueryRule::GetFieldName(rule->m_field);
    }
    for (const MEDIA::LIBRARY_QUERY::Source& source : MEDIA::LIBRARY_QUERY::SourcesOf(*type))
    {
      EXPECT_TRUE(source.section == MEDIA::MediaSection::MUSIC
                      ? MUSIC::LIBRARY_QUERY::ListingPath(source.itemType, query.GetGroup())
                      : VIDEO::LIBRARY_QUERY::ListingPath(source.itemType, query.GetGroup()))
          << path << " groups by what its library has no list of";
    }
  }
  EXPECT_GE(filters, 40) << "the walk found fewer filter nodes than are shipped";
}
