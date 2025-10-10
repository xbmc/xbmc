/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
#include <fstream>
#include "test/TestUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

struct TestURLParseDetailsFilename
{
  size_t filename;
};

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

std::ostream& operator<<(std::ostream& os, const TestURLParseDetailsFilename& rhs)
{
  return os << "(Input: " << rhs.filename << ".json)";
}

class TestURLParseDetails : public Test, public WithParamInterface<TestURLParseDetailsFilename>
{
};


namespace
{
  void read(bool& output, const CVariant& json, const char* key)
  {
    if (json.isMember(key))
      output = json[key].asBoolean();
  }
  void read(int& output, const CVariant& json, const char* key)
  {
    if (json.isMember(key))
      output = json[key].asInteger();
  }
  void read(std::string& output, const CVariant& json, const char* key)
  {
    if (json.isMember(key))
      output = json[key].asString();
  }

  TestURLParseDetailsData CreateParamFromJson(size_t filename)
  {
    CVariant json;
    TestURLParseDetailsData param;
  
    std::ostringstream oss;
    oss << "xbmc/utils/test/testdata/" << filename << ".json";
  
    std::ifstream reader(XBMC_REF_FILE_PATH(oss.str()));
    std::stringstream buffer;
    buffer << reader.rdbuf();
    std::string inputJson = buffer.str();
    reader.close();
  
    if (CJSONVariantParser::Parse(inputJson, json) && !json.isNull())
    {
      read(param.input, json, "input");
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
      read(param.expectedGetWithoutUserDetails_redacted, json, "expectedGetWithoutUserDetails_redacted");
      read(param.expectedGetWithoutUserDetails_removed, json, "expectedGetWithoutUserDetails_removed");
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
      read(param.expectedIsPVRRadioRecordingFileOrFolder, json, "expectedIsPVRRadioRecordingFileOrFolder");
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
}

TEST_P(TestURLParseDetails, ParseURLResults)
{
  auto param = CreateParamFromJson(GetParam().filename);
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

  /*
  { // output json
    static int test_index = 0;
    std::ostringstream oss;
    oss << "foo.json/" << ++test_index << ".json";
    auto f = std::ofstream(oss.str(), std::ios_base::out | std::ios_base::app);

    std::string jsonString;
    CVariant jsonObj;

    jsonObj["expectedGet"] = curl.Get();
    jsonObj["expectedGetPort"] = curl.GetPort();
    jsonObj["expectedGetHostName"] = curl.GetHostName();
    jsonObj["expectedGetDomain"] = curl.GetDomain();
    jsonObj["expectedGetUserName"] = curl.GetUserName();
    jsonObj["expectedGetPassWord"] = curl.GetPassWord();
    jsonObj["expectedGetFileName"] = curl.GetFileName();
    jsonObj["expectedGetFileNameUtil"] = URIUtils::GetFileName(p);
    jsonObj["expectedGetProtocol"] = curl.GetProtocol();
    jsonObj["expectedGetTranslatedProtocol"] = curl.GetTranslatedProtocol();
    jsonObj["expectedGetShareName"] = curl.GetShareName();
    jsonObj["expectedGetOptions"] = curl.GetOptions();
    jsonObj["expectedGetProtocolOptions"] = curl.GetProtocolOptions();
    jsonObj["expectedGetFileNameWithoutPath"] = curl.GetFileNameWithoutPath();
    jsonObj["expectedGetWithoutOptions"] = curl.GetWithoutOptions();
    jsonObj["expectedGetWithoutUserDetails_redacted"] = curl.GetWithoutUserDetails(true);
    jsonObj["expectedGetWithoutUserDetails_removed"] = curl.GetWithoutUserDetails(false);
    jsonObj["expectedGetWithoutFilename"] = curl.GetWithoutFilename();
    jsonObj["expectedGetRedacted"] = curl.GetRedacted();
    jsonObj["expectedIsLocal"] = curl.IsLocal();
    jsonObj["expectedIsLocalHost"] = curl.IsLocalHost();
    jsonObj["expectedIsFileOnly"] = CURL::IsFileOnly(p);
    jsonObj["expectedIsFullPath"] = CURL::IsFullPath(p);
    jsonObj["expectedDecode"] = CURL::Decode(p);
    jsonObj["expectedDecodeFileName"] = CURL::Decode(curl.GetFileName());
    jsonObj["expectedEncode"] = CURL::Encode(p);
    jsonObj["expectedExtension"] = URIUtils::GetExtension(p);
    jsonObj["expectedHasExtension"] = URIUtils::HasExtension(p);
    jsonObj["expectedHasSetExtension"] = URIUtils::HasExtension(curl, "avi|txt");
    jsonObj["expectedRemovedExtension"] = removedExtension;
    jsonObj["expectedReplacedExtension"] = URIUtils::ReplaceExtension(p, "xyz");
    jsonObj["expectedFileOrFolder"] = URIUtils::GetFileOrFolderName(p);
    jsonObj["expectedSplitPath"] = splitPath;
    jsonObj["expectedSplitFileName"] = splitFileName;
    jsonObj["expectedHasParentInHostname"] = URIUtils::HasParentInHostname(curl);
    jsonObj["expectedHasEncodedHostname"] = URIUtils::HasEncodedHostname(curl);
    jsonObj["expectedHasEncodedFilename"] = URIUtils::HasEncodedFilename(curl);
    jsonObj["expectedHasParentPath"] = URIUtils::GetParentPath(p);
    jsonObj["expectedGetParentPath"] = parentPath;
    jsonObj["expectedGetBasePath"] = URIUtils::GetBasePath(p);
    jsonObj["expectedGetDiscBase"] = URIUtils::GetDiscBase(p);
    jsonObj["expectedGetDiscBasePath"] = URIUtils::GetDiscBasePath(p);
    jsonObj["expectedGetDiscUnderlyingFile"] = URIUtils::GetDiscUnderlyingFile(curl);
    jsonObj["expectedGetBlurayFile"] = URIUtils::GetDiscFile(p);
    jsonObj["expectedGetBlurayRootPath"] = URIUtils::GetBlurayRootPath(p);
    jsonObj["expectedGetBlurayTitlesPath"] = URIUtils::GetBlurayTitlesPath(p);
    jsonObj["expectedGetBlurayEpisodePath"] = URIUtils::GetBlurayEpisodePath(p, 1, 2);
    jsonObj["expectedGetBlurayAllEpisodesPath"] = URIUtils::GetBlurayAllEpisodesPath(p);
    jsonObj["expectedGetBlurayPlaylistPath"] = URIUtils::GetBlurayPlaylistPath(p);
    jsonObj["expectedIsMusicDBProtocol"] = URIUtils::IsProtocol(p, "musicdb");
    jsonObj["expectedIsUDFProtocol"] = URIUtils::IsProtocol(p, "udf");
    jsonObj["expectedIsRemote"] = URIUtils::IsRemote(p);
    jsonObj["expectedIsOnDVD"] = URIUtils::IsOnDVD(p);
    jsonObj["expectedIsOnLAN"] = URIUtils::IsOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET);
    jsonObj["expectedIsHostOnLAN"] = URIUtils::IsHostOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET);
    jsonObj["expectedIsMultiPath"] = URIUtils::IsMultiPath(p);
    jsonObj["expectedIsHD"] = URIUtils::IsHD(p);
    jsonObj["expectedIsStack"] = URIUtils::IsStack(p);
    jsonObj["expectedIsFavourite"] = URIUtils::IsFavourite(p);
    jsonObj["expectedIsRAR"] = URIUtils::IsRAR(p);
    jsonObj["expectedIsInArchive"] = URIUtils::IsInArchive(p);
    jsonObj["expectedIsInAPK"] = URIUtils::IsInAPK(p);
    jsonObj["expectedIsInZIP"] = URIUtils::IsInZIP(p);
    jsonObj["expectedIsInRAR"] = URIUtils::IsInRAR(p);
    jsonObj["expectedIsAPK"] = URIUtils::IsAPK(p);
    jsonObj["expectedIsZIP"] = URIUtils::IsZIP(p);
    jsonObj["expectedIsArchive"] = URIUtils::IsArchive(p);
    jsonObj["expectedIsDiscImage"] = URIUtils::IsDiscImage(p);
    jsonObj["expectedIsDiscImageStack"] = URIUtils::IsDiscImageStack(p);
    jsonObj["expectedIsSpecial"] = URIUtils::IsSpecial(p);
    jsonObj["expectedIsPlugin"] = URIUtils::IsPlugin(p);
    jsonObj["expectedIsScript"] = URIUtils::IsScript(p);
    jsonObj["expectedIsAddonsPath"] = URIUtils::IsAddonsPath(p);
    jsonObj["expectedIsSourcesPath"] = URIUtils::IsSourcesPath(p);
    jsonObj["expectedIsCDDA"] = URIUtils::IsCDDA(p);
    jsonObj["expectedIsISO9660"] = URIUtils::IsISO9660(p);
    jsonObj["expectedIsSmb"] = URIUtils::IsSmb(p);
    jsonObj["expectedIsURL"] = URIUtils::IsURL(p);
    jsonObj["expectedIsFTP"] = URIUtils::IsFTP(p);
    jsonObj["expectedIsHTTP"] = URIUtils::IsHTTP(p);
    jsonObj["expectedIsUDP"] = URIUtils::IsUDP(p);
    jsonObj["expectedIsTCP"] = URIUtils::IsTCP(p);
    jsonObj["expectedIsPVR"] = URIUtils::IsPVR(p);
    jsonObj["expectedIsPVRChannel"] = URIUtils::IsPVRChannel(p);
    jsonObj["expectedIsPVRRadioChannel"] = URIUtils::IsPVRRadioChannel(p);
    jsonObj["expectedIsPVRChannelGroup"] = URIUtils::IsPVRChannelGroup(p);
    jsonObj["expectedIsPVRGuideItem"] = URIUtils::IsPVRGuideItem(p);
    jsonObj["expectedIsDAV"] = URIUtils::IsDAV(p);
    jsonObj["expectedIsInternetStream"] = URIUtils::IsInternetStream(p, false);
    jsonObj["expectedIsInternetStreamStrict"] = URIUtils::IsInternetStream(p, true);
    jsonObj["expectedIsStreamedFilesystem"] = URIUtils::IsStreamedFilesystem(p);
    jsonObj["expectedIsNetworkFilesystem"] = URIUtils::IsNetworkFilesystem(p);
    jsonObj["expectedIsUPnP"] = URIUtils::IsUPnP(p);
    jsonObj["expectedIsLiveTV"] = URIUtils::IsLiveTV(p);
    jsonObj["expectedIsPVRRecording"] = URIUtils::IsPVRRecording(p);
    jsonObj["expectedIsPVRRecordingFileOrFolder"] = URIUtils::IsPVRRecordingFileOrFolder(p);
    jsonObj["expectedIsPVRTVRecordingFileOrFolder"] = URIUtils::IsPVRTVRecordingFileOrFolder(p);
    jsonObj["expectedIsPVRRadioRecordingFileOrFolder"] = URIUtils::IsPVRRadioRecordingFileOrFolder(p);
    jsonObj["expectedIsMusicDb"] = URIUtils::IsMusicDb(p);
    jsonObj["expectedIsNfs"] = URIUtils::IsNfs(p);
    jsonObj["expectedIsVideoDb"] = URIUtils::IsVideoDb(p);
    jsonObj["expectedIsBlurayPath"] = URIUtils::IsBlurayPath(p);
    jsonObj["expectedIsOpticalMediaFile"] = URIUtils::IsOpticalMediaFile(p);
    jsonObj["expectedIsBDFile"] = URIUtils::IsBDFile(p);
    jsonObj["expectedIsDVDFile"] = URIUtils::IsDVDFile(p);
    jsonObj["expectedIsAndroidApp"] = URIUtils::IsAndroidApp(p);
    jsonObj["expectedIsLibraryFolder"] = URIUtils::IsLibraryFolder(p);
    jsonObj["expectedIsLibraryContent"] = URIUtils::IsLibraryContent(p);
    jsonObj["expectedIsDOSPath"] = URIUtils::IsDOSPath(p);
    jsonObj["expectedAppendSlash"] = URIUtils::AppendSlash(p);
    jsonObj["expectedHasSlashAtEnd"] = URIUtils::HasSlashAtEnd(p, false);
    jsonObj["expectedHasSlashAtEndURL"] = URIUtils::HasSlashAtEnd(p, true);
    jsonObj["expectedRemovedEndSlash"] = removedEndSlash;
    jsonObj["expectedIsFixSlashesAndDupsUnix"] = URIUtils::FixSlashesAndDups(p, '/');
    jsonObj["expectedIsFixSlashesAndDupsWin"] = URIUtils::FixSlashesAndDups(p, '\\');
    jsonObj["expectedCanonicalizePathUnix"] = URIUtils::CanonicalizePath(p, '/');
    jsonObj["expectedCanonicalizePathWin"] = URIUtils::CanonicalizePath(p, '\\');
    jsonObj["expectedAddFileToFolder"] = URIUtils::AddFileToFolder(p, "NewFile.pdf");
    jsonObj["expectedGetDirectory"] = URIUtils::GetDirectory(p);
    jsonObj["expectedCreateZIPArchivePath"] = URIUtils::CreateArchivePath("zip", curl, "/my/archived/path", "passwd").Get();
    jsonObj["expectedCreateRARArchivePath"] = URIUtils::CreateArchivePath("rar", curl, "/my/archived/path", "passwd").Get();
    jsonObj["expectedCreateAPKArchivePath"] = URIUtils::CreateArchivePath("apk", curl, "/my/archived/path", "passwd").Get();
    jsonObj["expectedGetRealPath"] = URIUtils::GetRealPath(p);
    jsonObj["expectedUpdateUrlEncoding"] = updatedURLEncoding;

    CJSONVariantWriter::Write(jsonObj, jsonString, false);
    f << jsonString;
  } // output json
  */
}

constexpr TestURLParseDetailsFilename values[] = {
#include "test_case_results.inc"
};

INSTANTIATE_TEST_SUITE_P(URL, TestURLParseDetails, ValuesIn(values));
