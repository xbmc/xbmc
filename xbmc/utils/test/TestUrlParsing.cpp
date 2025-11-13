/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/**
 * @file TestUrlParsing.cpp
 * @brief Unit tests for verifying URL parsing into CURL objects.
 *
 * This test suite validates the correctness and robustness of Kodi's URL
 * parsing implementation by ensuring that different types of URLs are
 * correctly decomposed into their component parts (scheme, host, port,
 * path, query, etc.) within CURL objects.
 *
 * ## Overview
 * Each test loads URL test cases from JSON files that define the expected
 * parse results. The tests then verify that the CURL object correctly
 * represents each URL as expected. This provides a comprehensive regression
 * suite to guard against changes or regressions in URL parsing logic.
 *
 * ## Test Data
 * The input and expected output for these tests are stored as JSON files
 * under `xbmc/utils/test/testdata/`. As there are many such test files, it
 * is often easier to regenerate them rather than edit each file manually.
 *
 * To regenerate all test case JSON files, define the following symbol before
 * building or running the tests:
 *
 * @code
 * #define GENERATE_JSON_TEST_FILES
 * @endcode
 *
 * This can be defined either inline in this test file or passed via the
 * build system when building `kodi-test`. When defined, executing the tests
 * will generate updated JSON test case files in your **CMake build
 * directory**.
 *
 * After generation, the new JSON files should be reviewed and copied or moved
 * into the `xbmc/utils/test/testdata/` directory before committing them with
 * your branch.
 *
 * ## Running the Tests
 * These tests are part of the `kodi-test` suite and are executed using
 * GoogleTest. To run all URL parsing tests:
 *
 * @code
 * kodi-test --gtest_filter=TestURLParse*
 * @endcode
 *
 * ## Modifying or Adding Tests
 * - To add a new test case, create or update a JSON file in the testdata
 *   directory and ensure it follows the existing format.
 *   Filename: increment the last json file number and the constant LAST_JSON_FILE_NUMBER
 *   Do not create gaps in the file numbers sequence.
 * - When modifying the parsing logic in Kodi, regenerate the JSON test files
 *   to reflect the new expected behavior.
 * - Always run the full `TestURLParseDetails` suite after changes to confirm
 *   no regressions were introduced.
 *
 * @see CURL
 * @see xbmc/utils/test/testdata/
 */

#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/MultiPathDirectory.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

#include <utility>

#include <gtest/gtest.h>

// JSON Output
#include "test/TestUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include <fstream>

constexpr int LAST_JSON_FILE_NUMBER = 748;

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

namespace
{

struct TestURLParseDetailsData
{
  std::string input = "";
  std::string expectedGet = "";
  int expectedGetPort = 0;
  std::string expectedGetHostName = "";
  std::string expectedGetDomain = "";
  std::string expectedGetUserName = "";
  std::string expectedGetPassWord = "";
  std::string expectedGetFileName = "";
  std::string expectedGetFileNameUtil = "";
  std::string expectedGetProtocol = "";
  std::string expectedGetTranslatedProtocol = "";
  std::string expectedGetShareName = "";
  std::string expectedGetOptions = "";
  std::string expectedGetProtocolOptions = "";
  std::string expectedGetFileNameWithoutPath = "";
  std::string expectedGetWithoutOptions = "";
  std::string expectedGetWithoutUserDetails_redacted = "";
  std::string expectedGetWithoutUserDetails_removed = "";
  std::string expectedGetWithoutFilename = "";
  std::string expectedGetRedacted = "";
  bool expectedIsLocal = false;
  bool expectedIsLocalHost = false;
  bool expectedIsFileOnly = false;
  bool expectedIsFullPath = false;
  std::string expectedDecode = "";
  std::string expectedDecodeFileName = "";
  std::string expectedEncode = "";
  std::string expectedExtension = "";
  bool expectedHasExtension = false;
  bool expectedHasSetExtension = false;
  std::string expectedRemovedExtension = "";
  std::string expectedReplacedExtension = "";
  std::string expectedFileOrFolder = "";
  std::string expectedSplitPath = "";
  std::string expectedSplitFileName = "";
  bool expectedHasParentInHostname = false;
  bool expectedHasEncodedHostname = false;
  bool expectedHasEncodedFilename = false;
  std::string expectedHasParentPath = "";
  std::string expectedGetParentPath = "";
  std::string expectedGetBasePath = "";
  std::string expectedGetDiscBase = "";
  std::string expectedGetDiscBasePath = "";
  std::string expectedGetDiscUnderlyingFile = "";
  std::string expectedGetBlurayFile = "";
  std::string expectedGetBlurayRootPath = "";
  std::string expectedGetBlurayTitlesPath = "";
  std::string expectedGetBlurayEpisodePath = "";
  std::string expectedGetBlurayAllEpisodesPath = "";
  std::string expectedGetBlurayPlaylistPath = "";
  bool expectedIsMusicDBProtocol = false;
  bool expectedIsUDFProtocol = false;
  bool expectedIsRemote = false;
  bool expectedIsOnDVD = false;
  bool expectedIsOnLAN = false;
  bool expectedIsHostOnLAN = false;
  bool expectedIsMultiPath = false;
  bool expectedIsHD = false;
  bool expectedIsStack = false;
  bool expectedIsFavourite = false;
  bool expectedIsRAR = false;
  bool expectedIsInArchive = false;
  bool expectedIsInAPK = false;
  bool expectedIsInZIP = false;
  bool expectedIsInRAR = false;
  bool expectedIsAPK = false;
  bool expectedIsZIP = false;
  bool expectedIsArchive = false;
  bool expectedIsDiscImage = false;
  bool expectedIsDiscImageStack = false;
  bool expectedIsSpecial = false;
  bool expectedIsPlugin = false;
  bool expectedIsScript = false;
  bool expectedIsAddonsPath = false;
  bool expectedIsSourcesPath = false;
  bool expectedIsCDDA = false;
  bool expectedIsISO9660 = false;
  bool expectedIsSmb = false;
  bool expectedIsURL = false;
  bool expectedIsFTP = false;
  bool expectedIsHTTP = false;
  bool expectedIsUDP = false;
  bool expectedIsTCP = false;
  bool expectedIsPVR = false;
  bool expectedIsPVRChannel = false;
  bool expectedIsPVRRadioChannel = false;
  bool expectedIsPVRChannelGroup = false;
  bool expectedIsPVRGuideItem = false;
  bool expectedIsDAV = false;
  bool expectedIsInternetStream = false;
  bool expectedIsInternetStreamStrict = false;
  bool expectedIsStreamedFilesystem = false;
  bool expectedIsNetworkFilesystem = false;
  bool expectedIsUPnP = false;
  bool expectedIsLiveTV = false;
  bool expectedIsPVRRecording = false;
  bool expectedIsPVRRecordingFileOrFolder = false;
  bool expectedIsPVRTVRecordingFileOrFolder = false;
  bool expectedIsPVRRadioRecordingFileOrFolder = false;
  bool expectedIsMusicDb = false;
  bool expectedIsNfs = false;
  bool expectedIsVideoDb = false;
  bool expectedIsBlurayPath = false;
  bool expectedIsOpticalMediaFile = false;
  bool expectedIsBDFile = false;
  bool expectedIsDVDFile = false;
  bool expectedIsAndroidApp = false;
  bool expectedIsLibraryFolder = false;
  bool expectedIsLibraryContent = false;
  bool expectedIsDOSPath = false;
  std::string expectedAppendSlash = "";
  bool expectedHasSlashAtEnd = false;
  bool expectedHasSlashAtEndURL = false;
  std::string expectedRemovedEndSlash = "";
  std::string expectedIsFixSlashesAndDupsUnix = "";
  std::string expectedIsFixSlashesAndDupsWin = "";
  std::string expectedCanonicalizePathUnix = "";
  std::string expectedCanonicalizePathWin = "";
  std::string expectedAddFileToFolder = "";
  std::string expectedGetDirectory = "";
  std::string expectedCreateZIPArchivePath = "";
  std::string expectedCreateRARArchivePath = "";
  std::string expectedCreateAPKArchivePath = "";
  std::string expectedGetRealPath = "";
  std::string expectedUpdateUrlEncoding = "";
};

void read(bool& output, const CVariant& jsonObj, const char* key)
{
  if (jsonObj.isMember(key))
    output = jsonObj[key].asBoolean();
}

void read(int& output, const CVariant& jsonObj, const char* key)
{
  if (jsonObj.isMember(key))
    output = jsonObj[key].asInteger();
}

void read(std::string& output, const CVariant& jsonObj, const char* key)
{
  if (jsonObj.isMember(key))
    output = jsonObj[key].asString();
}

#ifdef GENERATE_JSON_TEST_FILES
void write(CVariant& jsonObj, const char* key, const bool value)
{
  if (value != false)
    jsonObj[key] = value;
}

void write(CVariant& jsonObj, const char* key, const int value)
{
  if (value != 0)
    jsonObj[key] = value;
}

void write(CVariant& jsonObj, const char* key, const std::string& value)
{
  if (value != "")
    jsonObj[key] = value;
}
#endif // GENERATE_JSON_TEST_FILES

TestURLParseDetailsData CreateParamFromJson(std::string filename)
{
  CVariant json;
  TestURLParseDetailsData param;

  std::ifstream reader(filename);
  std::stringstream buffer;
  buffer << reader.rdbuf();
  std::string inputJson = buffer.str();
  reader.close();

  if (CJSONVariantParser::Parse(inputJson, json) && !json.isNull())
  {
    read(param.input, json, "__input");
    read(param.expectedGet, json, "expectedGet");
    read(param.expectedGetPort, json, "expectedGetPort");
    read(param.expectedGetHostName, json, "expectedGetHostName");
    read(param.expectedGetDomain, json, "expectedGetDomain");
    read(param.expectedGetUserName, json, "expectedGetUserName");
    read(param.expectedGetPassWord, json, "expectedGetPassWord");
    read(param.expectedGetFileName, json, "expectedGetFileName");
    read(param.expectedGetFileNameUtil, json, "expectedGetFileNameUtil");
    read(param.expectedGetProtocol, json, "expectedGetProtocol");
    read(param.expectedGetTranslatedProtocol, json, "expectedGetTranslatedProtocol");
    read(param.expectedGetShareName, json, "expectedGetShareName");
    read(param.expectedGetOptions, json, "expectedGetOptions");
    read(param.expectedGetProtocolOptions, json, "expectedGetProtocolOptions");
    read(param.expectedGetFileNameWithoutPath, json, "expectedGetFileNameWithoutPath");
    read(param.expectedGetWithoutOptions, json, "expectedGetWithoutOptions");
    read(param.expectedGetWithoutUserDetails_redacted, json,
         "expectedGetWithoutUserDetails_redacted");
    read(param.expectedGetWithoutUserDetails_removed, json,
         "expectedGetWithoutUserDetails_removed");
    read(param.expectedGetWithoutFilename, json, "expectedGetWithoutFilename");
    read(param.expectedGetRedacted, json, "expectedGetRedacted");
    read(param.expectedIsLocal, json, "expectedIsLocal");
    read(param.expectedIsLocalHost, json, "expectedIsLocalHost");
    read(param.expectedIsFileOnly, json, "expectedIsFileOnly");
    read(param.expectedIsFullPath, json, "expectedIsFullPath");
    read(param.expectedDecode, json, "expectedDecode");
    read(param.expectedDecodeFileName, json, "expectedDecodeFileName");
    read(param.expectedEncode, json, "expectedEncode");
    read(param.expectedExtension, json, "expectedExtension");
    read(param.expectedHasExtension, json, "expectedHasExtension");
    read(param.expectedHasSetExtension, json, "expectedHasSetExtension");
    read(param.expectedRemovedExtension, json, "expectedRemovedExtension");
    read(param.expectedReplacedExtension, json, "expectedReplacedExtension");
    read(param.expectedFileOrFolder, json, "expectedFileOrFolder");
    read(param.expectedSplitPath, json, "expectedSplitPath");
    read(param.expectedSplitFileName, json, "expectedSplitFileName");
    read(param.expectedHasParentInHostname, json, "expectedHasParentInHostname");
    read(param.expectedHasEncodedHostname, json, "expectedHasEncodedHostname");
    read(param.expectedHasEncodedFilename, json, "expectedHasEncodedFilename");
    read(param.expectedHasParentPath, json, "expectedHasParentPath");
    read(param.expectedGetParentPath, json, "expectedGetParentPath");
    read(param.expectedGetBasePath, json, "expectedGetBasePath");
    read(param.expectedGetDiscBase, json, "expectedGetDiscBase");
    read(param.expectedGetDiscBasePath, json, "expectedGetDiscBasePath");
    read(param.expectedGetDiscUnderlyingFile, json, "expectedGetDiscUnderlyingFile");
    read(param.expectedGetBlurayFile, json, "expectedGetBlurayFile");
    read(param.expectedGetBlurayRootPath, json, "expectedGetBlurayRootPath");
    read(param.expectedGetBlurayTitlesPath, json, "expectedGetBlurayTitlesPath");
    read(param.expectedGetBlurayEpisodePath, json, "expectedGetBlurayEpisodePath");
    read(param.expectedGetBlurayAllEpisodesPath, json, "expectedGetBlurayAllEpisodesPath");
    read(param.expectedGetBlurayPlaylistPath, json, "expectedGetBlurayPlaylistPath");
    read(param.expectedIsMusicDBProtocol, json, "expectedIsMusicDBProtocol");
    read(param.expectedIsUDFProtocol, json, "expectedIsUDFProtocol");
    read(param.expectedIsRemote, json, "expectedIsRemote");
    read(param.expectedIsOnDVD, json, "expectedIsOnDVD");
    read(param.expectedIsOnLAN, json, "expectedIsOnLAN");
    read(param.expectedIsHostOnLAN, json, "expectedIsHostOnLAN");
    read(param.expectedIsMultiPath, json, "expectedIsMultiPath");
    read(param.expectedIsHD, json, "expectedIsHD");
    read(param.expectedIsStack, json, "expectedIsStack");
    read(param.expectedIsFavourite, json, "expectedIsFavourite");
    read(param.expectedIsRAR, json, "expectedIsRAR");
    read(param.expectedIsInArchive, json, "expectedIsInArchive");
    read(param.expectedIsInAPK, json, "expectedIsInAPK");
    read(param.expectedIsInZIP, json, "expectedIsInZIP");
    read(param.expectedIsInRAR, json, "expectedIsInRAR");
    read(param.expectedIsAPK, json, "expectedIsAPK");
    read(param.expectedIsZIP, json, "expectedIsZIP");
    read(param.expectedIsArchive, json, "expectedIsArchive");
    read(param.expectedIsDiscImage, json, "expectedIsDiscImage");
    read(param.expectedIsDiscImageStack, json, "expectedIsDiscImageStack");
    read(param.expectedIsSpecial, json, "expectedIsSpecial");
    read(param.expectedIsPlugin, json, "expectedIsPlugin");
    read(param.expectedIsScript, json, "expectedIsScript");
    read(param.expectedIsAddonsPath, json, "expectedIsAddonsPath");
    read(param.expectedIsSourcesPath, json, "expectedIsSourcesPath");
    read(param.expectedIsCDDA, json, "expectedIsCDDA");
    read(param.expectedIsISO9660, json, "expectedIsISO9660");
    read(param.expectedIsSmb, json, "expectedIsSmb");
    read(param.expectedIsURL, json, "expectedIsURL");
    read(param.expectedIsFTP, json, "expectedIsFTP");
    read(param.expectedIsHTTP, json, "expectedIsHTTP");
    read(param.expectedIsUDP, json, "expectedIsUDP");
    read(param.expectedIsTCP, json, "expectedIsTCP");
    read(param.expectedIsPVR, json, "expectedIsPVR");
    read(param.expectedIsPVRChannel, json, "expectedIsPVRChannel");
    read(param.expectedIsPVRRadioChannel, json, "expectedIsPVRRadioChannel");
    read(param.expectedIsPVRChannelGroup, json, "expectedIsPVRChannelGroup");
    read(param.expectedIsPVRGuideItem, json, "expectedIsPVRGuideItem");
    read(param.expectedIsDAV, json, "expectedIsDAV");
    read(param.expectedIsInternetStream, json, "expectedIsInternetStream");
    read(param.expectedIsInternetStreamStrict, json, "expectedIsInternetStreamStrict");
    read(param.expectedIsStreamedFilesystem, json, "expectedIsStreamedFilesystem");
    read(param.expectedIsNetworkFilesystem, json, "expectedIsNetworkFilesystem");
    read(param.expectedIsUPnP, json, "expectedIsUPnP");
    read(param.expectedIsLiveTV, json, "expectedIsLiveTV");
    read(param.expectedIsPVRRecording, json, "expectedIsPVRRecording");
    read(param.expectedIsPVRRecordingFileOrFolder, json, "expectedIsPVRRecordingFileOrFolder");
    read(param.expectedIsPVRTVRecordingFileOrFolder, json, "expectedIsPVRTVRecordingFileOrFolder");
    read(param.expectedIsPVRRadioRecordingFileOrFolder, json,
         "expectedIsPVRRadioRecordingFileOrFolder");
    read(param.expectedIsMusicDb, json, "expectedIsMusicDb");
    read(param.expectedIsNfs, json, "expectedIsNfs");
    read(param.expectedIsVideoDb, json, "expectedIsVideoDb");
    read(param.expectedIsBlurayPath, json, "expectedIsBlurayPath");
    read(param.expectedIsOpticalMediaFile, json, "expectedIsOpticalMediaFile");
    read(param.expectedIsBDFile, json, "expectedIsBDFile");
    read(param.expectedIsDVDFile, json, "expectedIsDVDFile");
    read(param.expectedIsAndroidApp, json, "expectedIsAndroidApp");
    read(param.expectedIsLibraryFolder, json, "expectedIsLibraryFolder");
    read(param.expectedIsLibraryContent, json, "expectedIsLibraryContent");
    read(param.expectedIsDOSPath, json, "expectedIsDOSPath");
    read(param.expectedAppendSlash, json, "expectedAppendSlash");
    read(param.expectedHasSlashAtEnd, json, "expectedHasSlashAtEnd");
    read(param.expectedHasSlashAtEndURL, json, "expectedHasSlashAtEndURL");
    read(param.expectedRemovedEndSlash, json, "expectedRemovedEndSlash");
    read(param.expectedIsFixSlashesAndDupsUnix, json, "expectedIsFixSlashesAndDupsUnix");
    read(param.expectedIsFixSlashesAndDupsWin, json, "expectedIsFixSlashesAndDupsWin");
    read(param.expectedCanonicalizePathUnix, json, "expectedCanonicalizePathUnix");
    read(param.expectedCanonicalizePathWin, json, "expectedCanonicalizePathWin");
    read(param.expectedAddFileToFolder, json, "expectedAddFileToFolder");
    read(param.expectedGetDirectory, json, "expectedGetDirectory");
    read(param.expectedCreateZIPArchivePath, json, "expectedCreateZIPArchivePath");
    read(param.expectedCreateRARArchivePath, json, "expectedCreateRARArchivePath");
    read(param.expectedCreateAPKArchivePath, json, "expectedCreateAPKArchivePath");
    read(param.expectedGetRealPath, json, "expectedGetRealPath");
    read(param.expectedUpdateUrlEncoding, json, "expectedUpdateUrlEncoding");
  }
  return param;
}

void RunParseTest(const std::string& filename, const TestURLParseDetailsData& param)
{
  const std::string p{param.input};
  const CURL curl(p);

  std::string tmp1, tmp2;

  tmp1 = p;
  URIUtils::RemoveExtension(tmp1);
  const auto removedExtension = tmp1;

  URIUtils::Split(p, tmp1, tmp2);
  const auto splitPath = tmp1;
  const auto splitFileName = tmp2;

  URIUtils::Split(curl.GetFileName(), tmp1, tmp2);
  const auto splitCurlPath = tmp1;
  const auto splitCurlFileName = tmp2;

  URIUtils::GetParentPath(p, tmp1);
  const auto parentPath = tmp1;

  tmp1 = p;
  URIUtils::RemoveSlashAtEnd(tmp1);
  const auto removedEndSlash = tmp1;

  tmp1 = p;
  URIUtils::UpdateUrlEncoding(tmp1);
  const auto updatedURLEncoding = tmp1;

  //EXPECT_EQ(URIUtils::GetFileName(p), splitCurlFileName) << p;
  //EXPECT_EQ(URIUtils::GetFileName(p), curl.GetFileNameWithoutPath()) << p;

  EXPECT_EQ(param.expectedGet, curl.Get());
  EXPECT_EQ(param.expectedGetPort, curl.GetPort());
  EXPECT_EQ(param.expectedGetHostName, curl.GetHostName());
  EXPECT_EQ(param.expectedGetDomain, curl.GetDomain());
  EXPECT_EQ(param.expectedGetUserName, curl.GetUserName());
  EXPECT_EQ(param.expectedGetPassWord, curl.GetPassWord());
  EXPECT_EQ(param.expectedGetFileName, curl.GetFileName());
  EXPECT_EQ(param.expectedGetFileNameUtil, URIUtils::GetFileName(p));
  EXPECT_EQ(param.expectedGetProtocol, curl.GetProtocol());
  EXPECT_EQ(param.expectedGetTranslatedProtocol, curl.GetTranslatedProtocol());
  EXPECT_EQ(param.expectedGetShareName, curl.GetShareName());
  EXPECT_EQ(param.expectedGetOptions, curl.GetOptions());
  EXPECT_EQ(param.expectedGetProtocolOptions, curl.GetProtocolOptions());
  EXPECT_EQ(param.expectedGetFileNameWithoutPath, curl.GetFileNameWithoutPath());
  EXPECT_EQ(param.expectedGetWithoutOptions, curl.GetWithoutOptions());
  EXPECT_EQ(param.expectedGetWithoutUserDetails_redacted, curl.GetWithoutUserDetails(true));
  EXPECT_EQ(param.expectedGetWithoutUserDetails_removed, curl.GetWithoutUserDetails(false));
  EXPECT_EQ(param.expectedGetWithoutFilename, curl.GetWithoutFilename());
  EXPECT_EQ(param.expectedGetRedacted, curl.GetRedacted());
  EXPECT_EQ(param.expectedIsLocal, curl.IsLocal());
  EXPECT_EQ(param.expectedIsLocalHost, curl.IsLocalHost());
  EXPECT_EQ(param.expectedIsFileOnly, CURL::IsFileOnly(p));
  EXPECT_EQ(param.expectedIsFullPath, CURL::IsFullPath(p));
  EXPECT_EQ(param.expectedDecode, CURL::Decode(p));
  EXPECT_EQ(param.expectedDecodeFileName, CURL::Decode(curl.GetFileName()));
  EXPECT_EQ(param.expectedEncode, CURL::Encode(p));
  EXPECT_EQ(param.expectedExtension, URIUtils::GetExtension(p));
  EXPECT_EQ(param.expectedExtension, curl.GetExtension());
  EXPECT_EQ(param.expectedHasExtension, URIUtils::HasExtension(p));
  EXPECT_EQ(param.expectedHasSetExtension, URIUtils::HasExtension(curl, "avi|txt"));
  EXPECT_EQ(param.expectedHasSetExtension, curl.HasExtension("avi|txt"));
  EXPECT_EQ(param.expectedRemovedExtension, removedExtension);
  EXPECT_EQ(param.expectedReplacedExtension, URIUtils::ReplaceExtension(p, "xyz"));
  EXPECT_EQ(param.expectedFileOrFolder, URIUtils::GetFileOrFolderName(p));
  EXPECT_EQ(param.expectedSplitPath, splitPath);
  EXPECT_EQ(param.expectedSplitFileName, splitFileName);
  EXPECT_EQ(param.expectedHasParentInHostname, URIUtils::HasParentInHostname(curl));
  EXPECT_EQ(param.expectedHasParentInHostname, curl.HasParentInHostname());
  EXPECT_EQ(param.expectedHasEncodedHostname, URIUtils::HasEncodedHostname(curl));
  EXPECT_EQ(param.expectedHasEncodedHostname, curl.HasEncodedHostname());
  EXPECT_EQ(param.expectedHasEncodedFilename, URIUtils::HasEncodedFilename(curl));
  EXPECT_EQ(param.expectedHasEncodedFilename, curl.HasEncodedFilename());
  EXPECT_EQ(param.expectedHasParentPath, URIUtils::GetParentPath(p));
  EXPECT_EQ(param.expectedGetParentPath, parentPath);
  EXPECT_EQ(param.expectedGetBasePath, URIUtils::GetBasePath(p));
  EXPECT_EQ(param.expectedGetDiscBase, URIUtils::GetDiscBase(p));
  EXPECT_EQ(param.expectedGetDiscBasePath, URIUtils::GetDiscBasePath(p));
  EXPECT_EQ(param.expectedGetDiscUnderlyingFile, URIUtils::GetDiscUnderlyingFile(curl));
  EXPECT_EQ(param.expectedGetBlurayFile, URIUtils::GetDiscFile(p));
  EXPECT_EQ(param.expectedGetBlurayRootPath, URIUtils::GetBlurayRootPath(p));
  EXPECT_EQ(param.expectedGetBlurayTitlesPath, URIUtils::GetBlurayTitlesPath(p));
  EXPECT_EQ(param.expectedGetBlurayEpisodePath, URIUtils::GetBlurayEpisodePath(p, 1, 2));
  EXPECT_EQ(param.expectedGetBlurayAllEpisodesPath, URIUtils::GetBlurayAllEpisodesPath(p));
  EXPECT_EQ(param.expectedGetBlurayPlaylistPath, URIUtils::GetBlurayPlaylistPath(p));
  EXPECT_EQ(param.expectedIsMusicDBProtocol, URIUtils::IsProtocol(p, "musicdb"));
  EXPECT_EQ(param.expectedIsUDFProtocol, URIUtils::IsProtocol(p, "udf"));
  EXPECT_EQ(param.expectedIsRemote, URIUtils::IsRemote(p));
  EXPECT_EQ(param.expectedIsOnDVD, URIUtils::IsOnDVD(p));
  EXPECT_EQ(param.expectedIsOnLAN, URIUtils::IsOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  EXPECT_EQ(param.expectedIsHostOnLAN, URIUtils::IsHostOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  EXPECT_EQ(param.expectedIsMultiPath, URIUtils::IsMultiPath(p));
  EXPECT_EQ(param.expectedIsMultiPath, curl.IsMultiPath());
  EXPECT_EQ(param.expectedIsHD, URIUtils::IsHD(p));
  EXPECT_EQ(param.expectedIsStack, URIUtils::IsStack(p));
  EXPECT_EQ(param.expectedIsStack, curl.IsStack());
  EXPECT_EQ(param.expectedIsFavourite, URIUtils::IsFavourite(p));
  EXPECT_EQ(param.expectedIsFavourite, curl.IsFavourite());
  EXPECT_EQ(param.expectedIsRAR, URIUtils::IsRAR(p));
  EXPECT_EQ(param.expectedIsInArchive, URIUtils::IsInArchive(p));
  EXPECT_EQ(param.expectedIsInAPK, URIUtils::IsInAPK(p));
  EXPECT_EQ(param.expectedIsInZIP, URIUtils::IsInZIP(p));
  EXPECT_EQ(param.expectedIsInRAR, URIUtils::IsInRAR(p));
  EXPECT_EQ(param.expectedIsAPK, URIUtils::IsAPK(p));
  EXPECT_EQ(param.expectedIsAPK, curl.IsAPK());
  EXPECT_EQ(param.expectedIsZIP, URIUtils::IsZIP(p));
  EXPECT_EQ(param.expectedIsZIP, curl.IsZIP());
  EXPECT_EQ(param.expectedIsArchive, URIUtils::IsArchive(p));
  EXPECT_EQ(param.expectedIsArchive, curl.IsArchive());
  EXPECT_EQ(param.expectedIsDiscImage, URIUtils::IsDiscImage(p));
  EXPECT_EQ(param.expectedIsDiscImage, curl.IsDiscImage());
  EXPECT_EQ(param.expectedIsDiscImageStack, URIUtils::IsDiscImageStack(p));
  EXPECT_EQ(param.expectedIsSpecial, URIUtils::IsSpecial(p));
  EXPECT_EQ(param.expectedIsPlugin, URIUtils::IsPlugin(p));
  EXPECT_EQ(param.expectedIsPlugin, curl.IsPlugin());
  EXPECT_EQ(param.expectedIsScript, URIUtils::IsScript(p));
  EXPECT_EQ(param.expectedIsScript, curl.IsScript());
  EXPECT_EQ(param.expectedIsAddonsPath, URIUtils::IsAddonsPath(p));
  EXPECT_EQ(param.expectedIsAddonsPath, curl.IsAddonsPath());
  EXPECT_EQ(param.expectedIsSourcesPath, URIUtils::IsSourcesPath(p));
  EXPECT_EQ(param.expectedIsSourcesPath, curl.IsSourcesPath());
  EXPECT_EQ(param.expectedIsCDDA, URIUtils::IsCDDA(p));
  EXPECT_EQ(param.expectedIsCDDA, curl.IsCDDA());
  EXPECT_EQ(param.expectedIsISO9660, URIUtils::IsISO9660(p));
  EXPECT_EQ(param.expectedIsISO9660, curl.IsISO9660());
  EXPECT_EQ(param.expectedIsSmb, URIUtils::IsSmb(p));
  EXPECT_EQ(param.expectedIsURL, URIUtils::IsURL(p));
  EXPECT_EQ(param.expectedIsFTP, URIUtils::IsFTP(p));
  EXPECT_EQ(param.expectedIsHTTP, URIUtils::IsHTTP(p));
  EXPECT_EQ(param.expectedIsUDP, URIUtils::IsUDP(p));
  EXPECT_EQ(param.expectedIsTCP, URIUtils::IsTCP(p));
  EXPECT_EQ(param.expectedIsPVR, URIUtils::IsPVR(p));
  EXPECT_EQ(param.expectedIsPVRChannel, URIUtils::IsPVRChannel(p));
  EXPECT_EQ(param.expectedIsPVRRadioChannel, URIUtils::IsPVRRadioChannel(p));
  EXPECT_EQ(param.expectedIsPVRChannelGroup, URIUtils::IsPVRChannelGroup(p));
  EXPECT_EQ(param.expectedIsPVRGuideItem, URIUtils::IsPVRGuideItem(p));
  EXPECT_EQ(param.expectedIsDAV, URIUtils::IsDAV(p));
  EXPECT_EQ(param.expectedIsInternetStream, URIUtils::IsInternetStream(p, false));
  EXPECT_EQ(param.expectedIsInternetStreamStrict, URIUtils::IsInternetStream(p, true));
  EXPECT_EQ(param.expectedIsStreamedFilesystem, URIUtils::IsStreamedFilesystem(p));
  EXPECT_EQ(param.expectedIsNetworkFilesystem, URIUtils::IsNetworkFilesystem(p));
  EXPECT_EQ(param.expectedIsUPnP, URIUtils::IsUPnP(p));
  EXPECT_EQ(param.expectedIsUPnP, curl.IsUPnP());
  EXPECT_EQ(param.expectedIsLiveTV, URIUtils::IsLiveTV(p));
  EXPECT_EQ(param.expectedIsPVRRecording, URIUtils::IsPVRRecording(p));
  EXPECT_EQ(param.expectedIsPVRRecordingFileOrFolder, URIUtils::IsPVRRecordingFileOrFolder(p));
  EXPECT_EQ(param.expectedIsPVRTVRecordingFileOrFolder, URIUtils::IsPVRTVRecordingFileOrFolder(p));
  EXPECT_EQ(param.expectedIsPVRRadioRecordingFileOrFolder,
            URIUtils::IsPVRRadioRecordingFileOrFolder(p));
  EXPECT_EQ(param.expectedIsMusicDb, URIUtils::IsMusicDb(p));
  EXPECT_EQ(param.expectedIsMusicDb, curl.IsMusicDb());
  EXPECT_EQ(param.expectedIsNfs, URIUtils::IsNfs(p));
  EXPECT_EQ(param.expectedIsVideoDb, URIUtils::IsVideoDb(p));
  EXPECT_EQ(param.expectedIsVideoDb, curl.IsVideoDb());
  EXPECT_EQ(param.expectedIsBlurayPath, URIUtils::IsBlurayPath(p));
  EXPECT_EQ(param.expectedIsBlurayPath, curl.IsBlurayPath());
  EXPECT_EQ(param.expectedIsOpticalMediaFile, URIUtils::IsOpticalMediaFile(p));
  EXPECT_EQ(param.expectedIsOpticalMediaFile, curl.IsOpticalMediaFile());
  EXPECT_EQ(param.expectedIsBDFile, URIUtils::IsBDFile(p));
  EXPECT_EQ(param.expectedIsBDFile, curl.IsBDFile());
  EXPECT_EQ(param.expectedIsDVDFile, URIUtils::IsDVDFile(p));
  EXPECT_EQ(param.expectedIsDVDFile, curl.IsDVDFile());
  EXPECT_EQ(param.expectedIsAndroidApp, URIUtils::IsAndroidApp(p));
  EXPECT_EQ(param.expectedIsAndroidApp, curl.IsAndroidApp());
  EXPECT_EQ(param.expectedIsLibraryFolder, URIUtils::IsLibraryFolder(p));
  EXPECT_EQ(param.expectedIsLibraryFolder, curl.IsLibraryFolder());
  EXPECT_EQ(param.expectedIsLibraryContent, URIUtils::IsLibraryContent(p));
  EXPECT_EQ(param.expectedIsLibraryContent, curl.IsLibraryContent());
  EXPECT_EQ(param.expectedIsDOSPath, URIUtils::IsDOSPath(p));
  EXPECT_EQ(param.expectedAppendSlash, URIUtils::AppendSlash(p));
  EXPECT_EQ(param.expectedHasSlashAtEnd, URIUtils::HasSlashAtEnd(p, false));
  EXPECT_EQ(param.expectedHasSlashAtEndURL, URIUtils::HasSlashAtEnd(p, true));
  EXPECT_EQ(param.expectedRemovedEndSlash, removedEndSlash);
  EXPECT_EQ(param.expectedIsFixSlashesAndDupsUnix, URIUtils::FixSlashesAndDups(p, '/'));
  EXPECT_EQ(param.expectedIsFixSlashesAndDupsWin, URIUtils::FixSlashesAndDups(p, '\\'));
  EXPECT_EQ(param.expectedCanonicalizePathUnix, URIUtils::CanonicalizePath(p, '/'));
  EXPECT_EQ(param.expectedCanonicalizePathWin, URIUtils::CanonicalizePath(p, '\\'));
  EXPECT_EQ(param.expectedAddFileToFolder, URIUtils::AddFileToFolder(p, "NewFile.pdf"));
  EXPECT_EQ(param.expectedGetDirectory, URIUtils::GetDirectory(p));
  EXPECT_EQ(param.expectedCreateZIPArchivePath,
            URIUtils::CreateArchivePath("zip", curl, "/my/archived/path", "passwd").Get());
  EXPECT_EQ(param.expectedCreateRARArchivePath,
            URIUtils::CreateArchivePath("rar", curl, "/my/archived/path", "passwd").Get());
  EXPECT_EQ(param.expectedCreateAPKArchivePath,
            URIUtils::CreateArchivePath("apk", curl, "/my/archived/path", "passwd").Get());
  EXPECT_EQ(param.expectedGetRealPath, URIUtils::GetRealPath(p));
  EXPECT_EQ(param.expectedUpdateUrlEncoding, updatedURLEncoding);

#ifdef GENERATE_JSON_TEST_FILES
  CVariant jsonObj;

  jsonObj["__input"] = p;
  write(jsonObj, "expectedGet", curl.Get());
  write(jsonObj, "expectedGetPort", curl.GetPort());
  write(jsonObj, "expectedGetHostName", curl.GetHostName());
  write(jsonObj, "expectedGetDomain", curl.GetDomain());
  write(jsonObj, "expectedGetUserName", curl.GetUserName());
  write(jsonObj, "expectedGetPassWord", curl.GetPassWord());
  write(jsonObj, "expectedGetFileName", curl.GetFileName());
  write(jsonObj, "expectedGetFileNameUtil", URIUtils::GetFileName(p));
  write(jsonObj, "expectedGetProtocol", curl.GetProtocol());
  write(jsonObj, "expectedGetTranslatedProtocol", curl.GetTranslatedProtocol());
  write(jsonObj, "expectedGetShareName", curl.GetShareName());
  write(jsonObj, "expectedGetOptions", curl.GetOptions());
  write(jsonObj, "expectedGetProtocolOptions", curl.GetProtocolOptions());
  write(jsonObj, "expectedGetFileNameWithoutPath", curl.GetFileNameWithoutPath());
  write(jsonObj, "expectedGetWithoutOptions", curl.GetWithoutOptions());
  write(jsonObj, "expectedGetWithoutUserDetails_redacted", curl.GetWithoutUserDetails(true));
  write(jsonObj, "expectedGetWithoutUserDetails_removed", curl.GetWithoutUserDetails(false));
  write(jsonObj, "expectedGetWithoutFilename", curl.GetWithoutFilename());
  write(jsonObj, "expectedGetRedacted", curl.GetRedacted());
  write(jsonObj, "expectedIsLocal", curl.IsLocal());
  write(jsonObj, "expectedIsLocalHost", curl.IsLocalHost());
  write(jsonObj, "expectedIsFileOnly", CURL::IsFileOnly(p));
  write(jsonObj, "expectedIsFullPath", CURL::IsFullPath(p));
  write(jsonObj, "expectedDecode", CURL::Decode(p));
  write(jsonObj, "expectedDecodeFileName", CURL::Decode(curl.GetFileName()));
  write(jsonObj, "expectedEncode", CURL::Encode(p));
  write(jsonObj, "expectedExtension", URIUtils::GetExtension(p));
  write(jsonObj, "expectedHasExtension", URIUtils::HasExtension(p));
  write(jsonObj, "expectedHasSetExtension", URIUtils::HasExtension(curl, "avi|txt"));
  write(jsonObj, "expectedRemovedExtension", removedExtension);
  write(jsonObj, "expectedReplacedExtension", URIUtils::ReplaceExtension(p, "xyz"));
  write(jsonObj, "expectedFileOrFolder", URIUtils::GetFileOrFolderName(p));
  write(jsonObj, "expectedSplitPath", splitPath);
  write(jsonObj, "expectedSplitFileName", splitFileName);
  write(jsonObj, "expectedHasParentInHostname", URIUtils::HasParentInHostname(curl));
  write(jsonObj, "expectedHasEncodedHostname", URIUtils::HasEncodedHostname(curl));
  write(jsonObj, "expectedHasEncodedFilename", URIUtils::HasEncodedFilename(curl));
  write(jsonObj, "expectedHasParentPath", URIUtils::GetParentPath(p));
  write(jsonObj, "expectedGetParentPath", parentPath);
  write(jsonObj, "expectedGetBasePath", URIUtils::GetBasePath(p));
  write(jsonObj, "expectedGetDiscBase", URIUtils::GetDiscBase(p));
  write(jsonObj, "expectedGetDiscBasePath", URIUtils::GetDiscBasePath(p));
  write(jsonObj, "expectedGetDiscUnderlyingFile", URIUtils::GetDiscUnderlyingFile(curl));
  write(jsonObj, "expectedGetBlurayFile", URIUtils::GetDiscFile(p));
  write(jsonObj, "expectedGetBlurayRootPath", URIUtils::GetBlurayRootPath(p));
  write(jsonObj, "expectedGetBlurayTitlesPath", URIUtils::GetBlurayTitlesPath(p));
  write(jsonObj, "expectedGetBlurayEpisodePath", URIUtils::GetBlurayEpisodePath(p, 1, 2));
  write(jsonObj, "expectedGetBlurayAllEpisodesPath", URIUtils::GetBlurayAllEpisodesPath(p));
  write(jsonObj, "expectedGetBlurayPlaylistPath", URIUtils::GetBlurayPlaylistPath(p));
  write(jsonObj, "expectedIsMusicDBProtocol", URIUtils::IsProtocol(p, "musicdb"));
  write(jsonObj, "expectedIsUDFProtocol", URIUtils::IsProtocol(p, "udf"));
  write(jsonObj, "expectedIsRemote", URIUtils::IsRemote(p));
  write(jsonObj, "expectedIsOnDVD", URIUtils::IsOnDVD(p));
  write(jsonObj, "expectedIsOnLAN", URIUtils::IsOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  write(jsonObj, "expectedIsHostOnLAN", URIUtils::IsHostOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  write(jsonObj, "expectedIsMultiPath", URIUtils::IsMultiPath(p));
  write(jsonObj, "expectedIsHD", URIUtils::IsHD(p));
  write(jsonObj, "expectedIsStack", URIUtils::IsStack(p));
  write(jsonObj, "expectedIsFavourite", URIUtils::IsFavourite(p));
  write(jsonObj, "expectedIsRAR", URIUtils::IsRAR(p));
  write(jsonObj, "expectedIsInArchive", URIUtils::IsInArchive(p));
  write(jsonObj, "expectedIsInAPK", URIUtils::IsInAPK(p));
  write(jsonObj, "expectedIsInZIP", URIUtils::IsInZIP(p));
  write(jsonObj, "expectedIsInRAR", URIUtils::IsInRAR(p));
  write(jsonObj, "expectedIsAPK", URIUtils::IsAPK(p));
  write(jsonObj, "expectedIsZIP", URIUtils::IsZIP(p));
  write(jsonObj, "expectedIsArchive", URIUtils::IsArchive(p));
  write(jsonObj, "expectedIsDiscImage", URIUtils::IsDiscImage(p));
  write(jsonObj, "expectedIsDiscImageStack", URIUtils::IsDiscImageStack(p));
  write(jsonObj, "expectedIsSpecial", URIUtils::IsSpecial(p));
  write(jsonObj, "expectedIsPlugin", URIUtils::IsPlugin(p));
  write(jsonObj, "expectedIsScript", URIUtils::IsScript(p));
  write(jsonObj, "expectedIsAddonsPath", URIUtils::IsAddonsPath(p));
  write(jsonObj, "expectedIsSourcesPath", URIUtils::IsSourcesPath(p));
  write(jsonObj, "expectedIsCDDA", URIUtils::IsCDDA(p));
  write(jsonObj, "expectedIsISO9660", URIUtils::IsISO9660(p));
  write(jsonObj, "expectedIsSmb", URIUtils::IsSmb(p));
  write(jsonObj, "expectedIsURL", URIUtils::IsURL(p));
  write(jsonObj, "expectedIsFTP", URIUtils::IsFTP(p));
  write(jsonObj, "expectedIsHTTP", URIUtils::IsHTTP(p));
  write(jsonObj, "expectedIsUDP", URIUtils::IsUDP(p));
  write(jsonObj, "expectedIsTCP", URIUtils::IsTCP(p));
  write(jsonObj, "expectedIsPVR", URIUtils::IsPVR(p));
  write(jsonObj, "expectedIsPVRChannel", URIUtils::IsPVRChannel(p));
  write(jsonObj, "expectedIsPVRRadioChannel", URIUtils::IsPVRRadioChannel(p));
  write(jsonObj, "expectedIsPVRChannelGroup", URIUtils::IsPVRChannelGroup(p));
  write(jsonObj, "expectedIsPVRGuideItem", URIUtils::IsPVRGuideItem(p));
  write(jsonObj, "expectedIsDAV", URIUtils::IsDAV(p));
  write(jsonObj, "expectedIsInternetStream", URIUtils::IsInternetStream(p, false));
  write(jsonObj, "expectedIsInternetStreamStrict", URIUtils::IsInternetStream(p, true));
  write(jsonObj, "expectedIsStreamedFilesystem", URIUtils::IsStreamedFilesystem(p));
  write(jsonObj, "expectedIsNetworkFilesystem", URIUtils::IsNetworkFilesystem(p));
  write(jsonObj, "expectedIsUPnP", URIUtils::IsUPnP(p));
  write(jsonObj, "expectedIsLiveTV", URIUtils::IsLiveTV(p));
  write(jsonObj, "expectedIsPVRRecording", URIUtils::IsPVRRecording(p));
  write(jsonObj, "expectedIsPVRRecordingFileOrFolder", URIUtils::IsPVRRecordingFileOrFolder(p));
  write(jsonObj, "expectedIsPVRTVRecordingFileOrFolder", URIUtils::IsPVRTVRecordingFileOrFolder(p));
  write(jsonObj, "expectedIsPVRRadioRecordingFileOrFolder",
        URIUtils::IsPVRRadioRecordingFileOrFolder(p));
  write(jsonObj, "expectedIsMusicDb", URIUtils::IsMusicDb(p));
  write(jsonObj, "expectedIsNfs", URIUtils::IsNfs(p));
  write(jsonObj, "expectedIsVideoDb", URIUtils::IsVideoDb(p));
  write(jsonObj, "expectedIsBlurayPath", URIUtils::IsBlurayPath(p));
  write(jsonObj, "expectedIsOpticalMediaFile", URIUtils::IsOpticalMediaFile(p));
  write(jsonObj, "expectedIsBDFile", URIUtils::IsBDFile(p));
  write(jsonObj, "expectedIsDVDFile", URIUtils::IsDVDFile(p));
  write(jsonObj, "expectedIsAndroidApp", URIUtils::IsAndroidApp(p));
  write(jsonObj, "expectedIsLibraryFolder", URIUtils::IsLibraryFolder(p));
  write(jsonObj, "expectedIsLibraryContent", URIUtils::IsLibraryContent(p));
  write(jsonObj, "expectedIsDOSPath", URIUtils::IsDOSPath(p));
  write(jsonObj, "expectedAppendSlash", URIUtils::AppendSlash(p));
  write(jsonObj, "expectedHasSlashAtEnd", URIUtils::HasSlashAtEnd(p, false));
  write(jsonObj, "expectedHasSlashAtEndURL", URIUtils::HasSlashAtEnd(p, true));
  write(jsonObj, "expectedRemovedEndSlash", removedEndSlash);
  write(jsonObj, "expectedIsFixSlashesAndDupsUnix", URIUtils::FixSlashesAndDups(p, '/'));
  write(jsonObj, "expectedIsFixSlashesAndDupsWin", URIUtils::FixSlashesAndDups(p, '\\'));
  write(jsonObj, "expectedCanonicalizePathUnix", URIUtils::CanonicalizePath(p, '/'));
  write(jsonObj, "expectedCanonicalizePathWin", URIUtils::CanonicalizePath(p, '\\'));
  write(jsonObj, "expectedAddFileToFolder", URIUtils::AddFileToFolder(p, "NewFile.pdf"));
  write(jsonObj, "expectedGetDirectory", URIUtils::GetDirectory(p));
  write(jsonObj, "expectedCreateZIPArchivePath",
        URIUtils::CreateArchivePath("zip", curl, "/my/archived/path", "passwd").Get());
  write(jsonObj, "expectedCreateRARArchivePath",
        URIUtils::CreateArchivePath("rar", curl, "/my/archived/path", "passwd").Get());
  write(jsonObj, "expectedCreateAPKArchivePath",
        URIUtils::CreateArchivePath("apk", curl, "/my/archived/path", "passwd").Get());
  write(jsonObj, "expectedGetRealPath", URIUtils::GetRealPath(p));
  write(jsonObj, "expectedUpdateUrlEncoding", updatedURLEncoding);

  std::string jsonString;
  CJSONVariantWriter::Write(jsonObj, jsonString, false);

  auto jsonFile = std::ofstream(filename, std::ios_base::out | std::ios_base::trunc);
  jsonFile << jsonString;
#endif // GENERATE_JSON_TEST_FILES
}

} // namespace

class TestURLParseDetails : public testing::Test, public testing::WithParamInterface<int>
{
};

TEST_P(TestURLParseDetails, RunTest)
{
  std::string filename = "xbmc/utils/test/testdata/";
  filename.append(std::to_string(GetParam()));
  filename.append(".json");
  filename = XBMC_REF_FILE_PATH(filename);

  RunParseTest(filename, CreateParamFromJson(filename));
}

// The first test is 1.json
INSTANTIATE_TEST_SUITE_P(TestURLParse,
                         TestURLParseDetails,
                         testing::Range(1, LAST_JSON_FILE_NUMBER + 1),
                         [](const testing::TestParamInfo<int>& info)
                         { return std::to_string(info.param) + "_json"; });
