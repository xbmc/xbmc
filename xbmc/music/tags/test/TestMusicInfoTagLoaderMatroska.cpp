/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/tags/MusicInfoTagLoaderMatroska.h"

#include <gtest/gtest.h>
#include <taglib/taglib.h>

#if (TAGLIB_MAJOR_VERSION > 2) ||                                                                  \
    (TAGLIB_MAJOR_VERSION == 2 &&                                                                  \
     (TAGLIB_MINOR_VERSION > 3 || (TAGLIB_MINOR_VERSION == 3 && TAGLIB_PATCH_VERSION >= 1)))

using namespace MUSIC_INFO;

namespace
{
CMusicInfoTagLoaderMatroska::ChapterOrder MakeChapterOrder(
    std::initializer_list<unsigned long long> uids)
{
  CMusicInfoTagLoaderMatroska::ChapterOrder order;
  for (unsigned long long uid : uids)
    order.emplace_back(uid, "", 0.0, 1.0, 0ULL);
  return order;
}
} // namespace

TEST(TestMusicInfoTagLoaderMatroska, HasMatroskaDirectoryContent)
{
  std::map<std::string, std::string> emptyTags;
  std::map<std::string, std::string> fileTags{{"ALBUM", "Demo"}};
  const auto emptyOrder = MakeChapterOrder({});
  const auto chapterOrder = MakeChapterOrder({1ULL, 2ULL});

  EXPECT_FALSE(CMusicInfoTagLoaderMatroska::HasMatroskaDirectoryContent(emptyTags, emptyOrder));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::HasMatroskaDirectoryContent(fileTags, emptyOrder));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::HasMatroskaDirectoryContent(emptyTags, chapterOrder));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::HasMatroskaDirectoryContent(fileTags, chapterOrder));
}

TEST(TestMusicInfoTagLoaderMatroska, IsRealChapterUid)
{
  EXPECT_FALSE(
      CMusicInfoTagLoaderMatroska::IsRealChapterUid(CMusicInfoTagLoaderMatroska::DummyChapterUid));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::IsRealChapterUid(0ULL));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::IsRealChapterUid(1ULL));
  EXPECT_TRUE(CMusicInfoTagLoaderMatroska::IsRealChapterUid(42ULL));
}

TEST(TestMusicInfoTagLoaderMatroska, CountRealChapters)
{
  EXPECT_EQ(0u, CMusicInfoTagLoaderMatroska::CountRealChapters(MakeChapterOrder({})));
  EXPECT_EQ(0u, CMusicInfoTagLoaderMatroska::CountRealChapters(
                    MakeChapterOrder({CMusicInfoTagLoaderMatroska::DummyChapterUid})));
  EXPECT_EQ(2u, CMusicInfoTagLoaderMatroska::CountRealChapters(MakeChapterOrder({1ULL, 2ULL})));
  EXPECT_EQ(2u, CMusicInfoTagLoaderMatroska::CountRealChapters(
                    MakeChapterOrder({1ULL, CMusicInfoTagLoaderMatroska::DummyChapterUid, 2ULL})));
}

TEST(TestMusicInfoTagLoaderMatroska, ResolveTargetType30Route)
{
  EXPECT_EQ(MatroskaTrackTagRoute::BindFirstChapter,
            CMusicInfoTagLoaderMatroska::ResolveTargetType30Route(1, 0ULL));
  EXPECT_EQ(MatroskaTrackTagRoute::BindFirstChapter,
            CMusicInfoTagLoaderMatroska::ResolveTargetType30Route(1, 99ULL));

  EXPECT_EQ(MatroskaTrackTagRoute::BindChapterUid,
            CMusicInfoTagLoaderMatroska::ResolveTargetType30Route(2, 1ULL));
  EXPECT_EQ(MatroskaTrackTagRoute::BindChapterUid,
            CMusicInfoTagLoaderMatroska::ResolveTargetType30Route(3, 123456ULL));

  EXPECT_EQ(MatroskaTrackTagRoute::BindFileTags,
            CMusicInfoTagLoaderMatroska::ResolveTargetType30Route(2, 0ULL));
}

#endif // TagLib >= 2.3.1
