/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSource.h"
#include "URL.h"

#include <array>

#include <gtest/gtest.h>

namespace
{

CMediaSource createRef(const std::string& path,
                       const std::string& name,
                       SourceType type = SourceType::LOCAL)
{
  CMediaSource source;
  source.strPath = path;
  source.strName = name;
  source.m_iDriveType = type;
  source.vecPaths = {path};
  source.m_allowSharing = false;
  return source;
}

CMediaSource createRefv(const std::string& path,
                        const std::vector<std::string>& paths,
                        const std::string& name,
                        SourceType type)
{
  CMediaSource source;
  source.strPath = path;
  source.strName = name;
  source.m_iDriveType = type;
  source.vecPaths = paths;
  source.m_allowSharing = false;
  return source;
}

struct FromNameAndPathDef
{
  std::vector<std::string> path;
  std::string name;
};

struct FromNameAndPathTestDef
{
  FromNameAndPathDef in;
  CMediaSource ref;
};

const auto fromname_tests = std::array{
    FromNameAndPathTestDef{FromNameAndPathDef{{"/some/where/"}, "name"},
                           createRef("/some/where/", "name", SourceType::LOCAL)},
    FromNameAndPathTestDef{
        FromNameAndPathDef{{"/home/user/bar/", "/home/user/foo/"}, "yolo"},
        createRefv("multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f/",
                   {"/home/user/bar/", "/home/user/foo/"},
                   "yolo",
                   SourceType::VPATH)},
    FromNameAndPathTestDef{FromNameAndPathDef{{"iso9660://foo/"}, "name"},
                           createRef("iso9660://foo/", "name", SourceType::VIRTUAL_OPTICAL_DISC)},
    FromNameAndPathTestDef{
        FromNameAndPathDef{{"udf://foo/"}, "name"},
        createRefv(CURL("D:\\").Get(), {"udf://foo/"}, "name", SourceType::VIRTUAL_OPTICAL_DISC)},
    FromNameAndPathTestDef{FromNameAndPathDef{{"dvd://1"}, "name"},
                           createRefv("dvd://1/", {"dvd://1"}, "name", SourceType::OPTICAL_DISC)},
    FromNameAndPathTestDef{FromNameAndPathDef{{"smb://some/where/"}, "lets samba!"},
                           createRef("smb://some/where/", "lets samba!", SourceType::REMOTE)},
};

class FromNameAndPathTest : public testing::WithParamInterface<FromNameAndPathTestDef>,
                            public testing::Test
{
};

struct AddOrReplaceTestDef
{
  std::vector<CMediaSource> sources;
  std::vector<CMediaSource> replace;
  std::vector<CMediaSource> ref;
};

const auto addorreplace_tests = std::array{
    AddOrReplaceTestDef{
        {createRef("/some/where/", "first"), createRef("/some/where2/", "second")},
        {createRef("/some/where2/", "replace")},
        {createRef("/some/where/", "first"), createRef("/some/where2/", "replace")}},
    AddOrReplaceTestDef{{createRef("/some/where/", "first"), createRef("/some/nowhere/", "second")},
                        {createRef("/some/where2/", "third")},
                        {createRef("/some/where/", "first"), createRef("/some/nowhere/", "second"),
                         createRef("/some/where2/", "third")}},
    AddOrReplaceTestDef{
        {createRef("/some/where/", "first"), createRef("/some/where2/", "second")},
        {createRef("/some/where/", "replace1"), createRef("/some/where2/", "replace2")},
        {createRef("/some/where/", "replace1"), createRef("/some/where2/", "replace2")}},
    AddOrReplaceTestDef{
        {createRef("/some/where/", "first"), createRef("/some/nowhere/", "second")},
        {createRef("/some/where/", "replace1"), createRef("/some/nowhere2/", "third")},
        {createRef("/some/where/", "replace1"), createRef("/some/nowhere/", "second"),
         createRef("/some/nowhere2/", "third")}},
};

class AddOrReplaceTest : public testing::WithParamInterface<AddOrReplaceTestDef>,
                         public testing::Test
{
};

} // namespace

TEST_P(FromNameAndPathTest, FromNameAndPath)
{
  CMediaSource source;
  source.FromNameAndPaths(GetParam().in.name, GetParam().in.path);

  EXPECT_EQ(source.strName, GetParam().ref.strName);
  EXPECT_EQ(source.strPath, GetParam().ref.strPath);
  EXPECT_EQ(source.m_iDriveType, GetParam().ref.m_iDriveType);
  EXPECT_EQ(source.vecPaths, GetParam().ref.vecPaths);

  EXPECT_EQ(source, GetParam().ref);
}

INSTANTIATE_TEST_SUITE_P(TestMediaSource, FromNameAndPathTest, testing::ValuesIn(fromname_tests));

TEST_P(AddOrReplaceTest, AddOrReplace)
{
  auto sources = GetParam().sources;
  if (GetParam().replace.size() == 1)
    AddOrReplace(sources, GetParam().replace[0]);
  else
    AddOrReplace(sources, GetParam().replace);

  ASSERT_EQ(sources.size(), GetParam().ref.size());
  for (size_t i = 0; i < sources.size(); ++i)
    EXPECT_EQ(sources[i], GetParam().ref[i]) << "error for index " << i;
}

INSTANTIATE_TEST_SUITE_P(TestMediaSource, AddOrReplaceTest, testing::ValuesIn(addorreplace_tests));
