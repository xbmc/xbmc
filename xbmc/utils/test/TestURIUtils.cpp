/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "URL.h"

#include "gtest/gtest.h"

class TestURIUtils : public testing::Test
{
protected:
  TestURIUtils(){}
  ~TestURIUtils()
  {
    g_advancedSettings.m_pathSubstitutions.clear();
  }
};

TEST_F(TestURIUtils, GetParentFolderURI)
{
  CStdString ref, var;

  ref = "/path/to/";
  var = URIUtils::GetParentFolderURI("/path/to/movie.avi", false);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "/path/to/movie.avi";
  var = URIUtils::GetParentFolderURI("/path/to/movie.avi", true);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, IsInPath)
{
  EXPECT_TRUE(URIUtils::IsInPath("/path/to/movie.avi", "/path/to/"));
  EXPECT_FALSE(URIUtils::IsInPath("/path/to/movie.avi", "/path/2/"));
}

TEST_F(TestURIUtils, GetDirectory)
{
  CStdString ref, var;

  ref = "/path/to/";
  URIUtils::GetDirectory("/path/to/movie.avi", var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, GetExtension)
{
  CStdString ref, var;

  ref = ".avi";
  EXPECT_STREQ(ref.c_str(),
               URIUtils::GetExtension("/path/to/movie.avi").c_str());

  var.clear();
  URIUtils::GetExtension("/path/to/movie.avi", var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, GetFileName)
{
  EXPECT_STREQ("movie.avi",
               URIUtils::GetFileName("/path/to/movie.avi").c_str());
}

TEST_F(TestURIUtils, RemoveExtension)
{
  CStdString ref, var;

  /* NOTE: g_settings need to be set to find other extensions. */
  ref = "/path/to/file";
  var = "/path/to/file.xml";
  URIUtils::RemoveExtension(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, ReplaceExtension)
{
  CStdString ref, var;

  ref = "/path/to/file.xsd";
  var = URIUtils::ReplaceExtension("/path/to/file.xml", ".xsd");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, Split)
{
  CStdString refpath, reffile, varpath, varfile;

  refpath = "/path/to/";
  reffile = "movie.avi";
  URIUtils::Split("/path/to/movie.avi", varpath, varfile);
  EXPECT_STREQ(refpath.c_str(), varpath.c_str());
  EXPECT_STREQ(reffile.c_str(), varfile.c_str());
}

TEST_F(TestURIUtils, SplitPath)
{
  CStdStringArray strarray;

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
  CStdStringArray strarray;

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
  CStdString ref, var;

  ref = "/path/";
  var = "/path/2/movie.avi";
  URIUtils::GetCommonPath(var, "/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, GetParentPath)
{
  CStdString ref, var;

  ref = "/path/to/";
  var = URIUtils::GetParentPath("/path/to/movie.avi");
  EXPECT_STREQ(ref.c_str(), var.c_str());

  var.clear();
  EXPECT_TRUE(URIUtils::GetParentPath("/path/to/movie.avi", var));
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, SubstitutePath)
{
  CStdString from, to, ref, var;

  from = "/somepath";
  to = "/someotherpath";
  g_advancedSettings.m_pathSubstitutions.push_back(std::make_pair(from, to));

  ref = "/someotherpath/to/movie.avi";
  var = URIUtils::SubstitutePath("/somepath/to/movie.avi");
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

TEST_F(TestURIUtils, IsDAAP)
{
  EXPECT_TRUE(URIUtils::IsDAAP("daap://path/to/daap"));
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

TEST_F(TestURIUtils, IsHTSP)
{
  EXPECT_TRUE(URIUtils::IsHTSP("htsp://path/to/file"));
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

TEST_F(TestURIUtils, IsLastFM)
{
  EXPECT_TRUE(URIUtils::IsLastFM("lastfm://path/to/file"));
}

TEST_F(TestURIUtils, IsLiveTV)
{
  EXPECT_TRUE(URIUtils::IsLiveTV("tuxbox://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("vtp://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("hdhomerun://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("sling://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("htsp://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("sap://path/to/file"));
  EXPECT_TRUE(URIUtils::IsLiveTV("myth://path/channels/"));
}

TEST_F(TestURIUtils, IsMultiPath)
{
  EXPECT_TRUE(URIUtils::IsMultiPath("multipath://path/to/file"));
}

TEST_F(TestURIUtils, IsMusicDb)
{
  EXPECT_TRUE(URIUtils::IsMusicDb("musicdb://path/to/file"));
}

TEST_F(TestURIUtils, IsMythTV)
{
  EXPECT_TRUE(URIUtils::IsMythTV("myth://path/to/file"));
}

TEST_F(TestURIUtils, IsNfs)
{
  EXPECT_TRUE(URIUtils::IsNfs("nfs://path/to/file"));
  EXPECT_TRUE(URIUtils::IsNfs("stack://nfs://path/to/file"));
}

TEST_F(TestURIUtils, IsAfp)
{
  EXPECT_TRUE(URIUtils::IsAfp("afp://path/to/file"));
  EXPECT_TRUE(URIUtils::IsAfp("stack://afp://path/to/file"));
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
  EXPECT_TRUE(URIUtils::IsOnLAN("multipath://daap://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("stack://daap://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("daap://path/to/file"));
  EXPECT_FALSE(URIUtils::IsOnLAN("plugin://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("tuxbox://path/to/file"));
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

TEST_F(TestURIUtils, IsTuxBox)
{
  EXPECT_TRUE(URIUtils::IsTuxBox("tuxbox://path/to/file"));
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

TEST_F(TestURIUtils, IsVTP)
{
  EXPECT_TRUE(URIUtils::IsVTP("vtp://path/to/file"));
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
  CStdString ref, var;

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
  CStdString ref, var;

  ref = "bluray://path/to/file";
  var = "bluray://path/to/file/";
  URIUtils::RemoveSlashAtEnd(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, CreateArchivePath)
{
  CStdString ref, var;

  ref = "file://%2fpath%2fto%2f/file";
  URIUtils::CreateArchivePath(var, "file", "/path/to/", "file");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, AddFileToFolder)
{
  CStdString ref, var;

  ref = "/path/to/file";
  URIUtils::AddFileToFolder("/path/to", "file", var);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  var.clear();
  var = URIUtils::AddFileToFolder("/path/to", "file");
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST_F(TestURIUtils, ProtocolHasParentInHostname)
{
  EXPECT_TRUE(URIUtils::ProtocolHasParentInHostname("zip"));
  EXPECT_TRUE(URIUtils::ProtocolHasParentInHostname("rar"));
  EXPECT_TRUE(URIUtils::ProtocolHasParentInHostname("bluray"));
}

TEST_F(TestURIUtils, ProtocolHasEncodedHostname)
{
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedHostname("zip"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedHostname("rar"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedHostname("bluray"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedHostname("musicsearch"));
}

TEST_F(TestURIUtils, ProtocolHasEncodedFilename)
{
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("shout"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("daap"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("dav"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("tuxbox"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("lastfm"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("rss"));
  EXPECT_TRUE(URIUtils::ProtocolHasEncodedFilename("davs"));
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
