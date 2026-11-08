/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "guilib/GUIControlProfiler.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <string>

#include <gtest/gtest.h>

TEST(TestGUIControlProfiler, SaveResults)
{
  XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
  ASSERT_NE(nullptr, file);
  file->Close();
  const std::string path = XBMC_TEMPFILEPATH(file);

  CGUIControlProfiler& profiler = CGUIControlProfiler::Instance();
  const int maxFrameCount = profiler.GetMaxFrameCount();
  profiler.SetOutputFile(path);
  profiler.SetMaxFrameCount(1);
  profiler.Start();
  profiler.EndFrame();
  profiler.SetMaxFrameCount(maxFrameCount);
  profiler.SetOutputFile("");
  EXPECT_FALSE(CGUIControlProfiler::IsRunning());

  CXBMCTinyXML2 doc;
  const bool loaded = doc.LoadFile(path);
  XBMC_DELETETEMPFILE(file);
  ASSERT_TRUE(loaded);

  ASSERT_NE(nullptr, doc.FirstChild());
  EXPECT_NE(nullptr, doc.FirstChild()->ToDeclaration());

  const auto* root = doc.RootElement();
  ASSERT_NE(nullptr, root);
  EXPECT_STREQ("guicontrolprofiler", root->Value());
  EXPECT_STREQ("1", root->Attribute("framecount"));
  EXPECT_STREQ("ms", root->Attribute("timeunit"));

  const auto* control = root->FirstChildElement("control");
  ASSERT_NE(nullptr, control);
  EXPECT_EQ(nullptr, control->FirstAttribute());
  EXPECT_EQ(nullptr, control->FirstChild());
  EXPECT_EQ(nullptr, control->NextSibling());
}
