/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/FileFactory.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{

struct Predicate
{
  std::string_view name;
  bool (*isPath)(const std::string& path);
  // The same predicate as a CURL member, where one exists.
  bool (*isURL)(const CURL& url){nullptr};
  // The answer depends on the host: drive types, the network or special:// translation.
  bool hostDependent{false};
};

// clang-format off
const std::array Predicates{
    Predicate{"HasEncodedFilename", [](const std::string& path) { return URIUtils::HasEncodedFilename(CURL{path}); }, [](const CURL& url) { return url.HasEncodedFilename(); }},
    Predicate{"HasParentInHostname", [](const std::string& path) { return URIUtils::HasParentInHostname(CURL{path}); }, [](const CURL& url) { return url.HasParentInHostname(); }},
    Predicate{"IsAPK", URIUtils::IsAPK, [](const CURL& url) { return url.IsAPK(); }},
    Predicate{"IsAddonsPath", URIUtils::IsAddonsPath, [](const CURL& url) { return url.IsAddonsPath(); }},
    Predicate{"IsAndroidApp", URIUtils::IsAndroidApp, [](const CURL& url) { return url.IsAndroidApp(); }},
    Predicate{"IsArchive", [](const std::string& path) { return URIUtils::IsArchive(path); }, [](const CURL& url) { return url.IsArchive(); }},
    Predicate{"IsBDFile", URIUtils::IsBDFile, [](const CURL& url) { return url.IsBDFile(); }},
    Predicate{"IsBlurayPath", URIUtils::IsBlurayPath, [](const CURL& url) { return url.IsBlurayPath(); }},
    Predicate{"IsCDDA", URIUtils::IsCDDA, [](const CURL& url) { return url.IsCDDA(); }},
    Predicate{"IsDAV", URIUtils::IsDAV},
    Predicate{"IsDOSPath", URIUtils::IsDOSPath},
    Predicate{"IsDVD", URIUtils::IsDVD, nullptr, true},
    Predicate{"IsDVDFile", URIUtils::IsDVDFile, [](const CURL& url) { return url.IsDVDFile(); }},
    Predicate{"IsDiscImage", URIUtils::IsDiscImage, [](const CURL& url) { return url.IsDiscImage(); }},
    Predicate{"IsFTP", URIUtils::IsFTP},
    Predicate{"IsFavourite", URIUtils::IsFavourite, [](const CURL& url) { return url.IsFavourite(); }},
    Predicate{"IsHD", URIUtils::IsHD},
    Predicate{"IsHTTP", [](const std::string& path) { return URIUtils::IsHTTP(path); }},
    Predicate{"IsISO9660", URIUtils::IsISO9660, [](const CURL& url) { return url.IsISO9660(); }},
    Predicate{"IsInAPK", URIUtils::IsInAPK},
    Predicate{"IsInArchive", URIUtils::IsInArchive},
    Predicate{"IsInRAR", URIUtils::IsInRAR},
    Predicate{"IsInZIP", URIUtils::IsInZIP},
    Predicate{"IsInternetStream", [](const std::string& path) { return URIUtils::IsInternetStream(path); }},
    Predicate{"IsLibraryContent", URIUtils::IsLibraryContent, [](const CURL& url) { return url.IsLibraryContent(); }},
    Predicate{"IsLibraryFolder", URIUtils::IsLibraryFolder, [](const CURL& url) { return url.IsLibraryFolder(); }},
    Predicate{"IsLiveTV", URIUtils::IsLiveTV},
    Predicate{"IsLocalOrLAN", URIUtils::IsLocalOrLAN, nullptr, true},
    Predicate{"IsMultiPath", URIUtils::IsMultiPath, [](const CURL& url) { return url.IsMultiPath(); }},
    Predicate{"IsMusicDb", URIUtils::IsMusicDb, [](const CURL& url) { return url.IsMusicDb(); }},
    Predicate{"IsNetworkFilesystem", URIUtils::IsNetworkFilesystem},
    Predicate{"IsNfs", URIUtils::IsNfs},
    Predicate{"IsOnDVD", URIUtils::IsOnDVD, nullptr, true},
    Predicate{"IsOnLAN", [](const std::string& path) { return URIUtils::IsOnLAN(path); }, nullptr, true},
    Predicate{"IsPVR", URIUtils::IsPVR},
    Predicate{"IsPVRChannel", URIUtils::IsPVRChannel},
    Predicate{"IsPVRChannelGroup", URIUtils::IsPVRChannelGroup},
    Predicate{"IsPVRGuideItem", URIUtils::IsPVRGuideItem},
    Predicate{"IsPVRRadioChannel", URIUtils::IsPVRRadioChannel},
    Predicate{"IsPVRRadioRecordingFileOrFolder", URIUtils::IsPVRRadioRecordingFileOrFolder},
    Predicate{"IsPVRRecording", URIUtils::IsPVRRecording},
    Predicate{"IsPVRRecordingFileOrFolder", URIUtils::IsPVRRecordingFileOrFolder},
    Predicate{"IsPVRTVRecordingFileOrFolder", URIUtils::IsPVRTVRecordingFileOrFolder},
    Predicate{"IsPlugin", URIUtils::IsPlugin, [](const CURL& url) { return url.IsPlugin(); }},
    Predicate{"IsRAR", URIUtils::IsRAR},
    Predicate{"IsRemote", URIUtils::IsRemote, nullptr, true},
    Predicate{"IsScript", URIUtils::IsScript, [](const CURL& url) { return url.IsScript(); }},
    Predicate{"IsSmb", URIUtils::IsSmb},
    Predicate{"IsSourcesPath", URIUtils::IsSourcesPath, [](const CURL& url) { return url.IsSourcesPath(); }},
    Predicate{"IsSpecial", URIUtils::IsSpecial},
    Predicate{"IsStack", URIUtils::IsStack, [](const CURL& url) { return url.IsStack(); }},
    Predicate{"IsStreamedFilesystem", URIUtils::IsStreamedFilesystem},
    Predicate{"IsTCP", URIUtils::IsTCP},
    Predicate{"IsUDP", URIUtils::IsUDP},
    Predicate{"IsUPnP", URIUtils::IsUPnP, [](const CURL& url) { return url.IsUPnP(); }},
    Predicate{"IsURL", [](const std::string& path) { return URIUtils::IsURL(path); }},
    Predicate{"IsVideoDb", URIUtils::IsVideoDb, [](const CURL& url) { return url.IsVideoDb(); }},
    Predicate{"IsZIP", URIUtils::IsZIP, [](const CURL& url) { return url.IsZIP(); }},
};
// clang-format on

const Predicate* FindPredicate(std::string_view name)
{
  const auto it = std::ranges::find(Predicates, name, &Predicate::name);
  return it == Predicates.end() ? nullptr : &*it;
}

struct PathKind
{
  const char* path;
  // Every other predicate that does not depend on the host answers false.
  std::vector<std::string_view> trueFor;
};

// clang-format off
const PathKind PathKinds[] = {
    // Native paths
    {"/path/thing/file.avi", {"IsHD"}},
    {"C:\\My Videos\\sub dir\\movie name.avi", {"IsHD", "IsDOSPath"}},
    {"\\\\path\\to\\file", {"IsHD", "IsDOSPath"}},
    {"file:///path/to/movie.avi?showinfo=true", {"IsHD", "IsURL"}},
    {"special://home/movies/movie.avi", {"IsSpecial", "IsHD", "IsURL"}},
    // Network filesystems
    {"smb://user:pass@host/share/movie.mkv", {"IsSmb", "IsNetworkFilesystem", "IsURL"}},
    {"nfs://path/to/file", {"IsNfs", "IsNetworkFilesystem", "IsURL"}},
    {"ftp://some.where/foo.avi", {"IsFTP", "IsStreamedFilesystem", "IsNetworkFilesystem", "IsURL"}},
    {"sftp://user:pass@host/movie.mkv", {"IsStreamedFilesystem", "IsNetworkFilesystem", "IsURL"}},
    {"davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3", {"IsDAV", "IsStreamedFilesystem", "IsNetworkFilesystem", "HasEncodedFilename", "IsURL"}},
    {"upnp://some.where/123", {"IsUPnP", "IsStreamedFilesystem", "IsNetworkFilesystem", "IsURL"}},
    // Internet streams
    {"http://www.test.com/path/to/movie.avi", {"IsHTTP", "IsInternetStream", "IsStreamedFilesystem", "IsNetworkFilesystem", "HasEncodedFilename", "IsURL"}},
    {"https://myserver/some%20other%20path/sub%20dir/movie%20name.avi", {"IsHTTP", "IsInternetStream", "IsStreamedFilesystem", "IsNetworkFilesystem", "HasEncodedFilename", "IsURL"}},
    {"shout://host/stream", {"IsInternetStream", "IsStreamedFilesystem", "IsNetworkFilesystem", "HasEncodedFilename", "IsURL"}},
    {"tcp://user:pass@host:789", {"IsTCP", "IsInternetStream", "IsURL"}},
    {"udp://user:pass@host/path/to/file.avi", {"IsUDP", "IsInternetStream", "IsURL"}},
    {"rtp://user:pass@host/path/to/file.avi", {"IsInternetStream", "IsURL"}},
    // Containers
    {"stack:///home/user/bar/foo-cd1.avi , /home/user/bar/foo-cd2.avi", {"IsStack", "IsHD", "IsURL"}},
    {"stack://smb://server/movies/movie_part1.mkv , smb://server/movies/movie_part2.mkv", {"IsStack", "IsSmb", "IsNetworkFilesystem", "IsURL"}},
    {"multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f", {"IsMultiPath", "IsURL"}},
    // Archives
    {"/foo/bar.zip", {"IsZIP", "IsArchive", "IsHD"}},
    {"/foo/bar.rar", {"IsRAR", "IsArchive", "IsHD"}},
    {"/foo/bar.apk", {"IsAPK", "IsArchive", "IsHD"}},
    {"zip://%2fhome%2fuser%2fbar.zip/foo.avi", {"IsInZIP", "IsInArchive", "HasParentInHostname", "IsHD", "IsURL"}},
    {"zip://zip%3a%2f%2f%252fhome%252fuser%252fbar.zip%2fsecondary%2fzipfile.zip/bob.avi", {"IsInZIP", "IsInArchive", "HasParentInHostname", "IsHD", "IsURL"}},
    {"rar://%2fhome%2fuser%2fbar.rar/foo.avi", {"IsInRAR", "IsInArchive", "HasParentInHostname", "IsHD", "IsURL"}},
    {"rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.rar/movie.mkv", {"IsInRAR", "IsInArchive", "HasParentInHostname", "IsSmb", "IsNetworkFilesystem", "IsURL"}},
    {"archive://%2fhome%2fuser%2fmovie.tar.gz/movie.mkv", {"IsInArchive", "HasParentInHostname", "IsHD", "IsURL"}},
    {"apk://%2fhome%2fuser%2fapp.apk/assets/movie.mkv", {"IsInAPK", "IsInArchive", "HasParentInHostname", "IsHD", "IsURL"}},
    // Discs
    {"/home/user/movies/movie.iso", {"IsDiscImage", "IsHD"}},
    {"/home/user/BDMV/index.bdmv", {"IsBDFile", "IsHD"}},
    {"/home/user/VIDEO_TS/VIDEO_TS.IFO", {"IsDVDFile", "IsHD"}},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", {"IsBlurayPath", "HasParentInHostname", "IsSmb", "IsNetworkFilesystem", "IsURL"}},
    {"udf://%2fhome%2fuser%2fmovie.iso/VIDEO_TS/VIDEO_TS.IFO", {"IsDVDFile", "HasParentInHostname", "IsHD", "IsURL"}},
    {"iso9660://%2fhome%2fuser%2fmovie.iso/VIDEO_TS/VIDEO_TS.IFO", {"IsISO9660", "IsDVDFile", "HasParentInHostname", "IsHD", "IsURL"}},
    {"cdda://local/01.cdda", {"IsCDDA", "IsURL"}},
    // PVR
    {"pvr://channels/tv/groupname@0815/1@pvr.demo_4711.pvr", {"IsPVR", "IsPVRChannel", "IsLiveTV", "IsURL"}},
    {"pvr://channels/radio/groupname@0815/1@pvr.demo_4711.pvr", {"IsPVR", "IsPVRChannel", "IsPVRRadioChannel", "IsLiveTV", "IsURL"}},
    {"pvr://channels/radio/groupname@0815/", {"IsPVR", "IsPVRChannelGroup", "IsURL"}},
    {"pvr://guide/0006/2026-11-21 20:00:00.epg", {"IsPVR", "IsPVRGuideItem", "IsURL"}},
    {"pvr://recordings/tv/active/Movie (2020), TV (Channel), 20261121_200000, 42.pvr", {"IsPVR", "IsPVRRecording", "IsPVRRecordingFileOrFolder", "IsPVRTVRecordingFileOrFolder", "IsURL"}},
    {"pvr://recordings/radio/active/", {"IsPVR", "IsPVRRecordingFileOrFolder", "IsPVRRadioRecordingFileOrFolder", "IsURL"}},
    // Library
    {"videodb://movies/titles/", {"IsVideoDb", "IsLibraryContent", "IsURL"}},
    {"musicdb://genres/", {"IsMusicDb", "IsLibraryContent", "IsURL"}},
    {"library://video/movies/titles.xml/", {"IsLibraryFolder", "IsLibraryContent", "IsURL"}},
    {"special://profile/playlists/video/Action.xsp", {"IsSpecial", "IsLibraryContent", "IsHD", "IsURL"}},
    // Add-ons and virtual sources
    {"plugin://plugin.video.foo/", {"IsPlugin", "IsURL"}},
    {"script://script.foo/", {"IsScript", "IsURL"}},
    {"androidapp://app", {"IsAndroidApp", "IsURL"}},
    {"favourites://1", {"IsFavourite", "IsURL"}},
    {"addons://user/", {"IsAddonsPath", "IsURL"}},
    {"sources://video/", {"IsSourcesPath", "IsURL"}},
};
// clang-format on

struct PredicateExample
{
  std::string_view predicate;
  const char* path;
  bool expected;
};

// clang-format off
const PredicateExample PredicateExamples[] = {
    {"HasEncodedFilename", "dav://", true},
    {"HasEncodedFilename", "davs://", true},
    {"HasEncodedFilename", "rss://", true},
    {"HasEncodedFilename", "shout://", true},
    {"HasParentInHostname", "bluray://", true},
    {"HasParentInHostname", "zip://", true},
    {"IsAddonsPath", "addons://path/to/addons", true},
    {"IsBlurayPath", "bluray://path/to/file", true},
    {"IsBlurayPath", "zip://path/to/file", false},
    {"IsCDDA", "cdda://path/to/cdda", true},
    {"IsDOSPath", "C://path/to/dosfile", true},
    {"IsDVD", "dvd://path/in/video_ts.ifo", true},
#if defined(TARGET_WINDOWS)
    {"IsDVD", "dvd://path/in/file", true},
#else
    {"IsDVD", "iso9660://path/in/video_ts.ifo", true},
    {"IsDVD", "udf://path/in/video_ts.ifo", true},
    {"IsDVD", "dvd://1", true},
#endif
    {"IsFTP", "ftp://path/in/ftp", true},
    {"IsHD", "/path/to/file", true},
    {"IsHD", "file:///path/to/file", true},
    {"IsHD", "special://path/to/file", true},
    {"IsHD", "stack://path/to/file", true},
    {"IsHD", "zip://path/to/file", true},
    {"IsISO9660", "iso9660://path/to/file", true},
    {"IsInternetStream", "http://path/to/file", true},
    {"IsInternetStream", "https://path/to/file", true},
    {"IsLiveTV", "whatever://path/to/file.pvr", true},
    {"IsLocalOrLAN", "/home/user/video.mkv", true},
    {"IsLocalOrLAN", "C:\\Videos\\movie.mp4", true},
    {"IsLocalOrLAN", "file:///home/user/video.mkv", true},
    {"IsLocalOrLAN", "file://localhost/C:/Videos/movie.mp4", true},
    {"IsLocalOrLAN", "resource://plugin.name/video.mkv", true},
    {"IsLocalOrLAN", "stack:///home/user/video_part1.mkv , /home/user/video_part2.mkv", true},
    {"IsLocalOrLAN", "smb://192.168.1.10/share/video.mkv", true},
    {"IsLocalOrLAN", "smb://10.0.0.5/share/video.mkv", true},
    {"IsLocalOrLAN", "smb://172.16.0.1/share/video.mkv", true},
    {"IsLocalOrLAN", "upnp://uuid/item", true},
    {"IsLocalOrLAN", "http://192.168.1.10/video.mkv", false},
    {"IsLocalOrLAN", "https://192.168.1.10/video.mkv", false},
    {"IsLocalOrLAN", "rtmp://stream.example.com/live/video", false},
    {"IsLocalOrLAN", "rtsp://stream.example.com/video", false},
    {"IsLocalOrLAN", "mms://stream.example.com/video", false},
    {"IsLocalOrLAN", "rtp://239.0.0.1/video", false},
    {"IsLocalOrLAN", "stack://http://stream.example.com/part1.mkv , http://stream.example.com/part2.mkv", false},
    {"IsLocalOrLAN", "smb://203.0.113.5/share/video.mkv", false},
    {"IsLocalOrLAN", "nfs://203.0.113.5/export/video.mkv", false},
    {"IsLocalOrLAN", "plugin://plugin.video.example/play/123", false},
    {"IsLocalOrLAN", "videodb://tvshows/titles/1/1/42", false},
    {"IsMultiPath", "multipath://path/to/file", true},
    {"IsMusicDb", "musicdb://path/to/file", true},
    {"IsNfs", "nfs://path/to/file", true},
    {"IsNfs", "stack://nfs://path/to/file", true},
    {"IsOnDVD", "dvd://path/to/file", true},
    {"IsOnDVD", "udf://path/to/file", true},
    {"IsOnDVD", "iso9660://path/to/file", true},
    {"IsOnDVD", "cdda://path/to/file", true},
    {"IsOnLAN", "stack://smb://path/to/file", true},
    {"IsOnLAN", "smb://path/to/file", true},
    {"IsOnLAN", "plugin://path/to/file", false},
    {"IsOnLAN", "upnp://path/to/file", true},
    {"IsPVRRadioChannel", "pvr://channels/radio/groupname@0815/1@pvr.demo_4711.pvr", true},
    {"IsPVRRadioChannel", "pvr://channels/tv/groupname@0815/1@pvr.demo_4711.pvr", false},
    {"IsPVRRadioChannel", "pvr://channels/radio/", false},
    {"IsPVRRadioChannel", "pvr://channels/radio/groupname@0815/", false},
    {"IsPlugin", "plugin://path/to/file", true},
    {"IsRemote", "file:/videos/movie.mkv", false},
    {"IsRemote", "file:movie.mkv", false},
    {"IsRemote", "file://server/videos/movie.mkv", true},
    {"IsRemote", "data:,hello", true},
    {"IsRemote", "urn:example:movie", true},
    {"IsRemote", "magnet:?xt=urn:btih:hash", true},
    {"IsRemote", "unknown:movie", false},
    {"IsRemote", "http://path/to/file", true},
    {"IsRemote", "https://path/to/file", true},
    {"IsRemote", "addons://user/", false},
    {"IsRemote", "sources://video/", false},
    {"IsRemote", "videodb://movies/titles", false},
    {"IsRemote", "musicdb://genres/", false},
    {"IsRemote", "library://video/", false},
    {"IsRemote", "androidapp://app", false},
    {"IsRemote", "plugin://plugin.video.id", false},
    {"IsScript", "script://path/to/file", true},
    {"IsSmb", "smb://path/to/file", true},
    {"IsSmb", "stack://smb://path/to/file", true},
    {"IsSourcesPath", "sources://path/to/sources", true},
    {"IsSpecial", "special://path/to/file", true},
    {"IsSpecial", "stack://special://path/to/file", true},
    {"IsStack", "stack://path/to/file", true},
    {"IsURL", "someprotocol://path/to/file", true},
    {"IsURL", "/path/to/file", false},
    {"IsURL", "/tmp/a://b", false},
    {"IsURL", "data:,http://host/file", false},
    {"IsUPnP", "upnp://path/to/file", true},
    {"IsVideoDb", "videodb://path/to/file", true},
};
// clang-format on

struct ProtocolCase
{
  const char* path;
  const char* protocol;
  bool expected;
};

const ProtocolCase ProtocolCases[] = {
    {"smb://host/share/movie.mkv", "smb", true},
    {"SMB://host/share/movie.mkv", "smb", true},
    {"smbx://host/share/movie.mkv", "smb", false},
    {"smb:host/share/movie.mkv", "smb", false},
    {"stack://smb://host/share/movie.mkv", "smb", false},
    {"/smb://host/share/movie.mkv", "smb", false},
};

struct TranslatedProtocolCase
{
  const char* path;
  const char* translated;
};

const TranslatedProtocolCase TranslatedProtocolCases[] = {
    {"dav://host/music/track.mp3", "http"},
    {"davs://host/music/track.mp3", "https"},
    {"shout://host/stream", "http"},
    {"shouts://host/stream", "https"},
    {"rss://host/feed.xml", "http"},
    {"rsss://host/feed.xml", "https"},
    {"http://host/movie.mkv", "http"},
    {"smb://host/share/movie.mkv", "smb"},
    {"/path/to/movie.mkv", ""},
};

struct FileHandlerCase
{
  const char* path;
  bool hasFileHandler;
};

// An identifier is never a directory; only a scheme with a file backend can be read.
const FileHandlerCase FileHandlerCases[] = {
    {"data:,movie.zip", true},
    {"data://host/movie.zip", true},
    {"urn:example:movie.zip", false},
    {"magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567", false},
};

} // namespace

TEST(TestURIUtilsClassification, PathKinds)
{
  for (const auto& [path, trueFor] : PathKinds)
  {
    SCOPED_TRACE(path);
    for (const std::string_view name : trueFor)
    {
      EXPECT_NE(nullptr, FindPredicate(name)) << name;
    }

    const CURL url{path};
    for (const Predicate& predicate : Predicates)
    {
      if (predicate.hostDependent)
      {
        continue;
      }

      const bool expected{std::ranges::find(trueFor, predicate.name) != trueFor.end()};
      const bool actual{predicate.isPath(path)};
      EXPECT_EQ(expected, actual) << "URIUtils::" << predicate.name;
      if (predicate.isURL)
      {
        EXPECT_EQ(actual, predicate.isURL(url))
            << "URIUtils and CURL disagree on " << predicate.name;
      }
    }
  }
}

TEST(TestURIUtilsClassification, PredicateExamples)
{
  for (const auto& [name, path, expected] : PredicateExamples)
  {
    SCOPED_TRACE(path);
    const Predicate* predicate{FindPredicate(name)};
    ASSERT_NE(nullptr, predicate) << name;
    EXPECT_EQ(expected, predicate->isPath(path)) << name;
  }
}

TEST(TestURIUtilsClassification, IsOnLANFollowsTheFirstMultiPathSource)
{
  const std::vector<std::string> paths{"smb://path/to/file"};
  EXPECT_TRUE(URIUtils::IsOnLAN(CMultiPathDirectory::ConstructMultiPath(paths)));
}

TEST(TestURIUtilsClassification, IdentifierFactoriesRespectSchemeSupport)
{
  for (const auto& [path, hasFileHandler] : FileHandlerCases)
  {
    SCOPED_TRACE(path);
    const CURL url{path};
    const std::unique_ptr<IFile> file{CFileFactory::CreateLoader(url)};
    EXPECT_EQ(hasFileHandler, file != nullptr);
    const std::unique_ptr<IDirectory> directory{CDirectoryFactory::Create(url)};
    EXPECT_EQ(nullptr, directory);
  }
}

TEST(TestURIUtilsClassification, ExplicitUnknownIdentifiersHaveNoFilesystemSemantics)
{
  CURL url;
  ASSERT_TRUE(url.ParseURI("unknown+v1:/dir/movie.zip?x=%2F#part"));
  EXPECT_TRUE(URIUtils::GetFileName(url).empty());
  EXPECT_TRUE(URIUtils::GetExtension(url).empty());
  EXPECT_FALSE(URIUtils::HasExtension(url, ".zip"));
  EXPECT_EQ(nullptr, std::unique_ptr<IFile>{CFileFactory::CreateLoader(url)});
  EXPECT_EQ(nullptr, std::unique_ptr<IDirectory>{CDirectoryFactory::Create(url)});
}

TEST(TestURIUtilsClassification, IsProtocol)
{
  for (const auto& [path, protocol, expected] : ProtocolCases)
  {
    EXPECT_EQ(expected, URIUtils::IsProtocol(path, protocol)) << path;
  }
}

TEST(TestURIUtilsClassification, GetTranslatedProtocol)
{
  for (const auto& [path, translated] : TranslatedProtocolCases)
  {
    EXPECT_EQ(translated, CURL(path).GetTranslatedProtocol()) << path;
  }
}

namespace
{

struct TestIsHostOnLANData
{
  const static bool DONT_TEST_LOCAL_NETWORK = false;

  bool expectedHostOnAnyLAN;
  bool expectedOnAnyLAN;
  bool expectedHostOnLocalLAN;
  bool expectedOnLocalLAN;
  std::string input;
  bool testLocalNetwork = true;
};

std::ostream& operator<<(std::ostream& os, const TestIsHostOnLANData& rhs)
{
  return os << rhs.input;
}

class TestLANParamTest : public testing::Test,
                         public testing::WithParamInterface<TestIsHostOnLANData>
{
};

const TestIsHostOnLANData values[] = {
    // clang-format off
    /*
    +-------------+--------------+
    |   Private   |    Local     |
    | Host | URL  | Host | URL   |
    +------+------+------+-------+   */
    // Assumption that hostnames without a period are local (smb, netbios)
    { true,  false, true,  false, "localhost" },
    { true,  true,  true,  true,  "some_unresolveable_hostname" },
    { true,  true,  true,  true,  "google" },
    { true,  true,  true,  true,  "aol" },

    { false, false, false, false, "www.some_unresolveable_hostname.com" },
    { false, false, false, false, "www.google.com" },
    { false, false, false, false, "google.com" },
    { false, false, false, false, "www.aol.com" },
    { false, false, false, false, "aol.com" },
    { false, false, false, false, "168.219.34.129" },

    { false, false, false, false, "0.0.0.0" },
    { false, false, false, false, "127.0.0.1" },
    /*
     * The following checks cannot be reliable tested on build machines
     * These tests pass for a class B network (172.16.0.0/16) but cannot be guaranteed to work under different network configurations
     */
    { true,  true,  false, false, "192.168.0.3", TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    { true,  true,  true,  true,  "172.16.0.2",  TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    { true,  true,  false, false, "10.0.0.9",    TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    // clang-format on
};

} // namespace

TEST_P(TestLANParamTest, TestIsHostOnLAN)
{
  using enum LanCheckMode;

  auto& param = GetParam();
  auto hostnameURL = "http://" + param.input;

  EXPECT_EQ(param.expectedHostOnAnyLAN, URIUtils::IsHostOnLAN(param.input, ANY_PRIVATE_SUBNET));
  EXPECT_EQ(param.expectedOnAnyLAN, URIUtils::IsOnLAN(hostnameURL, ANY_PRIVATE_SUBNET));

  if (param.testLocalNetwork)
  {
    EXPECT_EQ(param.expectedHostOnLocalLAN, URIUtils::IsHostOnLAN(param.input, ONLY_LOCAL_SUBNET));
    EXPECT_EQ(param.expectedOnLocalLAN, URIUtils::IsOnLAN(hostnameURL, ONLY_LOCAL_SUBNET));
  }
}

INSTANTIATE_TEST_SUITE_P(TestURIUtilsClassification, TestLANParamTest, testing::ValuesIn(values));
