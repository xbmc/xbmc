/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AutoSwitch.h"
#include "FileItemList.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <array>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

namespace
{

struct FileItemSpec
{
  std::string path;
  bool isFolder;
  std::string thumb;
  bool hasVideo = false;
  bool hasMusic = false;
  bool hasPicture = false;
  bool hasAddonId = false;
};

void listFromDefs(CFileItemList& items, const std::vector<FileItemSpec>& defs)
{
  for (const auto& def : defs)
  {
    auto item = std::make_shared<CFileItem>(def.path, def.isFolder);
    if (!def.thumb.empty())
      item->SetArt("thumb", def.thumb);
    if (def.hasVideo)
      item->GetVideoInfoTag()->m_iEpisode = 1;
    if (def.hasMusic)
      item->GetMusicInfoTag()->SetAlbum("foo");
    if (def.hasPicture)
      item->GetPictureInfoTag()->SetInfo("foo", "bar");
    if (def.hasAddonId)
      item->SetProperty("Addon.ID", "foobar");
    items.Add(std::move(item));
  }
}

struct FileItemTestDef
{
  std::vector<FileItemSpec> items;
  bool result;
};

const auto byfolder_tests = std::array{
    FileItemTestDef{{{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, "thumb2.png"}},
                    true},
    FileItemTestDef{{{"/home/user/", false, "thumb.png"}, {"/home/user/foo/", true, "thumb2.png"}},
                    false},
    FileItemTestDef{{{"/home/user/", true, ""}, {"/home/user/foo/", true, "thumb2.png"}}, true},
    FileItemTestDef{{{"/home/user/", true, ""}, {"/home/user/foo/", true, ""}}, false},
};

class ByFoldersTest : public testing::WithParamInterface<FileItemTestDef>, public testing::Test
{
};

const auto byfiles_tests = std::array{
    FileItemTestDef{
        {{"/home/user/", true, "thumb.png"}, {"/home/user/foo.mkv", false, "thumb2.png"}}, true},
    FileItemTestDef{{{"/home/user/", true, ""}, {"/home/user/foo.mkv", false, ""}}, false},
    FileItemTestDef{
        {{"/home/user/foo.avi", false, "thumb.png"}, {"/home/user/foo.mkv", false, "thumb2.png"}},
        false},
    FileItemTestDef{{{"/home/user/foo.avi", false, ""}, {"/home/user/foo.mkv", false, ""}}, false},
};

class ByFilesTest : public testing::WithParamInterface<FileItemTestDef>, public testing::Test
{
};

struct PercentTestDef
{
  std::vector<FileItemSpec> items;
  int percentage;
  bool result;
};

const auto bythumbpercent_tests = std::array{
    PercentTestDef{
        {{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, "thumb2.png"}}, 50, true},
    PercentTestDef{{{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, ""}}, 70, false},
    PercentTestDef{{{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, ""}}, 30, true},
    PercentTestDef{{{"/home/user/", true, ""}, {"/home/user/foo/", true, ""}}, 0, false},
};

class ByThumbPercentTest : public testing::WithParamInterface<PercentTestDef>, public testing::Test
{
};

const auto byfilecount_tests = std::array{
    FileItemTestDef{{{"/home/user/foo.png", false, ""}, {"/home/user/foo.mkv", false, ""}}, true},
    FileItemTestDef{{{"/home/user/foo.png", false, ""}, {"/home/user/foo/", true, ""}}, true},
    FileItemTestDef{{{"/home/user/foo.png", false, ""},
                     {"/home/user/foo/", true, ""},
                     {"/home/user/bar/", true, ""},
                     {"/home/user/foobar/", true, ""}},
                    false},
};

class ByFileCountTest : public testing::WithParamInterface<FileItemTestDef>, public testing::Test
{
};

const auto byfolderthumbpercentage_tests = std::array{
    PercentTestDef{{{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, ""}}, 50, true},
    PercentTestDef{{{"/home/user/", true, "thumb.png"}, {"/home/user/foo/", true, ""}}, 60, false},
    PercentTestDef{
        {{"/home/user/foo.png", false, "thumb.png"}, {"/home/user/foo/", true, ""}}, 30, false},
    PercentTestDef{{{"/home/user/", true, "thumb.png"},
                    {"/home/user/foo/", true, ""},
                    {"/home/user/bar.png", false, "thumb.png"}},
                   60,
                   false},
    PercentTestDef{{{"/home/user/foo.png", false, "thumb.png"},
                    {"/home/user/foo/", true, ""},
                    {"/home/user/bar/", true, "thumb.png"},
                    {"/home/user/foo.avi", false, ""}},
                   50,
                   false},
    PercentTestDef{{{"/home/user/foo.png", false, "thumb.png"},
                    {"/home/user/foo/", true, ""},
                    {"/home/user/bar/", true, "thumb.png"},
                    {"/home/user/foobar/", true, ""}},
                   50,
                   false},
};

class ByFolderThumbPercentageTest : public testing::WithParamInterface<PercentTestDef>,
                                    public testing::Test
{
};

struct MetadataPercentageDef
{
  std::vector<FileItemSpec> items;
  float result;
};

const auto metadatapercentage_tests = std::array{
    MetadataPercentageDef{{{"/home/user/foo.png", false, ""}, {"/home/user/foo.mkv", false, ""}},
                          0.0},
    MetadataPercentageDef{
        {{"/home/user/foo.png", false, "", true}, {"/home/user/foo.mkv", false, ""}}, 0.5},
    MetadataPercentageDef{
        {{"/home/user/foo.png", false, "", false, true}, {"/home/user/foo.mkv", false, ""}}, 0.5},
    MetadataPercentageDef{
        {{"/home/user/foo.png", false, "", false, false, true}, {"/home/user/foo.mkv", false, ""}},
        0.5},
    MetadataPercentageDef{{{"/home/user/foo.png", false, "", false, false, false, true},
                           {"/home/user/foo.mkv", false, ""}},
                          0.5},
};

class MetadataPercentageTest : public testing::WithParamInterface<MetadataPercentageDef>,
                               public testing::Test
{
};

} // namespace

TEST_P(ByFoldersTest, ByFolders)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_EQ(CAutoSwitch::ByFolders(items), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch, ByFoldersTest, testing::ValuesIn(byfolder_tests));

TEST_P(ByFilesTest, ByFiles)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_EQ(CAutoSwitch::ByFiles(true, items), GetParam().result);

  // Add '..' item and make sure we get the same result
  auto item = std::make_shared<CFileItem>("/home/user", true);
  item->SetLabel("..");
  items.Add(std::move(item));
  EXPECT_EQ(CAutoSwitch::ByFiles(false, items), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch, ByFilesTest, testing::ValuesIn(byfiles_tests));

TEST_P(ByThumbPercentTest, ByThumbPercent)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_EQ(CAutoSwitch::ByThumbPercent(true, GetParam().percentage, items), GetParam().result);

  // Add '..' item and make sure we get the same result
  auto item = std::make_shared<CFileItem>("/home/user", true);
  item->SetLabel("..");
  items.Add(std::move(item));
  EXPECT_EQ(CAutoSwitch::ByThumbPercent(false, GetParam().percentage, items), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch,
                         ByThumbPercentTest,
                         testing::ValuesIn(bythumbpercent_tests));

TEST_P(ByFileCountTest, ByFileCount)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_EQ(CAutoSwitch::ByFileCount(items), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch, ByFileCountTest, testing::ValuesIn(byfilecount_tests));

TEST_P(ByFolderThumbPercentageTest, ByFolderThumbPercentage)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_EQ(CAutoSwitch::ByFolderThumbPercentage(true, GetParam().percentage, items),
            GetParam().result);

  // Add '..' item and make sure we get the same result
  auto item = std::make_shared<CFileItem>("/home/user", true);
  item->SetLabel("..");
  items.Add(std::move(item));
  EXPECT_EQ(CAutoSwitch::ByFolderThumbPercentage(false, GetParam().percentage, items),
            GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch,
                         ByFolderThumbPercentageTest,
                         testing::ValuesIn(byfolderthumbpercentage_tests));

TEST_P(MetadataPercentageTest, MetadataPercentage)
{
  CFileItemList items;
  listFromDefs(items, GetParam().items);

  EXPECT_FLOAT_EQ(CAutoSwitch::MetadataPercentage(items), GetParam().result);

  // Add '..' item
  auto item = std::make_shared<CFileItem>("/home/user", true);
  item->SetLabel("..");
  items.Add(std::move(item));
  EXPECT_FLOAT_EQ(CAutoSwitch::MetadataPercentage(items), GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestAutoSwitch,
                         MetadataPercentageTest,
                         testing::ValuesIn(metadatapercentage_tests));
