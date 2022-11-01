/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "utils/ExecString.h"

#include <gtest/gtest.h>

TEST(TestExecString, ctor_1)
{
  {
    const CExecString exec("ActivateWindow(Video, \"C:\\test\\foo\")");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "C:\\test\\foo");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video, \"C:\\test\\foo\")");
  }
  {
    const CExecString exec("ActivateWindow(Video, \"C:\\test\\foo\\\")");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "C:\\test\\foo");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video, \"C:\\test\\foo\\\")");
  }
  {
    const CExecString exec("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "C:\\test\\foo\\");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")");
  }
  {
    const CExecString exec("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "C:\\\\test\\\\foo\\");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")");
  }
  {
    const CExecString exec("SetProperty(Foo,\"\")");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "setproperty");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Foo");
    EXPECT_EQ(exec.GetParams()[1], "");
    EXPECT_EQ(exec.GetExecString(), "SetProperty(Foo,\"\")");
  }
  {
    const CExecString exec("SetProperty(foo,ba(\"ba black )\",sheep))");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "setproperty");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "foo");
    EXPECT_EQ(exec.GetParams()[1], "ba(\"ba black )\",sheep)");
    EXPECT_EQ(exec.GetExecString(), "SetProperty(foo,ba(\"ba black )\",sheep))");
  }
}

TEST(TestExecString, ctor_2)
{
  {
    const CExecString exec("ActivateWindow", {"Video", "C:\\test\\foo"});
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 2U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "C:\\test\\foo");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video,C:\\test\\foo)");
  }
}

TEST(TestExecString, ctor_3)
{
  {
    const CFileItem item("C:\\test\\foo", true);
    const CExecString exec(item, "Video");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 3U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "\"C:\\\\test\\\\foo\\\\\"");
    EXPECT_EQ(exec.GetParams()[2], "return");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video,\"C:\\\\test\\\\foo\\\\\",return)");
  }
  {
    const CFileItem item("C:\\test\\foo\\", true);
    const CExecString exec(item, "Video");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "activatewindow");
    EXPECT_EQ(exec.GetParams().size(), 3U);
    EXPECT_EQ(exec.GetParams()[0], "Video");
    EXPECT_EQ(exec.GetParams()[1], "\"C:\\\\test\\\\foo\\\\\"");
    EXPECT_EQ(exec.GetParams()[2], "return");
    EXPECT_EQ(exec.GetExecString(), "ActivateWindow(Video,\"C:\\\\test\\\\foo\\\\\",return)");
  }
  {
    const CFileItem item("C:\\test\\foo", false);
    const CExecString exec(item, "Video");
    EXPECT_EQ(exec.IsValid(), true);
    EXPECT_EQ(exec.GetFunction(), "playmedia");
    EXPECT_EQ(exec.GetParams().size(), 1U);
    EXPECT_EQ(exec.GetParams()[0], "\"C:\\\\test\\\\foo\"");
    EXPECT_EQ(exec.GetExecString(), "PlayMedia(\"C:\\\\test\\\\foo\")");
  }
}
