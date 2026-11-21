/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace
{

struct PathCase
{
  const char* path;
  const char* expected;
};

struct PathHasParentCase
{
  const char* path;
  const char* parent;
  bool expected;
};

const PathHasParentCase PathHasParentCases[] = {
    {"/path/to/movie.avi", "/path/to/", true},
    {"/path/to/movie.avi", "/path/2/", false},
    {"/path/archive.zip/movie.mkv", "/path/archive.zip", true},
    {"C:\\videos\\movie.mkv", "C:\\videos", true},
    {"C:\\videos2\\movie.mkv", "C:\\videos", false},
    {"/dir/file", "/dir", true},
    {"/dir/file", "/dir/file", true},
    {"/directory/file", "/dir", false},
    {"http://host/dir/file", "http://host/dir", true},
    {"http://host/directory/file", "http://host/dir", false},
    // An empty query or fragment is part of a magnet member's identity
    {"magnet:/dir/file?", "magnet:/dir/", false},
    {"magnet:/dir/file", "magnet:/dir/?", false},
    {"magnet:/dir/file#", "magnet:/dir/", false},
    {"magnet:/dir/file", "magnet:/dir/#", false},
};

const PathCase GetDirectoryCases[] = {
    {"/path/to/movie.avi", "/path/to/"},
    {"/path/to/", "/path/to/"},
    {"/path/to/movie.avi|option=foo", "/path/to/|option=foo"},
    {"/path/to/|option=foo", "/path/to/|option=foo"},
    {"movie.avi", ""},
    {"movie.avi|option=foo", ""},
    {"", ""},
    {"ftp://user;pass@host:77/", "ftp://user;pass@host:77/"},
    {"http://user;pass@host:123", "http://"},
    {"ftp://user;pass@host:77/dir/movie.mkv|seekable=0", "ftp://user;pass@host:77/dir/|seekable=0"},
    {"http://user;pass@host:123/dir/movie.mkv?next=/other/file",
     "http://user;pass@host:123/dir/movie.mkv?next=/other/"},
    {"special://home/a?b/leaf", "special://home/a?b/"},
    {"file:///tmp/a?b/leaf", "file:///tmp/a?b/"},
    // A delimiter in a legacy URL's file name is part of the name
    {"smb://host/share/album#1/movie.mkv", "smb://host/share/album#1/"},
    {"sftp://host/dir?name/movie.mkv", "sftp://host/dir?name/"},
    {"ftp://host/dir#name/movie.mkv?cache=1", "ftp://host/dir#name/"},
    {"nfs://host/share/#recently-added/movie.mkv", "nfs://host/share/#recently-added/"},
};

// A dotted name is not split without a list of known extensions, so the last dot always wins.
const PathCase GetExtensionCases[] = {
    {"/path/to/movie.avi", ".avi"},
    {"/path/to/movie.", ""},
    {"/path/to/.movie", ""},
    {"/path/to/movie", ""},
    {"/path/to/.movie.ext", ".ext"},
    {"/path/to/movie.tar.gz", ".tar.gz"},
    {"/path/to/movie.name.avi", ".avi"},
    {"/path/to/movie.name.", ""},
    {"/path/to/.movie.name.ext", ".ext"},
    {"/path/to/movie.name.tar.gz", ".tar.gz"},
    {"D:\\path\\movie.avi", ".avi"},
    {"D:\\path\\movie.", ""},
    {"D:\\path\\.movie", ""},
    {"D:\\path\\movie", ""},
    {"D:\\path\\.movie.ext", ".ext"},
    {"D:\\path\\movie.tar.gz", ".tar.gz"},
    {"D:\\path\\movie.name.avi", ".avi"},
    {"D:\\path\\movie.name.", ""},
    {"D:\\path\\.movie.name.ext", ".ext"},
    {"D:\\path\\movie.name.tar.gz", ".tar.gz"},
    {"\\\\Server\\path\\movie.avi", ".avi"},
    {"\\\\Server\\path\\movie.", ""},
    {"\\\\Server\\path\\.movie", ""},
    {"\\\\Server\\path\\movie", ""},
    {"\\\\Server\\path\\.movie.ext", ".ext"},
    {"\\\\Server\\path\\movie.tar.gz", ".tar.gz"},
    {"\\\\Server\\path\\movie.name.avi", ".avi"},
    {"\\\\Server\\path\\movie.name.", ""},
    {"\\\\Server\\path\\.movie.name.ext", ".ext"},
    {"\\\\Server\\path\\movie.name.tar.gz", ".tar.gz"},
    {"smb://path/to/movie.avi", ".avi"},
    {"smb://path/to/movie.", ""},
    {"smb://path/to/.movie", ""},
    {"smb://path/to/movie", ""},
    {"smb://path/to/.movie.ext", ".ext"},
    {"smb://path/to/movie.tar.gz", ".tar.gz"},
    {"smb://path/to/movie.name.avi", ".avi"},
    {"smb://path/to/movie.name.", ""},
    {"smb://path/to/.movie.name.ext", ".ext"},
    {"smb://path/to/movie.name.tar.gz", ".tar.gz"},
    {".", ""},
    {"..", ""},
    {"/path/to/.", ""},
    {"/path/to/..", ""},
    {"D:\\path\\.", ""},
    {"D:\\path\\..", ""},
    {"\\\\server\\path\\.", ""},
    {"\\\\server\\path\\..", ""},
    {"movie.avi", ".avi"},
    {"movie.", ""},
    {".movie", ""},
    {"movie", ""},
    {".movie.ext", ".ext"},
    {"movie.tar.gz", ".tar.gz"},
    {"movie.name.avi", ".avi"},
    {"movie.name.", ""},
    {".movie.name.ext", ".ext"},
    {"movie.name.tar.gz", ".tar.gz"},
};

struct HasExtensionCase
{
  const char* path;
  // nullptr asks whether the path has any extension at all
  const char* extensions;
  bool expected;
};

const HasExtensionCase HasExtensionCases[] = {
    {"", nullptr, false},
    {"", ".avi", false},
    {"", ".avi|.mkv|.mp4", false},
    {"/path/to/movie.AvI", nullptr, true},
    {"/path/to/movie", nullptr, false},
    {"/path/.to/movie", nullptr, false},
    {"/path/to/movie.AvI", ".avi", true},
    {"/path/to/movie.AvI", ".mkv", false},
    {"/path/.avi/movie", ".avi", false},
    {"/path/movie.AvI", ".avi|.mkv|.mp4", true},
    {"/path/movie.AvI", ".mkv|.avi|.mp4", true},
    {"/path/movie.AvI", ".mpg|.mkv|.mp4", false},
    {"/path.mkv/movie.AvI", ".mpg|.mkv|.mp4", false},
    {"D:\\Path\\movie.AvI", nullptr, true},
    {"D:\\Path\\movie", nullptr, false},
    {"D:\\Path\\.to\\movie", nullptr, false},
    {"D:\\Path\\movie.AvI", ".avi", true},
    {"D:\\Path\\movie.AvI", ".mkv", false},
    {"D:\\Path\\.avi\\movie", ".avi", false},
    {"D:\\Path\\movie.AvI", ".avi|.mkv|.mp4", true},
    {"D:\\Path\\movie.AvI", ".mkv|.avi|.mp4", true},
    {"D:\\Path\\movie.AvI", ".mpg|.mkv|.mp4", false},
    {"D:\\Path.mkv\\movie.AvI", ".mpg|.mkv|.mp4", false},
    {"smb://path/to/movie.AvI", nullptr, true},
    {"smb://path/to/movie", nullptr, false},
    {"smb://path/.to/movie", nullptr, false},
    {"smb://path/to/movie.AvI", ".avi", true},
    {"smb://path/to/movie.AvI", ".mkv", false},
    {"smb://path/.avi/movie", ".avi", false},
    {"smb://path/movie.AvI", ".avi|.mkv|.mp4", true},
    {"smb://path/movie.AvI", ".mkv|.avi|.mp4", true},
    {"smb://path/movie.AvI", ".mpg|.mkv|.mp4", false},
    {"smb://path.mkv/movie.AvI", ".mpg|.mkv|.mp4", false},
    {"movie.AvI", nullptr, true},
    {"movie", nullptr, false},
    {"movie.", nullptr, false},
    {".movie", nullptr, false},
    {"movie.AvI", ".avi", true},
    {"movie.AvI", ".mkv", false},
    {"movie", ".avi", false},
    {"movie.", ".avi", false},
    {".movie", ".avi", false},
    {"movie.AvI", ".avi|.mkv|.mp4", true},
    {"movie.AvI", ".mkv|.avi|.mp4", true},
    {"movie.AvI", ".mpg|.mkv|.mp4", false},
};

const PathCase RemoveExtensionCases[] = {
    {"/path/to/file.xml", "/path/to/file"},
    {"/path/to/file.zip", "/path/to/file"},
    {"/path/to/file.rar", "/path/to/file"},
    {"/path/to/file.gz", "/path/to/file"},
    {"/path/to/file.tar", "/path/to/file"},
    {"/path/to/file.tar.gz", "/path/to/file"},
    {"/path/to/file.", "/path/to/file"},
    {"/path/to/file", "/path/to/file"},
    {"/path/to/.file", "/path/to/.file"},
    {"/path/to/file.name.mp4", "/path/to/file.name"},
    {"/path/to/file.name.tar.gz", "/path/to/file.name"},
    {".", "."},
    {"..", ".."},
    {"", ""},
    {"file.xml", "file"},
    {"file.zip", "file"},
    {"file.rar", "file"},
    {"file.gz", "file"},
    {"file.tar", "file"},
    {"file.tar.gz", "file"},
    {"file.", "file"},
    {"file", "file"},
    {".file", ".file"},
    {"file.name.mp4", "file.name"},
    {"file.name.tar.gz", "file.name"},
};

const PathCase ReplaceExtensionCases[] = {
    {"/path/to/file.xml", "/path/to/file.xsd"}, {"/path/to/file.tar.gz", "/path/to/file.xsd"},
    {"/path/to/file.", "/path/to/file.xsd"},    {"/path/to/file", "/path/to/file.xsd"},
    {"/path/to/.file", "/path/to/.file.xsd"},   {"/path/to/.file.xml", "/path/to/.file.xsd"},
};

struct SplitCase
{
  const char* path;
  // nullptr where only the file is checked
  const char* directory;
  const char* file;
};

const SplitCase SplitCases[] = {
    {"/path/to/movie.avi", "/path/to/", "movie.avi"},
    {"/path/to/movie?movie.avi", "/path/to/", "movie?movie.avi"},
    {"file:///path/to/movie.avi?showinfo=true", "file:///path/to/", "movie.avi"},
    {"ftp://user;pass@host:77/dir/movie.mkv|seekable=0", "ftp://user;pass@host:77/dir/",
     "movie.mkv"},
    {"http://user;pass@host:123/dir/movie.mkv?next=/other/file",
     "http://user;pass@host:123/dir/movie.mkv?next=/other/", "file"},
    {"smb://host/share/album#1/movie.mkv", "smb://host/share/album#1/", "movie.mkv"},
    {"sftp://host/dir?name/movie.mkv", "sftp://host/dir?name/", "movie.mkv"},
    {"ftp://host/dir#name/movie.mkv?cache=1", "ftp://host/dir#name/", "movie.mkv"},
    {"nfs://host/share/#recently-added/movie.mkv", "nfs://host/share/#recently-added/",
     "movie.mkv"},
    {"pvr://guide", nullptr, "guide"},
    {"iso9660://path\\in\\video_ts.iso\\file.iso", nullptr, "file.iso"},
    // Split ends a legacy URL's file name at a '?'
    {"smb://host/share/a?b.mkv", "smb://host/share/", "a"},
    {"special://home/a?b", "special://home/", "a"},
};

struct FileNameCase
{
  const char* path;
  // CURL's file name, where it differs from the path; nullptr where it is not checked
  const char* urlFileName;
  const char* fileName;
};

// URIUtils::GetFileName(), URIUtils::Split() and Split() of CURL's file name all agree.
const FileNameCase FileNameCases[] = {
    {"/path/to/movie.avi", nullptr, "movie.avi"},
    {"?", "?", "?"},
    {"C:", nullptr, ""},
    {"/", nullptr, ""},
    {"\\", nullptr, ""},
    {"/.thing", nullptr, ".thing"},
    {"/hello/there/.thing", nullptr, ".thing"},
    {"/hello/there/a:b", nullptr, "a:b"},
    {"nfs://127.0.0.1/srv/share/movie.avi?option=true", "srv/share/movie.avi", "movie.avi"},
    {"nfs://127.0.0.1/srv/share/movie.avi|option=true", "srv/share/movie.avi", "movie.avi"},
    {"nfs://127.0.0.1/srv/share/#recently-added/movie.avi", "srv/share/#recently-added/movie.avi",
     "movie.avi"},
    {"nfs://127.0.0.1/srv/share/Comedy;Drama/movie.avi", "srv/share/Comedy;Drama/movie.avi",
     "movie.avi"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "BDMV/PLAYLIST/00800.mpls", "00800.mpls"},
    {"zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/movie.mkv", "movie.mkv", "movie.mkv"},
    {"rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv", "BDMV/index.bdmv",
     "index.bdmv"},
    {"archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv", "BDMV/index.bdmv",
     "index.bdmv"},
};

struct CommonPathCase
{
  const char* path;
  const char* other;
  const char* expected;
};

const CommonPathCase GetCommonPathCases[] = {
    {"/a/b/", "/a/b/", "/a/b/"},
    {"/a/b/cd1/", "/a/b/cd2", "/a/b/"},
    {"/a/b/", "/a/b/c/", "/a/b/"},
    {"/a/", "/b/", "/"},
    {"/a/b/movie.avi", "/a/b/", "/a/b/"},
    {"/a/b/cd1/movie.avi", "/a/b/cd2", "/a/b/"},
    {"/a/b/movie.avi", "/a/b/c/", "/a/b/"},
    {"/a/movie.avi", "/b/", "/"},
    {"/Media/disc/movie.mkv", "/media/disc/other.mkv", "/Media/disc/"},
};

const PathCase GetRealPathCases[] = {
    {"/path/to/file/", "/path/to/file/"},
    {"../path/to/file", "path/to/file"},
    {"./path/to/file", "path/to/file"},
    {"/path/to/file", "/path/to/file"},
    {"/path/to/./file", "/path/to/file"},
    {"/./path/to/./file", "/path/to/file"},
    {"/path/to/some/../file", "/path/to/file"},
    {"/../path/to/some/../file", "/path/to/file"},
    {"/path/to/some/../file/..", "/path/to"},
#ifdef TARGET_WINDOWS
    {"\\\\path\\to\\file\\", "\\\\path\\to\\file\\"},
    {"..\\path\\to\\file", "path\\to\\file"},
    {".\\path\\to\\file", "path\\to\\file"},
    {"\\\\path\\to\\file", "\\\\path\\to\\file"},
    {"\\\\path\\to\\.\\file", "\\\\path\\to\\file"},
    {"\\\\.\\path/to\\.\\file", "\\\\path\\to\\file"},
    {"\\\\path\\to\\some\\..\\file", "\\\\path\\to\\file"},
    {"\\\\..\\path\\to\\some\\..\\file", "\\\\path\\to\\file"},
    {"\\\\path\\to\\some\\..\\file\\..", "\\\\path\\to"},
#endif
};

// The archive is resolved inside the encoded hostname, whose escapes may change case.
const PathCase GetRealArchivePathCases[] = {
    {"zip://%2fpath%2fto%2fzip/subpath/to/file", "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2fzip/../subpath/to/file", "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2fzip/./subpath/to/file", "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2fzip/subpath/to/./file", "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2fzip/subpath/to/some/../file",
     "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2f.%2fzip/subpath/to/file", "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://%2fpath%2fto%2fsome%2f..%2fzip/subpath/to/file",
     "zip://%2fpath%2fto%2fzip/subpath/to/file"},
    {"zip://"
     "zip%3a%2f%2f%252Fpath%252Fto%252Fsome%252F..%252Fzip%2fpath%2fto%2fsome%2f..%2fzip/subpath/"
     "to/some/../file",
     "zip://zip%3a%2f%2f%252Fpath%252Fto%252Fzip%2fpath%2fto%2fzip/subpath/to/file"},
};

struct SubstitutePathCase
{
  const char* path;
  bool reverse;
  const char* expected;
};

const SubstitutePathCase SubstitutePathCases[] = {
    {"C:\\My Videos\\sub dir\\movie name.avi", false,
     "https://myserver/some%20other%20path/sub%20dir/movie%20name.avi"},
    {"https://myserver/some%20other%20path/sub%20dir/movie%20name.avi", true,
     "C:\\My Videos\\sub dir\\movie name.avi"},
    {"davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3",
     false, "D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3"},
    {"D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3", true,
     "davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3"},
    {"/this/path1/to/movie.avi", false, "/some/other/path2/to/movie.avi"},
    {"/some/other/path2/to/movie.avi", true, "/this/path1/to/movie.avi"},
    {"/no/translation path/", false, "/no/translation path/"},
    {"/no/translation path/", true, "/no/translation path/"},
    {"c:\\no\\translation path", false, "c:\\no\\translation path"},
    {"c:\\no\\translation path", true, "c:\\no\\translation path"},
};

struct TestPathData
{
  const char* path;
  const char* parent;
  const char* base;
};

class TestParentPath : public testing::Test, public testing::WithParamInterface<TestPathData>
{
};

class TestBasePath : public testing::Test, public testing::WithParamInterface<TestPathData>
{
};

const TestPathData Paths[] = {
    {.path = "folder/disc/movie.mkv", .parent = "folder/disc/", .base = "folder/disc/"},
#ifdef TARGET_WINDOWS
    {.path = "folder\\disc\\movie.mkv", .parent = "folder\\disc/", .base = "folder\\disc\\"},
#endif
    // Linux path tests
    {.path = "/home/user/movies/movie/file.iso",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/file.iso",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/video_ts/VIDEO_TS.IFO",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/BDMV/index.bdmv",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/BDMV/index.bdmv",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack:///home/user/movies/movie.avi , "
             "/home/user/movies/movie.avi",
     .parent = "/home/user/movies/",
     .base = "/home/user/movies/"},
    {.path = "stack:///home/user/movies/movie/cd1/some_file1.avi , "
             "/home/user/movies/movie/cd2/some_file2.avi",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    // DOS path tests
    {.path = "D:\\movies\\movie\\file.iso",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\file.iso",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\video_ts\\VIDEO_TS.IFO",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\BDMV\\index.bdmv",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://D:\\movies\\movie_part_1.avi , "
             "D:\\movies\\movie_part_2.avi",
     .parent = "D:\\movies\\",
     .base = "D:\\movies\\"},
    {.path = "stack://D:\\movies\\movie\\movie_part_1\\file.avi , "
             "D:\\movies\\movie\\movie_part_2\\file.avi",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    // Windows server path tests
    {.path = "\\\\Server\\Movies\\movie\\file.iso",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\file.iso",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\video_ts\\VIDEO_TS.IFO",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\BDMV\\index.bdmv",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.bdmv",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie_part_1.avi , "
             "\\\\Server\\Movies\\movie_part_2.avi",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\movie_part_1\\file.avi , "
             "\\\\Server\\Movies\\movie\\movie_part_2\\file.avi",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    // URL path tests using smb://
    {.path = "smb://server/movies/movie/file.iso",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/file.iso",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/video_ts/VIDEO_TS.IFO",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/video_ts/VIDEO_TS.IFO",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/BDMV/index.bdmv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/BDMV/index.bdmv",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "stack://smb://home/user/movies/movie.avi , "
             "smb://home/user/movies/movie.avi",
     .parent = "smb://home/user/movies/",
     .base = "smb://home/user/movies/"},
    {.path = "stack://smb://home/user/movies/movie/cd1/some_file1.avi , "
             "smb://home/user/movies/movie/cd2/some_file2.avi",
     .parent = "smb://home/user/movies/movie/",
     .base = "smb://home/user/movies/movie/"},
    // Embedded URL path tests using smb://
    {.path = "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.rar/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.rar/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://"
             "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    // Embedded linux path tests
    {.path = "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fmovie.zip/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fmovie.rar/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fmovie.rar/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fmovie.tar.gz/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "bluray://%2fsomepath%2fpath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "bluray://"
             "udf%3a%2f%2f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    // Embedded DOS path tests
    {.path = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cmovie.zip/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cmovie.zip/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cmovie.rar/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cmovie.rar/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cmovie.tar.gz/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://D%3a%5cMovies%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cDisc%25201%255cmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cDisc%201%5cmovie.zip/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cDisc%201%5cmovie.zip/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cDisc%201%5cmovie.rar/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cDisc%201%5cmovie.rar/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cDisc%201%5cmovie.tar.gz/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cDisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    // Embedded windows server path tests
    {.path = "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
             "00800.mpls",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cmovie.zip/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cmovie.rar/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "bluray://%5c%5cServer%5cMovies%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path =
         "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cDisc%25201%255cmovie.iso%2f/BDMV/"
         "PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.zip/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.zip/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.rar/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.rar/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.tar.gz/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    //
    // Stack path tests
    //
    {.path = "stack:///home/user/movies/movie/movie_part1.mkv , "
             "/home/user/movies/movie/movie_part2.mkv , "
             "/home/user/movies/movie/movie_part3.mkv",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack:///home/user/movies/movie/disc 1/movie.mkv , "
             "/home/user/movies/movie/disc 2/movie.mkv , "
             "/home/user/movies/movie/disc 3/movie.mkv",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack://D:\\movies\\movie\\movie_part1.mkv , "
             "D:\\movies\\movie\\movie_part2.mkv , "
             "D:\\movies\\movie\\movie_part3.mkv",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://D:\\movies\\movie\\disc 1\\movie.mkv , "
             "D:\\movies\\movie\\disc 2\\movie.mkv , "
             "D:\\movies\\movie\\disc 3\\movie.mkv",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\movie_part1.mkv , "
             "\\\\Server\\Movies\\movie\\movie_part2.mkv , "
             "\\\\Server\\Movies\\movie\\movie_part3.mkv",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\disc 1\\movie.mkv , "
             "\\\\Server\\Movies\\movie\\disc 2\\movie.mkv , "
             "\\\\Server\\Movies\\movie\\disc 3\\movie.mkv",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://smb://server/movies/movie/movie_part1.mkv , "
             "smb://server/movies/movie/movie_part2.mkv , "
             "smb://server/movies/movie/movie_part3.mkv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "stack://smb://server/movies/movie/disc 1/movie.mkv , "
             "smb://server/movies/movie/disc 2/movie.mkv , "
             "smb://server/movies/movie/disc 3/movie.mkv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    // Stack path tests with disc BDMV structures
    {.path = "stack:///home/user/movies/movie/disc 1/BDMV/index.bdmv , "
             "/home/user/movies/movie/disc 2/BDMV/index.bdmv , "
             "/home/user/movies/movie/disc 3/BDMV/index.bdmv",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack://D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv , "
             "D:\\movies\\movie\\disc 2\\BDMV\\index.bdmv , "
             "D:\\movies\\movie\\disc 3\\BDMV\\index.bdmv",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.bdmv , "
             "\\\\Server\\Movies\\movie\\disc 2\\BDMV\\index.bdmv , "
             "\\\\Server\\Movies\\movie\\disc 3\\BDMV\\index.bdmv",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://smb://server/movies/movie/disc 1/BDMV/index.bdmv , "
             "smb://server/movies/movie/disc 2/BDMV/index.bdmv , "
             "smb://server/movies/movie/disc 3/BDMV/index.bdmv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    // Stack path tests with disc VIDEO_TS structures
    {.path = "stack:///home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO , "
             "/home/user/movies/movie/disc 2/video_ts/VIDEO_TS.IFO , "
             "/home/user/movies/movie/disc 3/video_ts/VIDEO_TS.IFO",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack://D:\\movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO , "
             "D:\\movies\\movie\\disc 2\\video_ts\\VIDEO_TS.IFO , "
             "D:\\movies\\movie\\disc 3\\video_ts\\VIDEO_TS.IFO",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO , "
             "\\\\Server\\Movies\\movie\\disc 2\\video_ts\\VIDEO_TS.IFO , "
             "\\\\Server\\Movies\\movie\\disc 3\\video_ts\\VIDEO_TS.IFO",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://smb://server/movies/movie/disc 1/video_ts/VIDEO_TS.IFO , "
             "smb://server/movies/movie/disc 2/video_ts/VIDEO_TS.IFO , "
             "smb://server/movies/movie/disc 3/video_ts/VIDEO_TS.IFO",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    // Stack path tests with mixed folders
    {.path = "stack://smb://server/movies/movie/disc 1/BDMV/index.bdmv , "
             "smb://server/movies/movie/disc 2/VIDEO_TS/VIDEO_TS.IFO , "
             "smb://server/movies/movie/disc 3/part a/movie.mkv , "
             "smb://server/movies/movie/disc 3/part b/movie.mkv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    //
    // The following tests are only specifically supported in GetParentPath()
    //
    // plugin:// path tests
    // GetParentPath has dedicated handling that jumps to addon root in one step;
    // GetBasePath falls through to GetDirectory which strips only the last component.
    {.path = "plugin://addon.id/path/to/file.strm",
     .parent = "plugin://addon.id/",
     .base = "plugin://addon.id/path/to/"},
    {.path = "plugin://addon.id/path/to/",
     .parent = "plugin://addon.id/",
     .base = "plugin://addon.id/path/to/"},
    {.path = "plugin://addon.id/", .parent = "plugin://", .base = "plugin://addon.id/"},
    // options stripped by GetParentPath; GetDirectory (in GetBasePath) ignores | and stops at last /
    {.path = "plugin://addon.id/path/file.strm|option=val",
     .parent = "plugin://addon.id/|option=val",
     .base = "plugin://addon.id/path/|option=val"},
    // single content_type option with empty filename -> GetParentPath clears host too
    {.path = "plugin://addon.id/?content_type=video",
     .parent = "plugin://",
     .base = "plugin://addon.id/"},
    // special:// path tests
    // For files: both agree (normal directory stripping).
    // For directories: GetParentPath strips one level; GetBasePath returns the path unchanged.
    {.path = "special://home/movies/movie.avi",
     .parent = "special://home/movies/",
     .base = "special://home/movies/"},
    {.path = "special://home/subfolder/movies/",
     .parent = "special://home/subfolder/",
     .base = "special://home/subfolder/movies/"},
    {.path = "special://home/movies/",
     .parent = "special://home/",
     .base = "special://home/movies/"},
    // multipath:// path tests
    // GetParentPath recurses into GetParentPath(GetFirstPath(...));
    // GetBasePath falls through to GetDirectory of the raw multipath URL (returns itself).
    {.path = "multipath://%2fpath1%2f/%2fpath2%2f/",
     .parent = "/",
     .base = "multipath://%2fpath1%2f/%2fpath2%2f/"},
    {.path = "multipath://smb%3a%2f%2fserver%2fmovies%2f/smb%3a%2f%2fserver2%2fmovies%2f/",
     .parent = "smb://server/",
     .base = "multipath://smb%3a%2f%2fserver%2fmovies%2f/smb%3a%2f%2fserver2%2fmovies%2f/"},
    // Bare hostname (no filename) tests
    // GetParentPath clears hostname to reach protocol root;
    // GetBasePath falls through to GetDirectory which keeps the trailing-slash path.
    {.path = "ftp://server/", .parent = "ftp://", .base = "ftp://server/"}};

struct GetFileOrFolderNameTest
{
  std::string_view input;
  std::string expected;
};

// The function was written for limited cases and has UB for inputs like D:\\ D:foo
// clang-format off
const auto GetFileOrFolderNameTests = std::array{
    GetFileOrFolderNameTest{"foo", "foo"},
    GetFileOrFolderNameTest{"foo/bar", "bar"},
    GetFileOrFolderNameTest{"foo/bar/", "bar"},
    GetFileOrFolderNameTest{"foo/", "foo"},
    GetFileOrFolderNameTest{"/foo", "foo"},
    GetFileOrFolderNameTest{"/foo/", "foo"},
    GetFileOrFolderNameTest{"/", ""},
    GetFileOrFolderNameTest{"", ""},
    GetFileOrFolderNameTest{"foo\\bar", "bar"},
    GetFileOrFolderNameTest{"foo\\bar\\", "bar"},
    GetFileOrFolderNameTest{"foo\\", "foo"},
    GetFileOrFolderNameTest{"\\foo\\", "foo"},
    GetFileOrFolderNameTest{"\\foo", "foo"},
    GetFileOrFolderNameTest{"smb://server/", "server"},
    GetFileOrFolderNameTest{"plugin://plugin.video.foo/", "plugin.video.foo"},
    GetFileOrFolderNameTest{"file:/videos/?x=1#part", "videos"},
    GetFileOrFolderNameTest{"file:/", ""},
    GetFileOrFolderNameTest{"urn:example:folder/", ""},
};
// clang-format on

class GetFileOrFolderNameTester : public testing::WithParamInterface<GetFileOrFolderNameTest>,
                                  public testing::Test
{
};

struct ParentAppendCase
{
  const char* file;
  const char* parent;
};

// The parent of a file, and the file added back to it, keep the query and fragment.
const ParentAppendCase ParentAppendRoundTrips[] = {
    {"file:movie.mkv", "file:"},
    {"file:/movie.mkv", "file:/"},
    {"file:movie.mkv?x=%2f&x=+#part", "file:?x=%2f&x=+#part"},
    {"file:/movie.mkv?x=%2f&x=+#part", "file:/?x=%2f&x=+#part"},
    {"http://host/movie.mkv?x=%2f#part", "http://host/?x=%2f#part"},
};

// Identifiers name a resource as a whole and have no hierarchy to navigate.
const char* const Identifiers[] = {
    "data:,hello",
    "DATA:text/plain,http://example.test/a//b?x=1#fragment",
    "data:text/plain,../movie.mkv",
    "data:text/plain,a%2Fb+%23c",
    "data://x/movie.mkv",
    "data:,bad%",
    "data:,a|b",
    "urn:example:a/b?+resolver?=query#part",
    "mailto:user@example.test",
    "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567",
};

struct NativeColonFileName
{
  const char* base;
  const char* extension;
};

// Native file names with a colon are not mistaken for URIs.
const NativeColonFileName NativeColonFileNames[] = {
    {"Re:Zero - 01", ".mkv"},          {"AC:DC - Thunderstruck", ".mp3"},
    {"Alien:Covenant (2017)", ".mkv"}, {"Special:Edition", ".mkv"},
    {"Archive:Disc 1", ".chd"},        {"Episodes:Part 2", ".mkv"},
    {"Library:Collection", ".mkv"},
};

const char* const AncestryRoots[] = {
    "Re:Zero/",          "example+v1:disc2/", "/media/archive.zip/", "/media/app.apk/",
    "smb://host/share/", "http://host/",      "custom+vfs://host/",
#ifndef TARGET_WINDOWS
    "x://host/",
#endif
    "C:\\media\\",
};

const PathCase ValidatePathCases[] = {
#ifdef TARGET_WINDOWS
    {"x://host/file", "x:\\host\\file"},
#else
    {"x://host/file", "x://host/file"},
#endif
    // An authority-free URI keeps its bytes
    {"http:/a//b?x=one//two#part//three", "http:/a//b?x=one//two#part//three"},
    {"http:\\\\host\\path", "http://host/path"},
};

class TestURIUtilsPaths : public testing::Test
{
protected:
  ~TestURIUtilsPaths() override
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions.clear();
  }
};

} // namespace

TEST_F(TestURIUtilsPaths, PathHasParent)
{
  for (const auto& [path, parent, expected] : PathHasParentCases)
  {
    EXPECT_EQ(expected, URIUtils::PathHasParent(path, parent)) << path << " in " << parent;
  }
}

TEST_F(TestURIUtilsPaths, GetDirectory)
{
  for (const auto& [path, expected] : GetDirectoryCases)
  {
    EXPECT_EQ(expected, URIUtils::GetDirectory(path)) << path;
  }
}

TEST_F(TestURIUtilsPaths, GetDirectoryIntoItsOwnArgument)
{
  std::string var = "/path/to/movie.avi|option=foo";
  var = URIUtils::GetDirectory(var);
  EXPECT_EQ("/path/to/|option=foo", var);
}

TEST_F(TestURIUtilsPaths, GetExtension)
{
  for (const auto& [path, expected] : GetExtensionCases)
  {
    EXPECT_EQ(expected, URIUtils::GetExtension(path)) << path;
  }
}

TEST_F(TestURIUtilsPaths, HasExtension)
{
  for (const auto& [path, extensions, expected] : HasExtensionCases)
  {
    if (extensions)
    {
      EXPECT_EQ(expected, URIUtils::HasExtension(path, extensions)) << path << " " << extensions;
    }
    else
    {
      EXPECT_EQ(expected, URIUtils::HasExtension(path)) << path;
    }
  }
}

TEST_F(TestURIUtilsPaths, RemoveExtension)
{
  for (const auto& [path, expected] : RemoveExtensionCases)
  {
    std::string var{path};
    URIUtils::RemoveExtension(var);
    EXPECT_EQ(expected, var) << path;
  }
}

TEST_F(TestURIUtilsPaths, ReplaceExtension)
{
  for (const auto& [path, expected] : ReplaceExtensionCases)
  {
    EXPECT_EQ(expected, URIUtils::ReplaceExtension(path, ".xsd")) << path;
  }
}

TEST_F(TestURIUtilsPaths, Split)
{
  for (const auto& [path, expectedDirectory, expectedFile] : SplitCases)
  {
    std::string directory;
    std::string file;
    URIUtils::Split(path, directory, file);
    if (expectedDirectory)
    {
      EXPECT_EQ(expectedDirectory, directory) << path;
    }
    EXPECT_EQ(expectedFile, file) << path;
  }
}

TEST_F(TestURIUtilsPaths, SplitPath)
{
  const std::vector<std::string> array{
      URIUtils::SplitPath("http://www.test.com/path/to/movie.avi")};

  EXPECT_EQ("http://www.test.com/", array.at(0));
  EXPECT_EQ("path", array.at(1));
  EXPECT_EQ("to", array.at(2));
  EXPECT_EQ("movie.avi", array.at(3));
}

TEST_F(TestURIUtilsPaths, SplitPathLocal)
{
#ifndef TARGET_LINUX
  const char* path = "C:\\path\\to\\movie.avi";
#else
  const char* path = "/path/to/movie.avi";
#endif
  const std::vector<std::string> array{URIUtils::SplitPath(path)};

#ifndef TARGET_LINUX
  EXPECT_EQ("C:", array.at(0));
#else
  EXPECT_EQ("", array.at(0));
#endif
  EXPECT_EQ("path", array.at(1));
  EXPECT_EQ("to", array.at(2));
  EXPECT_EQ("movie.avi", array.at(3));
}

TEST_F(TestURIUtilsPaths, FileNames)
{
  const auto splitFileName = [](const std::string& path)
  {
    std::string directory;
    std::string file;
    URIUtils::Split(path, directory, file);
    return file;
  };

  for (const auto& [path, urlFileName, fileName] : FileNameCases)
  {
    SCOPED_TRACE(path);
    const std::string curlFileName{CURL(path).GetFileName()};
    if (urlFileName)
    {
      EXPECT_EQ(urlFileName, curlFileName);
    }
    EXPECT_EQ(fileName, URIUtils::GetFileName(path));
    EXPECT_EQ(fileName, splitFileName(path));
    EXPECT_EQ(fileName, splitFileName(curlFileName));
  }
}

TEST_F(TestURIUtilsPaths, GetCommonPath)
{
  for (const auto& [path, other, expected] : GetCommonPathCases)
  {
    std::string var{path};
    URIUtils::GetCommonPath(var, other);
    EXPECT_EQ(expected, var) << path << " and " << other;
  }
}

TEST_P(TestParentPath, GetParentPath)
{
  const std::string& path{GetParam().path};
  const std::string& parent{URIUtils::GetParentPath(path)};
  EXPECT_EQ(parent, GetParam().parent);
}

INSTANTIATE_TEST_SUITE_P(ParentPath, TestParentPath, testing::ValuesIn(Paths));

TEST_P(TestBasePath, GetBasePath)
{
  const std::string& path{GetParam().path};
  const std::string& base{URIUtils::GetBasePath(path)};
  EXPECT_EQ(base, GetParam().base);
}

INSTANTIATE_TEST_SUITE_P(BasePath, TestBasePath, testing::ValuesIn(Paths));

TEST_P(GetFileOrFolderNameTester, TestValue)
{
  EXPECT_EQ(GetParam().expected, URIUtils::GetFileOrFolderName(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(TestURIUtilsPaths,
                         GetFileOrFolderNameTester,
                         testing::ValuesIn(GetFileOrFolderNameTests));

TEST_F(TestURIUtilsPaths, Slashes)
{
  std::string var{"bluray://path/to/file/"};
  URIUtils::AddSlashAtEnd(var);
  EXPECT_EQ("bluray://path/to/file/", var);

  EXPECT_TRUE(URIUtils::HasSlashAtEnd("bluray://path/to/file/"));
  EXPECT_FALSE(URIUtils::HasSlashAtEnd("bluray://path/to/file"));

  var = "bluray://path/to/file/";
  URIUtils::RemoveSlashAtEnd(var);
  EXPECT_EQ("bluray://path/to/file", var);

  EXPECT_EQ("iso9660://path/in/video_ts.txt/",
            URIUtils::AppendSlash("iso9660://path/in/video_ts.txt\\"));
}

TEST_F(TestURIUtilsPaths, AddFileToFolder)
{
  EXPECT_EQ("/path/to/file", URIUtils::AddFileToFolder("/path/to", "file"));
  EXPECT_EQ("/path/to/file/and/more",
            URIUtils::AddFileToFolder("/path", "to", "file", "and", "more"));
  EXPECT_EQ("http://host/movie.mkv", URIUtils::AddFileToFolder("http://host", "movie.mkv"));
}

TEST_F(TestURIUtilsPaths, GetRealPath)
{
  for (const auto& [path, expected] : GetRealPathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetRealPath(path)) << path;
  }

  for (const auto& [path, expected] : GetRealArchivePathCases)
  {
    EXPECT_STRCASEEQ(expected, URIUtils::GetRealPath(path).c_str()) << path;
  }
}

TEST_F(TestURIUtilsPaths, SubstitutePath)
{
  auto& substitutions =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions;
  substitutions.emplace_back("C:\\My Videos", "https://myserver/some%20other%20path");
  substitutions.emplace_back("/this/path1", "/some/other/path2");
  substitutions.emplace_back("davs://otherserver/my%20music%20path",
                             "D:\\Local Music\\MP3 Collection");

  for (const auto& [path, reverse, expected] : SubstitutePathCases)
  {
    EXPECT_EQ(expected, URIUtils::SubstitutePath(path, reverse)) << path;
  }
}

TEST_F(TestURIUtilsPaths, SubstitutePathPreservesExplicitIntent)
{
  auto& substitutions =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions;
  substitutions.clear();
  for (const bool haveSubstitutions : {false, true})
  {
    SCOPED_TRACE(haveSubstitutions);
    if (haveSubstitutions)
    {
      substitutions.emplace_back("/unrelated/source", "https://host/target");
    }

    for (const bool reverse : {false, true})
    {
      SCOPED_TRACE(reverse);
      CURL native;
      native.ParseNativePath("notes:2026");
      const CURL result = URIUtils::SubstitutePath(native, reverse);
      EXPECT_EQ("notes:2026", result.Get());
      EXPECT_FALSE(result.IsLosslessURI());
      EXPECT_TRUE(result.SupportsPathOperations());
      EXPECT_TRUE(result.GetProtocol().empty());
      EXPECT_EQ("notes:2026", result.GetFileName());
      EXPECT_EQ("notes:2026", result.GetRawPath());
      EXPECT_FALSE(result.HasAuthority());
      EXPECT_FALSE(result.HasQuery());
      EXPECT_FALSE(result.HasFragment());

      for (const std::string input : {"X:/dir/file?x=1#part", "HTTP://Host/dir/file?x=1#part"})
      {
        SCOPED_TRACE(input);
        CURL uri;
        ASSERT_TRUE(uri.ParseURI(input));
        const CURL substituted = URIUtils::SubstitutePath(uri, reverse);
        EXPECT_EQ(input, substituted.Get());
        EXPECT_TRUE(substituted.IsLosslessURI());
        EXPECT_EQ(uri.GetProtocol(), substituted.GetProtocol());
        EXPECT_EQ(uri.SupportsPathOperations(), substituted.SupportsPathOperations());
        EXPECT_EQ(uri.GetFileName(), substituted.GetFileName());
        EXPECT_EQ(uri.GetHostName(), substituted.GetHostName());
        EXPECT_EQ(uri.HasAuthority(), substituted.HasAuthority());
        EXPECT_EQ(uri.GetRawAuthority(), substituted.GetRawAuthority());
        EXPECT_EQ("/dir/file", substituted.GetRawPath());
        EXPECT_TRUE(substituted.HasQuery());
        EXPECT_EQ("x=1", substituted.GetRawQuery());
        EXPECT_TRUE(substituted.HasFragment());
        EXPECT_EQ("part", substituted.GetRawFragment());
      }
    }
  }
}

TEST_F(TestURIUtilsPaths, SubstitutePathUsesReplacementIntent)
{
  auto& substitutions =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions;
  substitutions.emplace_back("/local", "https://host/remote");
  CURL native;
  native.ParseNativePath("/local/movie name.mkv");
  const CURL remote = URIUtils::SubstitutePath(native);
  EXPECT_EQ("https://host/remote/movie%20name.mkv", remote.Get());
  EXPECT_EQ("https", remote.GetProtocol());
  EXPECT_EQ("host", remote.GetHostName());
  EXPECT_EQ("remote/movie%20name.mkv", remote.GetFileName());
  EXPECT_EQ("/remote/movie%20name.mkv", remote.GetRawPath());
  EXPECT_TRUE(remote.HasAuthority());
  EXPECT_FALSE(remote.IsLosslessURI());
  EXPECT_TRUE(remote.SupportsPathOperations());

  CURL explicitURI;
  ASSERT_TRUE(explicitURI.ParseURI("https://host/remote/movie%20name.mkv"));
  for (const CURL& uri : {remote, explicitURI})
  {
    const CURL local = URIUtils::SubstitutePath(uri, true);
    EXPECT_EQ("/local/movie name.mkv", local.Get());
    EXPECT_TRUE(local.GetProtocol().empty());
    EXPECT_EQ("/local/movie name.mkv", local.GetFileName());
    EXPECT_EQ("/local/movie name.mkv", local.GetRawPath());
    EXPECT_FALSE(local.HasAuthority());
    EXPECT_FALSE(local.HasQuery());
    EXPECT_FALSE(local.HasFragment());
    EXPECT_FALSE(local.IsLosslessURI());
    EXPECT_TRUE(local.SupportsPathOperations());
  }
}

TEST_F(TestURIUtilsPaths, ParentAppendRoundTrip)
{
  for (const auto& [file, parent] : ParentAppendRoundTrips)
  {
    SCOPED_TRACE(file);
    EXPECT_EQ(parent, URIUtils::GetParentPath(file));
    EXPECT_EQ(file, URIUtils::AddFileToFolder(parent, "movie.mkv"));
  }
}

TEST_F(TestURIUtilsPaths, IdentifierPathOperationsHaveNoHierarchy)
{
  for (const std::string identifier : Identifiers)
  {
    SCOPED_TRACE(identifier);
    EXPECT_TRUE(URIUtils::GetFileName(identifier).empty());
    EXPECT_TRUE(URIUtils::GetDirectory(identifier).empty());
    EXPECT_TRUE(URIUtils::GetExtension(identifier).empty());
    EXPECT_FALSE(URIUtils::HasExtension(identifier));
    EXPECT_FALSE(URIUtils::HasExtension(identifier, ".mkv|.test"));
    EXPECT_TRUE(URIUtils::SplitPath(identifier).empty());
    std::string parent{"stale"};
    EXPECT_FALSE(URIUtils::GetParentPath(identifier, parent));
    EXPECT_TRUE(parent.empty());
    std::string file{"stale"};
    URIUtils::Split(identifier, parent, file);
    EXPECT_TRUE(parent.empty());
    EXPECT_TRUE(file.empty());
    EXPECT_TRUE(URIUtils::AddFileToFolder(identifier, "child").empty());
    EXPECT_TRUE(URIUtils::AddFileToFolder(identifier, "child", "leaf").empty());
    EXPECT_FALSE(URIUtils::HasSlashAtEnd(identifier, true));
    std::string modified{identifier};
    URIUtils::AddSlashAtEnd(modified);
    EXPECT_EQ(identifier, modified);
    URIUtils::RemoveSlashAtEnd(modified);
    EXPECT_EQ(identifier, modified);
    URIUtils::RemoveExtension(modified);
    EXPECT_EQ(identifier, modified);
    EXPECT_EQ(identifier, URIUtils::ReplaceExtension(identifier, ".avi"));
    EXPECT_EQ(identifier, URIUtils::GetRealPath(identifier));
    EXPECT_EQ(identifier, URIUtils::CanonicalizePath(identifier, '/'));
    EXPECT_EQ(identifier, URIUtils::FixSlashesAndDups(identifier));
    EXPECT_EQ(identifier, URIUtils::SanitiseUrlEncoding(identifier));
    EXPECT_EQ(identifier, URIUtils::GetDecodedPath(identifier));
    EXPECT_EQ(identifier, CUtil::ValidatePath(identifier, true));
    EXPECT_TRUE(URIUtils::ChangeBasePath(identifier, "file.mkv", "/tmp").empty());
    EXPECT_TRUE(URIUtils::ChangeBasePath("/tmp", "file.mkv", identifier).empty());
    std::string base{"stale"};
    CUtil::GetVideoBasePathAndFileName(identifier, base, file);
    EXPECT_TRUE(base.empty());
    EXPECT_TRUE(file.empty());
  }
}

TEST_F(TestURIUtilsPaths, MagnetMemberNavigationPreservesIdentity)
{
  // Kodi member paths extend BEP 9; repeated trackers remain intact during navigation.
  // https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
  const std::string identity =
      "?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&tr=https://one.test/"
      "&tr=https%3a%2F%2Ftwo.test&empty=&flag&&x=a+b#part";
  const std::string member = "magnet:/dir/movie.mkv" + identity;
  EXPECT_EQ("movie.mkv", URIUtils::GetFileName(member));
  EXPECT_EQ(".mkv", URIUtils::GetExtension(member));
  EXPECT_EQ("magnet:/dir/" + identity, URIUtils::GetDirectory(member));
  EXPECT_EQ("magnet:/dir/" + identity, URIUtils::GetParentPath(member));
  EXPECT_EQ("magnet:/" + identity, URIUtils::GetParentPath("magnet:/dir/" + identity));
  EXPECT_TRUE(URIUtils::GetParentPath("magnet:/" + identity).empty());
  EXPECT_TRUE(URIUtils::GetParentPath("magnet:" + identity).empty());
  EXPECT_EQ(member, URIUtils::AddFileToFolder("magnet:/dir" + identity, "movie.mkv"));
  EXPECT_EQ("magnet:/dir/movie.avi" + identity, URIUtils::ReplaceExtension(member, ".avi"));
  std::string parent;
  std::string filename;
  URIUtils::Split(member, parent, filename);
  EXPECT_EQ("magnet:/dir/" + identity, parent);
  EXPECT_EQ("movie.mkv", filename);
  std::string folder = "magnet:/dir" + identity;
  URIUtils::AddSlashAtEnd(folder);
  EXPECT_EQ("magnet:/dir/" + identity, folder);
  EXPECT_TRUE(URIUtils::HasSlashAtEnd(folder, true));
  URIUtils::RemoveSlashAtEnd(folder);
  EXPECT_EQ("magnet:/dir" + identity, folder);
  CUtil::GetVideoBasePathAndFileName(member, parent, filename);
  EXPECT_EQ("magnet:/dir/" + identity, parent);
  EXPECT_EQ("movie", filename);
}

TEST_F(TestURIUtilsPaths, MagnetMemberAncestryPreservesIdentity)
{
  const std::string identity =
      "?xt=urn:btih:0123456789012345678901234567890123456789&tr=one&tr=two&x=a+b%2f#part";
  const std::string child = "magnet:/dir/movie.mkv" + identity;
  const std::string parent = URIUtils::GetParentPath(child);
  ASSERT_EQ("magnet:/dir/" + identity, parent);
  EXPECT_TRUE(URIUtils::PathHasParent(child, parent));
  EXPECT_TRUE(URIUtils::PathHasParent(child, "magnet:/dir" + identity));
  EXPECT_TRUE(URIUtils::PathHasParent(child, "magnet:/" + identity));
  EXPECT_TRUE(URIUtils::PathHasParent(parent, URIUtils::GetParentPath(parent)));
  EXPECT_TRUE(URIUtils::PathHasParent(child, child));
  EXPECT_TRUE(URIUtils::PathHasParent(parent, parent));
  EXPECT_TRUE(URIUtils::PathHasParent("magnet:/" + identity, "magnet:/" + identity));
  EXPECT_FALSE(URIUtils::PathHasParent("magnet:/" + identity, parent));
  EXPECT_FALSE(URIUtils::PathHasParent("magnet:/dir" + identity, parent));
  EXPECT_FALSE(URIUtils::PathHasParent("magnet:/directory/movie.mkv" + identity, parent));
  EXPECT_FALSE(URIUtils::PathHasParent(child, "magnet:" + identity));

  for (const std::string differentIdentity :
       {"?xt=urn:btih:1123456789012345678901234567890123456789&tr=one&tr=two&x=a+b%2f#part",
        "?xt=urn:btih:0123456789012345678901234567890123456789&tr=two&tr=one&x=a+b%2f#part",
        "?xt=urn:btih:0123456789012345678901234567890123456789&tr=one&tr=two&x=a%20b%2f#part",
        "?xt=urn:btih:0123456789012345678901234567890123456789&tr=one&tr=two&x=a+b%2F#part",
        "?xt=urn:btih:0123456789012345678901234567890123456789&tr=one&tr=two&x=a+b%2f#other"})
  {
    SCOPED_TRACE(differentIdentity);
    EXPECT_FALSE(URIUtils::PathHasParent(child, "magnet:/dir/" + differentIdentity));
  }
}

TEST_F(TestURIUtilsPaths, MagnetMemberAncestryPreservesComponentPresence)
{
  for (const std::string suffix : {"", "?", "#", "?#", "?x=1&x=2+a%2f#part"})
  {
    SCOPED_TRACE(suffix);
    const std::string child = "magnet:/dir/file" + suffix;
    EXPECT_TRUE(URIUtils::PathHasParent(child, URIUtils::GetParentPath(child)));
    EXPECT_TRUE(URIUtils::PathHasParent(child, "magnet:/" + suffix));
  }
}

TEST_F(TestURIUtilsPaths, IdentifierPrefixesHaveNoAncestry)
{
  for (const std::string identifier :
       {"data:,body", "urn:example:body",
        "magnet:?xt=urn:btih:0123456789012345678901234567890123456789"})
  {
    SCOPED_TRACE(identifier);
    EXPECT_FALSE(URIUtils::PathHasParent(identifier + "/child", identifier));
    EXPECT_TRUE(URIUtils::PathHasParent(identifier, identifier));
  }
}

TEST_F(TestURIUtilsPaths, URIComponentsAreNotReparsedAsLocations)
{
  const std::string path = "http://host/notes:2026/movie.mkv";
  EXPECT_EQ("movie.mkv", URIUtils::GetFileName(path));
  EXPECT_EQ(".mkv", CURL{path}.GetExtension());
  EXPECT_EQ("http://host/notes:2026/movie.avi", URIUtils::ReplaceExtension(path, ".avi"));
  EXPECT_EQ("http://host/notes:2026/", URIUtils::GetParentPath(path));
  EXPECT_EQ("http://host/notes:2026/movie.mkv",
            URIUtils::AddFileToFolder("http://host/notes:2026", "movie.mkv"));
  EXPECT_EQ("b", URIUtils::GetFileName("/tmp/a://b"));
  EXPECT_EQ("notes:2026", URIUtils::GetFileName("./notes:2026"));
  EXPECT_EQ("file?#%+", URIUtils::GetFileName("file?#%+"));
}

TEST_F(TestURIUtilsPaths, NativeColonFilenameExtensions)
{
  for (const auto& [base, extension] : NativeColonFileNames)
  {
    const std::string path = std::string{base} + extension;
    SCOPED_TRACE(path);
    EXPECT_EQ(path, URIUtils::GetFileName(path));
    EXPECT_EQ(path, URIUtils::GetFileOrFolderName(path));
    EXPECT_EQ(extension, URIUtils::GetExtension(path));
    EXPECT_TRUE(URIUtils::HasExtension(path));
    EXPECT_TRUE(URIUtils::HasExtension(path, ".mkv|.mp3|.chd"));
    std::string withoutExtension = URIUtils::ReplaceExtension(path, ".srt");
    EXPECT_EQ(std::string{base} + ".srt", withoutExtension);
    URIUtils::RemoveExtension(withoutExtension);
    EXPECT_EQ(base, withoutExtension);
    EXPECT_TRUE(URIUtils::GetDirectory(path).empty());
    std::string directory;
    std::string filename;
    URIUtils::Split(path, directory, filename);
    EXPECT_TRUE(directory.empty());
    EXPECT_EQ(path, filename);
    EXPECT_EQ(path + "/child", URIUtils::AddFileToFolder(path, "child"));
    EXPECT_EQ(path + "/", URIUtils::GetParentPath(path + "/child"));
    std::string folder = path;
    URIUtils::AddSlashAtEnd(folder);
    EXPECT_EQ(path + "/", folder);
    EXPECT_TRUE(URIUtils::HasSlashAtEnd(folder, true));
    URIUtils::RemoveSlashAtEnd(folder);
    EXPECT_EQ(path, folder);
  }
}

TEST_F(TestURIUtilsPaths, NativeColonPathOperations)
{
  const std::string path = "Re:Zero/disc/movie.mkv";
  EXPECT_EQ("movie.mkv", URIUtils::GetFileName(path));
  EXPECT_EQ("Re:Zero/disc/", URIUtils::GetDirectory(path));
  EXPECT_EQ("Re:Zero/disc/", URIUtils::GetParentPath(path));
  EXPECT_EQ(path, URIUtils::AddFileToFolder("Re:Zero/disc", "movie.mkv"));
#ifdef TARGET_WINDOWS
  EXPECT_EQ((std::vector<std::string>{"Re:Zero", "disc", "movie.mkv"}),
            URIUtils::SplitPath("Re:Zero\\disc\\movie.mkv"));
#else
  EXPECT_EQ((std::vector<std::string>{"Re:Zero", "disc", "movie.mkv"}), URIUtils::SplitPath(path));
#endif
  std::string directory;
  std::string filename;
  URIUtils::Split(path, directory, filename);
  EXPECT_EQ("Re:Zero/disc/", directory);
  EXPECT_EQ("movie.mkv", filename);
  std::string folder = "Re:Zero/disc";
  URIUtils::AddSlashAtEnd(folder);
  EXPECT_EQ("Re:Zero/disc/", folder);
  EXPECT_TRUE(URIUtils::HasSlashAtEnd(folder, true));
  URIUtils::RemoveSlashAtEnd(folder);
  EXPECT_EQ("Re:Zero/disc", folder);
  EXPECT_EQ(path, URIUtils::CanonicalizePath("Re:Zero/disc/../disc/movie.mkv", '/'));
  EXPECT_EQ(path, URIUtils::FixSlashesAndDups("Re:Zero//disc//movie.mkv"));
#ifndef TARGET_WINDOWS
  EXPECT_EQ(path, CUtil::ValidatePath("Re:Zero//disc//movie.mkv", true));
#endif
}

TEST_F(TestURIUtilsPaths, NativeAndLegacyAncestryAndCommonPaths)
{
  for (const std::string root : AncestryRoots)
  {
    SCOPED_TRACE(root);
    const std::string separator = root.ends_with('\\') ? "\\" : "/";
    const std::string directory = root + "disc";
    const std::string child = directory + separator + "movie.mkv";
    EXPECT_TRUE(URIUtils::PathHasParent(child, directory));
    EXPECT_FALSE(URIUtils::PathHasParent(root + "disc2" + separator + "movie.mkv", directory));
    std::string common = child;
    URIUtils::GetCommonPath(common, directory + separator + "other.mkv");
    EXPECT_EQ(directory + separator, common);
  }
}

TEST_F(TestURIUtilsPaths, CommonMagnetPathPreservesSharedIdentity)
{
  const std::string identity =
      "?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&tr=one&tr=two#part";
  std::string parent = "magnet:/dir/a.mkv" + identity;
  URIUtils::GetCommonPath(parent, "magnet:/dir/b.mkv" + identity);
  EXPECT_EQ("magnet:/dir/" + identity, parent);
  URIUtils::GetCommonPath(parent,
                          "magnet:/dir/c.mkv?xt=urn:btih:1123456789abcdef0123456789abcdef01234567");
  EXPECT_TRUE(parent.empty());
}

TEST_F(TestURIUtilsPaths, RootlessURIAncestryAndCommonPaths)
{
  for (const std::string suffix : {"", "?x=%2f&x=+#part", "?#"})
  {
    SCOPED_TRACE(suffix);
    EXPECT_TRUE(URIUtils::PathHasParent("file:movie.mkv" + suffix, "file:" + suffix));
    EXPECT_FALSE(URIUtils::PathHasParent("file:/movie.mkv" + suffix, "file:" + suffix));
    std::string common = "file:movie.mkv" + suffix;
    URIUtils::GetCommonPath(common, "file:other.mkv" + suffix);
    EXPECT_EQ("file:" + suffix, common);
    URIUtils::GetCommonPath(common, "file:/other.mkv" + suffix);
    EXPECT_TRUE(common.empty());
  }
}

TEST_F(TestURIUtilsPaths, ValidatePath)
{
  for (const auto& [path, expected] : ValidatePathCases)
  {
    EXPECT_EQ(expected, CUtil::ValidatePath(path, true)) << path;
  }
}

TEST_F(TestURIUtilsPaths, CustomVFSAndPlaylistNavigation)
{
  for (const std::string scheme : {
#ifndef TARGET_WINDOWS
           "x",
#endif
           "custom+vfs", "playlistmusic", "playlistvideo"})
  {
    SCOPED_TRACE(scheme);
    const std::string root = scheme + "://host/";
    const std::string file = root + "dir/movie.mkv";
    EXPECT_EQ("movie.mkv", URIUtils::GetFileName(file));
    EXPECT_EQ(".mkv", URIUtils::GetExtension(file));
    EXPECT_EQ(root + "dir/", URIUtils::GetDirectory(file));
    EXPECT_EQ(root + "dir/", URIUtils::GetParentPath(file));
    EXPECT_EQ(file, URIUtils::AddFileToFolder(root + "dir", "movie.mkv"));
    EXPECT_TRUE(URIUtils::PathHasParent(file, root + "dir/"));
  }
}

TEST_F(TestURIUtilsPaths, LosslessNavigationRetainsEmptyDelimitersAndEscapes)
{
  for (const std::string suffix : {"?", "#", "?#", "?x=%2f&x=+&flag&&#%2F"})
  {
    const std::string root = "magnet:/notes:2026/%3F%23/";
    const std::string file = root + "movie.mkv" + suffix;
    EXPECT_EQ(root + suffix, URIUtils::GetParentPath(file));
    EXPECT_EQ(root + suffix, URIUtils::GetDirectory(file));
    EXPECT_EQ(file, URIUtils::AddFileToFolder(root + suffix, "movie.mkv"));
    EXPECT_EQ(root + "movie.avi" + suffix, URIUtils::ReplaceExtension(file, ".avi"));
    EXPECT_TRUE(URIUtils::PathHasParent(file, root + suffix));
    std::string common = file;
    URIUtils::GetCommonPath(common, root + "other.mkv" + suffix);
    EXPECT_EQ(root + suffix, common);
    URIUtils::GetCommonPath(common, "/notes:2026/%3F%23/movie.mkv");
    EXPECT_TRUE(common.empty());
  }
}
