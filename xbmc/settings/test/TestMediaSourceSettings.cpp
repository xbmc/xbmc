/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSource.h"
#include "filesystem/File.h"
#include "settings/MediaSourceSettings.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <gtest/gtest.h>

TEST(TestMediaSourceSettings, LoadString)
{
  CMediaSourceSettings& ms = CMediaSourceSettings::GetInstance();
  EXPECT_TRUE(ms.Load(XBMC_REF_FILE_PATH("/xbmc/settings/test/test-MediaSources.xml")));

  EXPECT_EQ(ms.GetSources("programs")->size(), 0);
  EXPECT_EQ(ms.GetSources("files")->size(), 0);
  EXPECT_EQ(ms.GetSources("music")->size(), 2);
  EXPECT_EQ(ms.GetSources("video")->size(), 4);
  EXPECT_EQ(ms.GetSources("pictures")->size(), 1);
  EXPECT_EQ(ms.GetSources("games")->size(), 0);
}

TEST(TestMediaSourceSettings, SaveString)
{
  CMediaSourceSettings& ms = CMediaSourceSettings::GetInstance();
  EXPECT_TRUE(ms.Load(XBMC_REF_FILE_PATH("/xbmc/settings/test/test-MediaSources.xml")));

  int refprograms = ms.GetSources("programs")->size();
  int reffiles = ms.GetSources("files")->size();
  int refmusic = ms.GetSources("music")->size();
  int refvideo = ms.GetSources("video")->size();
  int refpictures = ms.GetSources("pictures")->size();
  int refgames = ms.GetSources("games")->size();

  XFILE::CFile* file;
  file = XBMC_CREATETEMPFILE(".xml");
  std::string xmlfile = XBMC_TEMPFILEPATH(file);
  std::cout << "Reference file generated at '" << XBMC_TEMPFILEPATH(file) << "'" << std::endl;
  file->Close();

  EXPECT_TRUE(ms.Save(xmlfile));
  ms.Clear();
  EXPECT_TRUE(ms.Load(xmlfile));
  auto progsources = ms.GetSources("programs");
  auto progsources2 = ms.GetSources("myprograms");
  EXPECT_TRUE(progsources == progsources2);
  EXPECT_EQ(progsources->size(), refprograms);
  auto filessources = ms.GetSources("files");
  EXPECT_EQ(filessources->size(), reffiles);
  auto musicsources = ms.GetSources("music");
  EXPECT_EQ(musicsources->size(), refmusic);
  auto videosources = ms.GetSources("video");
  auto videosources2 = ms.GetSources("videos");
  EXPECT_TRUE(videosources == videosources2);
  EXPECT_EQ(videosources->size(), refvideo);
  auto picturessources = ms.GetSources("pictures");
  EXPECT_EQ(picturessources->size(), refpictures);
  auto gamessources = ms.GetSources("games");
  EXPECT_EQ(gamessources->size(), refgames);
}

TEST(TestMediaSourceSettings, GetSource)
{
  // ToDo: implement test
  //  Test Malformed source
  //  Test missing name/path element
}

TEST(TestMediaSourceSettings, SetSources)
{
  // ToDo: implement test
}
