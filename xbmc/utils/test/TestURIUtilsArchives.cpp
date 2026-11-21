/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <array>
#include <ostream>
#include <string>

#include <gtest/gtest.h>

namespace
{

struct ArchivePredicateCase
{
  const char* name;
  bool (*predicate)(const std::string& path);
  const char* path;
  bool expected;
};

const ArchivePredicateCase ArchivePredicateCases[] = {
    {"IsZIP", URIUtils::IsZIP, "/path/to/zipfile.zip", true},
    {"IsZIP", URIUtils::IsZIP, "/path/to/zipfile.cbz", true},
    {"IsZIP", URIUtils::IsZIP, "/path/to/file", false},
    {"IsZIP", URIUtils::IsZIP, "zip://path/to/file", false},
    {"IsRAR", URIUtils::IsRAR, "/path/to/rarfile.rar", true},
    {"IsRAR", URIUtils::IsRAR, "/path/to/rarfile.cbr", true},
    {"IsRAR", URIUtils::IsRAR, "/path/to/file", false},
    {"IsRAR", URIUtils::IsRAR, "rar://path/to/file", false},
    {"IsInZIP", URIUtils::IsInZIP, "zip://path/to/file", true},
    {"IsInRAR", URIUtils::IsInRAR, "rar://path/to/file", true},
    {"IsInArchive", URIUtils::IsInArchive, "zip://path/to/file", true},
};

struct ArchivePathCase
{
  const char* protocol;
  const char* archive;
  const char* pathInArchive;
  const char* expected;
};

const ArchivePathCase CreateArchivePathCases[] = {
    {"zip", "/path/to/", "file", "zip://%2fpath%2fto%2f/file"},
    {"zip", "/path/thing", "/my/archived/path", "zip://%2fpath%2fthing/my/archived/path"},
    {"apk", "/path/thing", "/my/archived/path", "apk://%2fpath%2fthing/my/archived/path"},
    {"udf", "/path/thing", "/my/archived/path", "udf://%2fpath%2fthing/my/archived/path"},
    {"iso9660", "/path/thing", "/my/archived/path", "iso9660://%2fpath%2fthing/my/archived/path"},
    {"rar", "/path/thing", "/my/archived/path", "rar://%2fpath%2fthing/my/archived/path"},
};

struct CURLArchiveConstructionTestData
{
  std::string input;
  std::string protocol;
  std::string filename;
  std::string hostname;
  bool translateFilename{false};
};

std::ostream& operator<<(std::ostream& os, const CURLArchiveConstructionTestData& data)
{
  return os << "CURLArchiveConstructionTestData { " << data.input << " }" << std::endl;
}

const auto ArchiveFileParsingTests = std::array{
    // Zip file tests
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/does_not_exist.zip/kodi-dev.png",
        "",
        "xbmc/utils/test/resources/does_not_exist.zip/kodi-dev.png",
        "",
        true,
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/zipfile.zip/kodi-dev.png",
        "zip",
        "kodi-dev.png",
        "xbmc/utils/test/resources/zipfile.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/does_not_exist.png",
        "zip",
        "does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip",
        "zip",
        "zipfile.zip",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/rarfile.rar",
        "zip",
        "rarfile.rar",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip/does_not_exist.png",
        "zip",
        "zipfile.zip/does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip/kodi-dev.png",
        "zip",
        "zipfile.zip/kodi-dev.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/directory/does_not_exist.png",
        "zip",
        "directory/does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/directory/kodi-dev.png",
        "zip",
        "directory/kodi-dev.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
};

class ArchiveFileParsingTester : public testing::Test,
                                 public testing::WithParamInterface<CURLArchiveConstructionTestData>
{
};

} // namespace

TEST(TestURIUtilsArchives, Predicates)
{
  for (const auto& [name, predicate, path, expected] : ArchivePredicateCases)
  {
    EXPECT_EQ(expected, predicate(path)) << name << "(" << path << ")";
  }
}

TEST(TestURIUtilsArchives, CreateArchivePath)
{
  for (const auto& [protocol, archive, pathInArchive, expected] : CreateArchivePathCases)
  {
    EXPECT_EQ(expected, URIUtils::CreateArchivePath(protocol, CURL(archive), pathInArchive).Get());
  }
}

TEST_P(ArchiveFileParsingTester, TestArchiveFileParsingNativeSlashes)
{
  const auto& param = GetParam();
  const std::string path = XBMC_REF_FILE_PATH(param.input);
  const CURL url(path);

  EXPECT_EQ(param.protocol, url.GetProtocol());
  if (param.translateFilename)
  {
    EXPECT_EQ(XBMC_REF_FILE_PATH(param.filename), url.GetFileName());
    EXPECT_EQ(param.hostname, CURL::Decode(url.GetHostName()));
  }
  else
  {
    EXPECT_EQ(param.filename, url.GetFileName());
    EXPECT_EQ(XBMC_REF_FILE_PATH(param.hostname), CURL::Decode(url.GetHostName()));
  }
}

INSTANTIATE_TEST_SUITE_P(TestURIUtilsArchives,
                         ArchiveFileParsingTester,
                         testing::ValuesIn(ArchiveFileParsingTests));
