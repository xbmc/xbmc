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

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

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

std::ostream& operator<<(std::ostream& os, const TestURLParseDetailsData& rhs)
{
  return os << "(Input: " << rhs.input << ")";
}

class TestURLParseDetails : public Test, public WithParamInterface<TestURLParseDetailsData>
{
};

/*
#include <fstream>
void write(std::ostream& f, const std::string& key, const int& value)
{
	if (value != 0)
	  f << "  ." << key << " = " << value << ",\n";
}

void write(std::ostream& f, const std::string& key, const bool& value)
{
	if (value)
	  f << "  ." << key << " = " << (value ? "true":"false") << ",\n";
}

void write_escape(std::ostream& f, const std::string& value)
{
  for (const char ch: value)
  {
  	if (ch == '\\')
  		f << ch;
  	f << ch;
  }
}

void write(std::ostream& f, const std::string& key, const std::string& value)
{
	if (value != "")
	{
	  f << "  ." << key << " = \"" ;
		write_escape(f, value);
	  f << "\",\n";
	}
}
*/

TEST_P(TestURLParseDetails, ParseURLResults)
{
  auto& param = GetParam();
  const auto& p = param.input;
  const CURL curl(param.input);

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
	auto f = std::ofstream("foo.inc", std::ios_base::out | std::ios_base::app);
	f << "{\n";
	f << "  .input" << " = \"";
	write_escape(f, p);
	f << "\",\n";
  write(f, "expectedGet", curl.Get());
  write(f, "expectedGetPort", curl.GetPort());
  write(f, "expectedGetHostName", curl.GetHostName());
  write(f, "expectedGetDomain", curl.GetDomain());
  write(f, "expectedGetUserName", curl.GetUserName());
  write(f, "expectedGetPassWord", curl.GetPassWord());
  write(f, "expectedGetFileName", curl.GetFileName());
  write(f, "expectedGetFileNameUtil", URIUtils::GetFileName(p));
  write(f, "expectedGetProtocol", curl.GetProtocol());
  write(f, "expectedGetTranslatedProtocol", curl.GetTranslatedProtocol());
  write(f, "expectedGetShareName", curl.GetShareName());
  write(f, "expectedGetOptions", curl.GetOptions());
  write(f, "expectedGetProtocolOptions", curl.GetProtocolOptions());
  write(f, "expectedGetFileNameWithoutPath", curl.GetFileNameWithoutPath());
  write(f, "expectedGetWithoutOptions", curl.GetWithoutOptions());
  write(f, "expectedGetWithoutUserDetails_redacted", curl.GetWithoutUserDetails(true));
  write(f, "expectedGetWithoutUserDetails_removed", curl.GetWithoutUserDetails(false));
  write(f, "expectedGetWithoutFilename", curl.GetWithoutFilename());
  write(f, "expectedGetRedacted", curl.GetRedacted());
  write(f, "expectedIsLocal", curl.IsLocal());
  write(f, "expectedIsLocalHost", curl.IsLocalHost());
  write(f, "expectedIsFileOnly", CURL::IsFileOnly(p));
  write(f, "expectedIsFullPath", CURL::IsFullPath(p));
  write(f, "expectedDecode", CURL::Decode(p));
  write(f, "expectedDecodeFileName", CURL::Decode(curl.GetFileName()));
  write(f, "expectedEncode", CURL::Encode(p));
  write(f, "expectedExtension", URIUtils::GetExtension(p));
  write(f, "expectedHasExtension", URIUtils::HasExtension(p));
  write(f, "expectedHasSetExtension", URIUtils::HasExtension(curl, "avi|txt"));
  write(f, "expectedRemovedExtension", removedExtension);
  write(f, "expectedReplacedExtension", URIUtils::ReplaceExtension(p, "xyz"));
  write(f, "expectedFileOrFolder", URIUtils::GetFileOrFolderName(p));
  write(f, "expectedSplitPath", splitPath);
  write(f, "expectedSplitFileName", splitFileName);
  write(f, "expectedHasParentInHostname", URIUtils::HasParentInHostname(curl));
  write(f, "expectedHasEncodedHostname", URIUtils::HasEncodedHostname(curl));
  write(f, "expectedHasEncodedFilename", URIUtils::HasEncodedFilename(curl));
  write(f, "expectedHasParentPath", URIUtils::GetParentPath(p));
  write(f, "expectedGetParentPath", parentPath);
  write(f, "expectedGetBasePath", URIUtils::GetBasePath(p));
  write(f, "expectedGetDiscBase", URIUtils::GetDiscBase(p));
  write(f, "expectedGetDiscBasePath", URIUtils::GetDiscBasePath(p));
  write(f, "expectedGetDiscUnderlyingFile", URIUtils::GetDiscUnderlyingFile(curl));
  write(f, "expectedGetBlurayFile", URIUtils::GetDiscFile(p));
  write(f, "expectedGetBlurayRootPath", URIUtils::GetBlurayRootPath(p));
  write(f, "expectedGetBlurayTitlesPath", URIUtils::GetBlurayTitlesPath(p));
  write(f, "expectedGetBlurayEpisodePath", URIUtils::GetBlurayEpisodePath(p, 1, 2));
  write(f, "expectedGetBlurayAllEpisodesPath", URIUtils::GetBlurayAllEpisodesPath(p));
  write(f, "expectedGetBlurayPlaylistPath", URIUtils::GetBlurayPlaylistPath(p));
  write(f, "expectedIsMusicDBProtocol", URIUtils::IsProtocol(p, "musicdb"));
  write(f, "expectedIsUDFProtocol", URIUtils::IsProtocol(p, "udf"));
  write(f, "expectedIsRemote", URIUtils::IsRemote(p));
  write(f, "expectedIsOnDVD", URIUtils::IsOnDVD(p));
  write(f, "expectedIsOnLAN", URIUtils::IsOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  write(f, "expectedIsHostOnLAN", URIUtils::IsHostOnLAN(p, LanCheckMode::ANY_PRIVATE_SUBNET));
  write(f, "expectedIsMultiPath", URIUtils::IsMultiPath(p));
  write(f, "expectedIsHD", URIUtils::IsHD(p));
  write(f, "expectedIsStack", URIUtils::IsStack(p));
  write(f, "expectedIsFavourite", URIUtils::IsFavourite(p));
  write(f, "expectedIsRAR", URIUtils::IsRAR(p));
  write(f, "expectedIsInArchive", URIUtils::IsInArchive(p));
  write(f, "expectedIsInAPK", URIUtils::IsInAPK(p));
  write(f, "expectedIsInZIP", URIUtils::IsInZIP(p));
  write(f, "expectedIsInRAR", URIUtils::IsInRAR(p));
  write(f, "expectedIsAPK", URIUtils::IsAPK(p));
  write(f, "expectedIsZIP", URIUtils::IsZIP(p));
  write(f, "expectedIsArchive", URIUtils::IsArchive(p));
  write(f, "expectedIsDiscImage", URIUtils::IsDiscImage(p));
  write(f, "expectedIsDiscImageStack", URIUtils::IsDiscImageStack(p));
  write(f, "expectedIsSpecial", URIUtils::IsSpecial(p));
  write(f, "expectedIsPlugin", URIUtils::IsPlugin(p));
  write(f, "expectedIsScript", URIUtils::IsScript(p));
  write(f, "expectedIsAddonsPath", URIUtils::IsAddonsPath(p));
  write(f, "expectedIsSourcesPath", URIUtils::IsSourcesPath(p));
  write(f, "expectedIsCDDA", URIUtils::IsCDDA(p));
  write(f, "expectedIsISO9660", URIUtils::IsISO9660(p));
  write(f, "expectedIsSmb", URIUtils::IsSmb(p));
  write(f, "expectedIsURL", URIUtils::IsURL(p));
  write(f, "expectedIsFTP", URIUtils::IsFTP(p));
  write(f, "expectedIsHTTP", URIUtils::IsHTTP(p));
  write(f, "expectedIsUDP", URIUtils::IsUDP(p));
  write(f, "expectedIsTCP", URIUtils::IsTCP(p));
  write(f, "expectedIsPVR", URIUtils::IsPVR(p));
  write(f, "expectedIsPVRChannel", URIUtils::IsPVRChannel(p));
  write(f, "expectedIsPVRRadioChannel", URIUtils::IsPVRRadioChannel(p));
  write(f, "expectedIsPVRChannelGroup", URIUtils::IsPVRChannelGroup(p));
  write(f, "expectedIsPVRGuideItem", URIUtils::IsPVRGuideItem(p));
  write(f, "expectedIsDAV", URIUtils::IsDAV(p));
  write(f, "expectedIsInternetStream", URIUtils::IsInternetStream(p, false));
  write(f, "expectedIsInternetStreamStrict", URIUtils::IsInternetStream(p, true));
  write(f, "expectedIsStreamedFilesystem", URIUtils::IsStreamedFilesystem(p));
  write(f, "expectedIsNetworkFilesystem", URIUtils::IsNetworkFilesystem(p));
  write(f, "expectedIsUPnP", URIUtils::IsUPnP(p));
  write(f, "expectedIsLiveTV", URIUtils::IsLiveTV(p));
  write(f, "expectedIsPVRRecording", URIUtils::IsPVRRecording(p));
  write(f, "expectedIsPVRRecordingFileOrFolder", URIUtils::IsPVRRecordingFileOrFolder(p));
  write(f, "expectedIsPVRTVRecordingFileOrFolder", URIUtils::IsPVRTVRecordingFileOrFolder(p));
  write(f, "expectedIsPVRRadioRecordingFileOrFolder", URIUtils::IsPVRRadioRecordingFileOrFolder(p));
  write(f, "expectedIsMusicDb", URIUtils::IsMusicDb(p));
  write(f, "expectedIsNfs", URIUtils::IsNfs(p));
  write(f, "expectedIsVideoDb", URIUtils::IsVideoDb(p));
  write(f, "expectedIsBlurayPath", URIUtils::IsBlurayPath(p));
  write(f, "expectedIsOpticalMediaFile", URIUtils::IsOpticalMediaFile(p));
  write(f, "expectedIsBDFile", URIUtils::IsBDFile(p));
  write(f, "expectedIsDVDFile", URIUtils::IsDVDFile(p));
  write(f, "expectedIsAndroidApp", URIUtils::IsAndroidApp(p));
  write(f, "expectedIsLibraryFolder", URIUtils::IsLibraryFolder(p));
  write(f, "expectedIsLibraryContent", URIUtils::IsLibraryContent(p));
  write(f, "expectedIsDOSPath", URIUtils::IsDOSPath(p));
  write(f, "expectedAppendSlash", URIUtils::AppendSlash(p));
  write(f, "expectedHasSlashAtEnd", URIUtils::HasSlashAtEnd(p, false));
  write(f, "expectedHasSlashAtEndURL", URIUtils::HasSlashAtEnd(p, true));
  write(f, "expectedRemovedEndSlash", removedEndSlash);
  write(f, "expectedIsFixSlashesAndDupsUnix", URIUtils::FixSlashesAndDups(p, '/'));
  write(f, "expectedIsFixSlashesAndDupsWin", URIUtils::FixSlashesAndDups(p, '\\'));
  write(f, "expectedCanonicalizePathUnix", URIUtils::CanonicalizePath(p, '/'));
  write(f, "expectedCanonicalizePathWin", URIUtils::CanonicalizePath(p, '\\'));
  write(f, "expectedAddFileToFolder", URIUtils::AddFileToFolder(p, "NewFile.pdf"));
  write(f, "expectedGetDirectory", URIUtils::GetDirectory(p));
  write(f, "expectedCreateZIPArchivePath", URIUtils::CreateArchivePath("zip", curl, "/my/archived/path", "passwd").Get());
  write(f, "expectedCreateRARArchivePath", URIUtils::CreateArchivePath("rar", curl, "/my/archived/path", "passwd").Get());
  write(f, "expectedCreateAPKArchivePath", URIUtils::CreateArchivePath("apk", curl, "/my/archived/path", "passwd").Get());
  write(f, "expectedGetRealPath", URIUtils::GetRealPath(p));
  write(f, "expectedUpdateUrlEncoding", updatedURLEncoding);
	f << "},\n";
	*/
}

const TestURLParseDetailsData values[] = {
#include "test_case_results.inc"
};

INSTANTIATE_TEST_SUITE_P(URL, TestURLParseDetails, ValuesIn(values));
