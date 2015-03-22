/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "filesystem/MultiPathDirectory.h"
#include "URL.h"

#include "gtest/gtest.h"

using namespace XFILE;

class TestURIUtils : public testing::Test
{
protected:
  TestURIUtils(){}
  ~TestURIUtils()
  {
    g_advancedSettings.m_pathSubstitutions.clear();
  }
};

TEST_F(TestURIUtils, IsInPath)
{
  EXPECT_TRUE(URIUtils::IsInPath("/path/to/movie.avi", "/path/to/"));
  EXPECT_FALSE(URIUtils::IsInPath("/path/to/movie.avi", "/path/2/"));
}

TEST_F(TestURIUtils, GetDirectory)
{
  EXPECT_STREQ("/path/to/", URIUtils::GetDirectory("/path/to/movie.avi").c_str());
  EXPECT_STREQ("/path/to/", URIUtils::GetDirectory("/path/to/").c_str());
  EXPECT_STREQ("/path/to/|option=foo", URIUtils::GetDirectory("/path/to/movie.avi|option=foo").c_str());
  EXPECT_STREQ("/path/to/|option=foo", URIUtils::GetDirectory("/path/to/|option=foo").c_str());
  EXPECT_STREQ("", URIUtils::GetDirectory("movie.avi").c_str());
  EXPECT_STREQ("", URIUtils::GetDirectory("movie.avi|option=foo").c_str());
  EXPECT_STREQ("", URIUtils::GetDirectory("").c_str());

  // Make sure it works when assigning to the same str as the reference parameter
  std::string var = "/path/to/movie.avi|option=foo";
  var = URIUtils::GetDirectory(var);
  EXPECT_STREQ("/path/to/|option=foo", var.c_str());
}

TEST_F(TestURIUtils, GetExtension)
{
  EXPECT_STREQ(".avi",
               URIUtils::GetExtension("/path/to/movie.avi").c_str());
}

TEST_F(TestURIUtils, HasExtension)
{
  EXPECT_TRUE (URIUtils::HasExtension("/path/to/movie.AvI"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/.to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension(""));

  EXPECT_TRUE (URIUtils::HasExtension("/path/to/movie.AvI", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/to/movie.AvI", ".mkv"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/.avi/movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi"));

  EXPECT_TRUE (URIUtils::HasExtension("/path/movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE (URIUtils::HasExtension("/path/movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("/path.mkv/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi|.mkv|.mp4"));
}

TEST_F(TestURIUtils, GetFileName)
{
  EXPECT_STREQ("movie.avi",
               URIUtils::GetFileName("/path/to/movie.avi").c_str());
}

TEST_F(TestURIUtils, RemoveExtension)
{
  std::string ref, var;

  /* NOTE: CSettings need to be set to find other extensions. */
  ref = "/path/to/file";
  var = "/path/to/file.xml";
  URIUtils::RemoveExtension(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, ReplaceExtension)
{
  std::string ref, var;

  ref = "/path/to/file.xsd";
  var = URIUtils::ReplaceExtension("/path/to/file.xml", ".xsd");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, Split)
{
  std::string refpath, reffile, varpath, varfile;

  refpath = "/path/to/";
  reffile = "movie.avi";
  URIUtils::Split("/path/to/movie.avi", varpath, varfile);
  EXPECT_STREQ(refpath.c_str(), varpath.c_str());
  EXPECT_STREQ(reffile.c_str(), varfile.c_str());
}

TEST_F(TestURIUtils, SplitPath)
{
  std::vector<std::string> strarray;

  strarray = URIUtils::SplitPath("http://www.test.com/path/to/movie.avi");

  EXPECT_STREQ("http://www.test.com/", strarray.at(0).c_str());
  EXPECT_STREQ("path", strarray.at(1).c_str());
  EXPECT_STREQ("to", strarray.at(2).c_str());
  EXPECT_STREQ("movie.avi", strarray.at(3).c_str());
}

TEST_F(TestURIUtils, SplitPathLocal)
{
#ifndef TARGET_LINUX
  const char *path = "C:\\path\\to\\movie.avi";
#else
  const char *path = "/path/to/movie.avi";
#endif
  std::vector<std::string> strarray;

  strarray = URIUtils::SplitPath(path);

#ifndef TARGET_LINUX
  EXPECT_STREQ("C:", strarray.at(0).c_str());
#else
  EXPECT_STREQ("", strarray.at(0).c_str());
#endif
  EXPECT_STREQ("path", strarray.at(1).c_str());
  EXPECT_STREQ("to", strarray.at(2).c_str());
  EXPECT_STREQ("movie.avi", strarray.at(3).c_str());
}

TEST_F(TestURIUtils, GetCommonPath)
{
  std::string ref, var;

  ref = "/path/";
  var = "/path/2/movie.avi";
  URIUtils::GetCommonPath(var, "/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, GetParentPath)
{
  std::string ref, var;

  ref = "/path/to/";
  var = URIUtils::GetParentPath("/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  var.clear();
  EXPECT_TRUE(URIUtils::GetParentPath("/path/to/movie.avi", var));
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, SubstitutePath)
{
  std::string from, to, ref, var;

  from = "C:\\My Videos";
  to = "https://myserver/some%20other%20path";
  g_advancedSettings.m_pathSubstitutions.push_back(std::make_pair(from, to));

  from = "/this/path1";
  to = "/some/other/path2";
  g_advancedSettings.m_pathSubstitutions.push_back(std::make_pair(from, to));

  from = "davs://otherserver/my%20music%20path";
  to = "D:\\Local Music\\MP3 Collection";
  g_advancedSettings.m_pathSubstitutions.push_back(std::make_pair(from, to));

  ref = "https://myserver/some%20other%20path/sub%20dir/movie%20name.avi";
  var = URIUtils::SubstitutePath("C:\\My Videos\\sub dir\\movie name.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "C:\\My Videos\\sub dir\\movie name.avi";
  var = URIUtils::SubstitutePath("https://myserver/some%20other%20path/sub%20dir/movie%20name.avi", true);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3";
  var = URIUtils::SubstitutePath("davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3";
  var = URIUtils::SubstitutePath("D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3", true);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "/some/other/path2/to/movie.avi";
  var = URIUtils::SubstitutePath("/this/path1/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "/this/path1/to/movie.avi";
  var = URIUtils::SubstitutePath("/some/other/path2/to/movie.avi", true);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "/no/translation path/";
  var = URIUtils::SubstitutePath(ref);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "/no/translation path/";
  var = URIUtils::SubstitutePath(ref, true);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "c:\\no\\translation path";
  var = URIUtils::SubstitutePath(ref);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "c:\\no\\translation path";
  var = URIUtils::SubstitutePath(ref, true);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, IsAddonsPath)
{
  EXPECT_TRUE(URIUtils::IsAddonsPath("addons://path/to/addons"));
}

TEST_F(TestURIUtils, IsSourcesPath)
{
  EXPECT_TRUE(URIUtils::IsSourcesPath("sources://path/to/sources"));
}

TEST_F(TestURIUtils, IsCDDA)
{
  EXPECT_TRUE(URIUtils::IsCDDA("cdda://path/to/cdda"));
}

TEST_F(TestURIUtils, IsDOSPath)
{
  EXPECT_TRUE(URIUtils::IsDOSPath("C://path/to/dosfile"));
}

TEST_F(TestURIUtils, IsDVD)
{
  EXPECT_TRUE(URIUtils::IsDVD("dvd://path/in/video_ts.ifo"));
#if defined(TARGET_WINDOWS)
  EXPECT_TRUE(URIUtils::IsDVD("dvd://path/in/file"));
#else
  EXPECT_TRUE(URIUtils::IsDVD("iso9660://path/in/video_ts.ifo"));
  EXPECT_TRUE(URIUtils::IsDVD("udf://path/in/video_ts.ifo"));
  EXPECT_TRUE(URIUtils::IsDVD("dvd://1"));
#endif
}

TEST_F(TestURIUtils, IsFTP)
{
  EXPECT_TRUE(URIUtils::IsFTP("ftp://path/in/ftp"));
}

TEST_F(TestURIUtils, IsHD)
{
  EXPECT_TRUE(URIUtils::IsHD("/path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("file:///path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("special://path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("stack://path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("zip://path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsHDHomeRun)
{
  EXPECT_TRUE(URIUtils::IsHDHomeRun("hdhomerun://path/to/file"));
}

TEST_F(TestURIUtils, IsSlingbox)
{
  EXPECT_TRUE(URIUtils::IsSlingbox("sling://path/to/file"));
}

TEST_F(TestURIUtils, IsInArchive)
{
  EXPECT_TRUE(URIUtils::IsInArchive("zip://path/to/file"));
  EXPECT_TRUE(URIUtils::IsInArchive("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsInRAR)
{
  EXPECT_TRUE(URIUtils::IsInRAR("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsInternetStream)
{
  CURL url1("http://path/to/file");
  CURL url2("https://path/to/file");
  EXPECT_TRUE(URIUtils::IsInternetStream(url1));
  EXPECT_TRUE(URIUtils::IsInternetStream(url2));
}

TEST_F(TestURIUtils, IsInZIP)
{
  EXPECT_TRUE(URIUtils::IsInZIP("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsISO9660)
{
  EXPECT_TRUE(URIUtils::IsISO9660("iso9660://path/to/file"));
}

TEST_F(TestURIUtils, IsLiveTV)
{
  EXPECT_TRUE(URIUtils::IsLiveTV("hdhomerun://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("sling://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("sap://path/to/file"));
}

TEST_F(TestURIUtils, IsMultiPath)
{
  EXPECT_TRUE(URIUtils::IsMultiPath("multipath://path/to/file"));
}

TEST_F(TestURIUtils, IsMusicDb)
{
  EXPECT_TRUE(URIUtils::IsMusicDb("musicdb://path/to/file"));
}

TEST_F(TestURIUtils, IsNfs)
{
  EXPECT_TRUE(URIUtils::IsNfs("nfs://path/to/file"));
  EXPECT_TRUE(URIUtils::IsNfs("stack://nfs://path/to/file"));
}

TEST_F(TestURIUtils, IsOnDVD)
{
  EXPECT_TRUE(URIUtils::IsOnDVD("dvd://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("udf://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("iso9660://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("cdda://path/to/file"));
}

TEST_F(TestURIUtils, IsOnLAN)
{
  std::vector<std::string> multiVec;
  multiVec.push_back("smb://path/to/file");
  EXPECT_TRUE(URIUtils::IsOnLAN(CMultiPathDirectory::ConstructMultiPath(multiVec)));
  EXPECT_TRUE(URIUtils::IsOnLAN("stack://smb://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("smb://path/to/file"));
  EXPECT_FALSE(URIUtils::IsOnLAN("plugin://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("upnp://path/to/file"));
}

TEST_F(TestURIUtils, IsPlugin)
{
  EXPECT_TRUE(URIUtils::IsPlugin("plugin://path/to/file"));
}

TEST_F(TestURIUtils, IsScript)
{
  EXPECT_TRUE(URIUtils::IsScript("script://path/to/file"));
}

TEST_F(TestURIUtils, IsRAR)
{
  EXPECT_TRUE(URIUtils::IsRAR("/path/to/rarfile.rar"));
  EXPECT_TRUE(URIUtils::IsRAR("/path/to/rarfile.cbr"));
  EXPECT_FALSE(URIUtils::IsRAR("/path/to/file"));
  EXPECT_FALSE(URIUtils::IsRAR("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsRemote)
{
  EXPECT_TRUE(URIUtils::IsRemote("http://path/to/file"));
  EXPECT_TRUE(URIUtils::IsRemote("https://path/to/file"));
}

TEST_F(TestURIUtils, IsSmb)
{
  EXPECT_TRUE(URIUtils::IsSmb("smb://path/to/file"));
  EXPECT_TRUE(URIUtils::IsSmb("stack://smb://path/to/file"));
}

TEST_F(TestURIUtils, IsSpecial)
{
  EXPECT_TRUE(URIUtils::IsSpecial("special://path/to/file"));
  EXPECT_TRUE(URIUtils::IsSpecial("stack://special://path/to/file"));
}

TEST_F(TestURIUtils, IsStack)
{
  EXPECT_TRUE(URIUtils::IsStack("stack://path/to/file"));
}

TEST_F(TestURIUtils, IsUPnP)
{
  EXPECT_TRUE(URIUtils::IsUPnP("upnp://path/to/file"));
}

TEST_F(TestURIUtils, IsURL)
{
  EXPECT_TRUE(URIUtils::IsURL("someprotocol://path/to/file"));
  EXPECT_FALSE(URIUtils::IsURL("/path/to/file"));
}

TEST_F(TestURIUtils, IsVideoDb)
{
  EXPECT_TRUE(URIUtils::IsVideoDb("videodb://path/to/file"));
}

TEST_F(TestURIUtils, IsZIP)
{
  EXPECT_TRUE(URIUtils::IsZIP("/path/to/zipfile.zip"));
  EXPECT_TRUE(URIUtils::IsZIP("/path/to/zipfile.cbz"));
  EXPECT_FALSE(URIUtils::IsZIP("/path/to/file"));
  EXPECT_FALSE(URIUtils::IsZIP("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsBluray)
{
  EXPECT_TRUE(URIUtils::IsBluray("bluray://path/to/file"));
}

TEST_F(TestURIUtils, AddSlashAtEnd)
{
  std::string ref, var;

  ref = "bluray://path/to/file/";
  var = "bluray://path/to/file/";
  URIUtils::AddSlashAtEnd(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, HasSlashAtEnd)
{
  EXPECT_TRUE(URIUtils::HasSlashAtEnd("bluray://path/to/file/"));
  EXPECT_FALSE(URIUtils::HasSlashAtEnd("bluray://path/to/file"));
}

TEST_F(TestURIUtils, RemoveSlashAtEnd)
{
  std::string ref, var;

  ref = "bluray://path/to/file";
  var = "bluray://path/to/file/";
  URIUtils::RemoveSlashAtEnd(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, CreateArchivePath)
{
  std::string ref, var;

  ref = "zip://%2fpath%2fto%2f/file";
  var = URIUtils::CreateArchivePath("zip", CURL("/path/to/"), "file").Get();
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, AddFileToFolder)
{
  std::string ref = "/path/to/file";
  std::string var = URIUtils::AddFileToFolder("/path/to", "file");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, HasParentInHostname)
{
  EXPECT_TRUE(URIUtils::HasParentInHostname(CURL("zip://")));
  EXPECT_TRUE(URIUtils::HasParentInHostname(CURL("rar://")));
  EXPECT_TRUE(URIUtils::HasParentInHostname(CURL("bluray://")));
}

TEST_F(TestURIUtils, HasEncodedHostname)
{
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("zip://")));
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("rar://")));
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("bluray://")));
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("musicsearch://")));
}

TEST_F(TestURIUtils, HasEncodedFilename)
{
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("shout://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("dav://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("rss://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("davs://")));
}

TEST_F(TestURIUtils, GetRealPath)
{
  std::string ref;
  
  ref = "/path/to/file/";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());
  
  ref = "path/to/file";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("../path/to/file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("./path/to/file").c_str());

  ref = "/path/to/file";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("/path/to/./file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("/./path/to/./file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("/path/to/some/../file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("/../path/to/some/../file").c_str());

  ref = "/path/to";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("/path/to/some/../file/..").c_str());

#ifdef TARGET_WINDOWS
  ref = "\\\\path\\to\\file\\";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());
  
  ref = "path\\to\\file";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("..\\path\\to\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath(".\\path\\to\\file").c_str());

  ref = "\\\\path\\to\\file";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("\\\\path\\to\\.\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("\\\\.\\path/to\\.\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("\\\\path\\to\\some\\..\\file").c_str());
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("\\\\..\\path\\to\\some\\..\\file").c_str());

  ref = "\\\\path\\to";
  EXPECT_STREQ(ref.c_str(), URIUtils::GetRealPath("\\\\path\\to\\some\\..\\file\\..").c_str());
#endif

  // test rar/zip paths
  ref = "rar://%2fpath%2fto%2frar/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());
  
  // test rar/zip paths
  ref = "rar://%2fpath%2fto%2frar/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2frar/../subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2frar/./subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2frar/subpath/to/./file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2frar/subpath/to/some/../file").c_str());
  
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2f.%2frar/subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("rar://%2fpath%2fto%2fsome%2f..%2frar/subpath/to/file").c_str());

  // test rar/zip path in rar/zip path
  ref ="zip://rar%3A%2F%2F%252Fpath%252Fto%252Frar%2Fpath%2Fto%2Fzip/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://rar%3A%2F%2F%252Fpath%252Fto%252Fsome%252F..%252Frar%2Fpath%2Fto%2Fsome%2F..%2Fzip/subpath/to/some/../file").c_str());
}

TEST_F(TestURIUtils, UpdateUrlEncoding)
{
  std::string oldUrl = "stack://rar://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD1%2erar/video.avi , rar://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD2%2erar/video.avi";
  std::string newUrl = "stack://rar://%2fpath%2fto%2farchive%2fsome-archive-file.CD1.rar/video.avi , rar://%2fpath%2fto%2farchive%2fsome-archive-file.CD2.rar/video.avi";

  EXPECT_TRUE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());

  oldUrl = "rar://%2fpath%2fto%2farchive%2fsome%2darchive%2efile%2erar/video.avi";
  newUrl = "rar://%2fpath%2fto%2farchive%2fsome-archive.file.rar/video.avi";
  
  EXPECT_TRUE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
  
  oldUrl = "/path/to/some/long%2dnamed%2efile";
  newUrl = "/path/to/some/long%2dnamed%2efile";
  
  EXPECT_FALSE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
  
  oldUrl = "/path/to/some/long-named.file";
  newUrl = "/path/to/some/long-named.file";
  
  EXPECT_FALSE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
}
