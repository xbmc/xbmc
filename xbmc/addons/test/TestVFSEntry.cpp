/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "addons/VFSEntry.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
AddonInfoPtr MakeVFSInfo()
{
  CAddonInfoBuilderFromDB builder;
  builder.SetId("vfs.test");
  builder.SetExtensions(CAddonType(AddonType::VFS));
  return builder.get();
}

class TestVFS : public CVFSEntry
{
public:
  TestVFS() : CVFSEntry(MakeVFSInfo())
  {
    // Supply the binary add-on callbacks without loading a library.
    m_ifc.vfs->toAddon->addonInstance = this;
    m_ifc.vfs->toAddon->get_directory =
        [](const AddonInstance_VFSEntry* instance, const VFSURL*, VFSDirEntry** entries, int* count,
           VFSGetDirectoryCallbacks*) { return Self(instance).Read(entries, count); };
    m_ifc.vfs->toAddon->contains_files = [](const AddonInstance_VFSEntry* instance, const VFSURL*,
                                            VFSDirEntry** entries, int* count, char* rootpath)
    {
      std::strcpy(rootpath, "test://archive/");
      return Self(instance).Read(entries, count);
    };
    m_ifc.vfs->toAddon->free_directory =
        [](const AddonInstance_VFSEntry* instance, VFSDirEntry* entries, int count)
    {
      auto& self = Self(instance);
      ++self.freeCalls;
      self.freedEntries = entries;
      self.freedCount = count;
    };
  }

  VFSDirEntry* entries{nullptr};
  int count{0};
  bool success{true};
  int freeCalls{0};
  VFSDirEntry* freedEntries{nullptr};
  int freedCount{0};

private:
  static TestVFS& Self(const AddonInstance_VFSEntry* instance)
  {
    return *static_cast<TestVFS*>(instance->toAddon->addonInstance);
  }

  bool Read(VFSDirEntry** output, int* outputCount)
  {
    *output = entries;
    *outputCount = count;
    return success;
  }
};

class TestVFSEntry : public testing::TestWithParam<bool>
{
protected:
  void SetUp() override
  {
    entry.label = label;
    entry.path = path;
    entry.size = std::numeric_limits<uint64_t>::max();
    vfs.entries = &entry;
    vfs.count = 1;
  }

  bool Read(CFileItemList& items)
  {
    const CURL url("test://archive/");
    return GetParam() ? vfs.ContainsFiles(url, items) : vfs.GetDirectory(url, items, nullptr);
  }

  TestVFS vfs;
  VFSDirEntry entry{};
  char label[5]{"Page"};
  char path[24]{"test://archive/page.jpg"};
};

TEST_P(TestVFSEntry, AcceptsOnlySizesRepresentableByCFileItem)
{
  constexpr uint64_t maximum = std::numeric_limits<int64_t>::max();
  for (const auto [input, expected] : {std::pair<uint64_t, int64_t>{0, 0},
                                       {12345, 12345},
                                       {maximum, INT64_MAX},
                                       {maximum + 1, 0},
                                       {UINT64_MAX, 0}})
  {
    SCOPED_TRACE(input);
    entry.size = input;
    CFileItemList items;
    ASSERT_TRUE(Read(items));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_EQ(items[0]->GetSize(), expected);
  }
}

TEST_P(TestVFSEntry, PreservesRepresentableDates)
{
  for (const auto [input, expected] : {std::pair<int64_t, const char*>{-1, "1969-12-31 23:59:59"},
                                       {-2208988800, "1900-01-01 00:00:00"},
                                       {1, "1970-01-01 00:00:01"},
                                       {1000000000, "2001-09-09 01:46:40"},
                                       {-11644473600, "1601-01-01 00:00:00"},
                                       {-2147483648, "1901-12-13 20:45:52"},
                                       {2147483647, "2038-01-19 03:14:07"},
                                       {910670515199, "30827-12-31 23:59:59"}})
  {
    if (!std::in_range<time_t>(input))
      continue;
    SCOPED_TRACE(input);
    entry.date_time = static_cast<time_t>(input);
    CFileItemList items;
    ASSERT_TRUE(Read(items));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_TRUE(items[0]->GetDateTime().IsValid());
    EXPECT_EQ(items[0]->GetDateTime().GetAsDBDateTime(), expected);
  }
}

TEST_P(TestVFSEntry, LeavesMissingAndUnrepresentableDatesUnset)
{
  constexpr int64_t invalidDates[]{
      0, -11644473601LL, 910670515200LL, 910692730086LL, 922337203686LL, INT64_MIN, INT64_MAX};
  for (int64_t input : invalidDates)
  {
    if (!std::in_range<time_t>(input))
      continue;
    SCOPED_TRACE(input);
    entry.date_time = static_cast<time_t>(input);
    CFileItemList items;
    ASSERT_TRUE(Read(items));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_FALSE(items[0]->GetDateTime().IsValid());
  }
}

TEST_P(TestVFSEntry, RejectsMissingEntryArraysWithoutPassingThemToFreeDirectory)
{
  vfs.entries = nullptr;
  CFileItemList items;
  EXPECT_FALSE(Read(items));
  EXPECT_TRUE(items.IsEmpty());
  EXPECT_EQ(vfs.freeCalls, 0);
}

TEST_P(TestVFSEntry, RejectsNegativeCountsWithMissingEntries)
{
  vfs.count = -1;
  vfs.entries = nullptr;
  CFileItemList items;
  EXPECT_FALSE(Read(items));
  EXPECT_TRUE(items.IsEmpty());
  EXPECT_EQ(vfs.freeCalls, 0);
}

TEST_P(TestVFSEntry, RejectsNegativeCountsWithExistingEntries)
{
  vfs.count = -1;
  CFileItemList items;
  EXPECT_FALSE(Read(items));
  EXPECT_TRUE(items.IsEmpty());
  EXPECT_EQ(vfs.freeCalls, 0);
}

TEST_P(TestVFSEntry, PreservesEmptyDirectoryOwnership)
{
  vfs.entries = nullptr;
  vfs.count = 0;
  CFileItemList items;
  EXPECT_TRUE(Read(items));
  EXPECT_TRUE(items.IsEmpty());
  EXPECT_EQ(vfs.freeCalls, 1);
  EXPECT_EQ(vfs.freedEntries, nullptr);
  EXPECT_EQ(vfs.freedCount, 0);
}

TEST_P(TestVFSEntry, DoesNotFreeFailedDirectoryResults)
{
  vfs.success = false;
  CFileItemList items;
  EXPECT_FALSE(Read(items));
  EXPECT_TRUE(items.IsEmpty());
  EXPECT_EQ(vfs.freeCalls, 0);
}

TEST_P(TestVFSEntry, SkipsMissingPathsAndReleasesTheOriginalArray)
{
  VFSDirEntry entries[]{entry, entry};
  entries[0].path = nullptr;
  vfs.entries = entries;
  vfs.count = 2;
  CFileItemList items;
  ASSERT_TRUE(Read(items));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetPath(), path);
  EXPECT_EQ(vfs.freeCalls, 1);
  EXPECT_EQ(vfs.freedEntries, entries);
  EXPECT_EQ(vfs.freedCount, 2);
}

TEST_P(TestVFSEntry, KeepsEntriesWithMissingOptionalMetadata)
{
  entry.label = nullptr;
  entry.num_props = 1;
  entry.properties = nullptr;
  CFileItemList items;
  ASSERT_TRUE(Read(items));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetPath(), path);
  EXPECT_TRUE(items[0]->GetLabel().empty());
  EXPECT_TRUE(items[0]->GetTitle().empty());
}

TEST_P(TestVFSEntry, SkipsNullPropertiesAndPreservesValidMetadata)
{
  char name[]{"example"};
  char missingValue[]{"missingValue"};
  char value[]{"value"};
  char preformatted[]{"PropMisusePreformatted"};
  char truth[]{"TrUe"};
  char title[]{"Title"};
  VFSProperty properties[]{
      {nullptr, value}, {missingValue, nullptr}, {name, value}, {preformatted, truth}};
  entry.properties = properties;
  entry.num_props = 4;
  entry.title = title;
  entry.folder = true;
  CFileItemList items;
  ASSERT_TRUE(Read(items));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetLabel(), label);
  EXPECT_EQ(items[0]->GetTitle(), title);
  EXPECT_TRUE(items[0]->IsFolder());
  EXPECT_EQ(items[0]->GetProperty(name).asString(), value);
  EXPECT_FALSE(items[0]->HasProperty(missingValue));
  EXPECT_TRUE(items[0]->IsLabelPreformatted());
  EXPECT_FALSE(items[0]->HasProperty(preformatted));
  if (GetParam())
    EXPECT_EQ(items.GetPath(), "test://archive/");

  truth[0] = 'f';
  CFileItemList next;
  ASSERT_TRUE(Read(next));
  ASSERT_EQ(next.Size(), 1);
  EXPECT_FALSE(next[0]->IsLabelPreformatted());
}

INSTANTIATE_TEST_SUITE_P(DirectoryAndFileDirectory, TestVFSEntry, testing::Bool());
} // namespace
