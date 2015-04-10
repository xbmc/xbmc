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

#include "Util.h"

#include "gtest/gtest.h"

TEST(TestUtil, GetQualifiedFilename)
{
  std::string file = "../foo";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "foo");
  file = "C:\\foo\\bar";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "C:\\foo\\bar");
  file = "../foo/./bar";
  CUtil::GetQualifiedFilename("smb://my/path", file);
  EXPECT_EQ(file, "smb://my/foo/bar");
  file = "smb://foo/bar/";
  CUtil::GetQualifiedFilename("upnp://", file);
  EXPECT_EQ(file, "smb://foo/bar/");
}

TEST(TestUtil, MakeLegalPath)
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = "C:\\foo\\bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo\\bar");
  path = "C:\\foo:\\bar\\";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo_\\bar\\");
#else
  path = "/foo/bar/";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo/bar/");
  path = "/foo?/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo_/bar");
#endif
  path = "smb://foo/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar");
  path = "smb://foo/bar?/";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar_/");
}

TEST(TestUtil, SplitExec)
{
  std::string function;
  std::vector<std::string> params;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo\\");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\\\test\\\\foo\\");
  params.clear();
  CUtil::SplitExecFunction("SetProperty(Foo,\"\")", function, params);
  EXPECT_EQ(function,  "SetProperty");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "Foo");
  EXPECT_EQ(params[1], "");
  params.clear();
  CUtil::SplitExecFunction("SetProperty(foo,ba(\"ba black )\",sheep))", function, params);
  EXPECT_EQ(function,  "SetProperty");
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0], "foo");
  EXPECT_EQ(params[1], "ba(\"ba black )\",sheep)");
}
