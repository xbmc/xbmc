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

#include <string>
#include <vector>

#include <gtest/gtest.h>

using KODI::MEDIA::MediaSection;

namespace
{
class TestMediaSourceSettingsDevicePath : public testing::Test, protected CMediaSourceSettings
{
};

class TestMediaSourceSettingsSections : public testing::Test, protected CMediaSourceSettings
{
};
} // namespace

TEST_F(TestMediaSourceSettingsSections, VideoAndGamesHaveNoDefaultSource)
{
  for (const MediaSection section : KODI::MEDIA::MEDIA_SECTIONS)
    SetDefaultSource(section, "smb://server/share/");

  EXPECT_EQ(GetDefaultSource(MediaSection::VIDEO), "");
  EXPECT_EQ(GetDefaultSource(MediaSection::GAMES), "");
  EXPECT_EQ(GetDefaultSource(MediaSection::MUSIC), "smb://server/share/");
  EXPECT_EQ(GetDefaultSource(MediaSection::PICTURES), "smb://server/share/");
  EXPECT_EQ(GetDefaultSource(MediaSection::FILES), "smb://server/share/");
  EXPECT_EQ(GetDefaultSource(MediaSection::PROGRAMS), "smb://server/share/");
}

TEST_F(TestMediaSourceSettingsSections, ClearForgetsTheDefaultSources)
{
  SetDefaultSource(MediaSection::MUSIC, "smb://server/music/");
  Clear();

  EXPECT_EQ(GetDefaultSource(MediaSection::MUSIC), "");
}

TEST_F(TestMediaSourceSettingsSections, SavesEverySectionAndReloadsItsDefault)
{
  SetDefaultSource(MediaSection::MUSIC, "smb://server/music/");
  SetDefaultSource(MediaSection::PICTURES, "smb://server/pictures/");
  SetDefaultSource(MediaSection::FILES, "smb://server/files/");
  SetDefaultSource(MediaSection::PROGRAMS, "smb://server/programs/");

  XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
  ASSERT_NE(file, nullptr);
  const std::string xmlfile = XBMC_TEMPFILEPATH(file);
  file->Close();

  ASSERT_TRUE(Save(xmlfile));

  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.LoadFile(xmlfile));
  std::vector<std::string> sections;
  for (const tinyxml2::XMLElement* element = doc.RootElement()->FirstChildElement(); element;
       element = element->NextSiblingElement())
    sections.emplace_back(element->Value());
  EXPECT_EQ(sections,
            std::vector<std::string>({"programs", "video", "music", "pictures", "files", "games"}));

  Clear();
  EXPECT_TRUE(Load(xmlfile));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));

  EXPECT_EQ(GetDefaultSource(MediaSection::MUSIC), "smb://server/music/");
  EXPECT_EQ(GetDefaultSource(MediaSection::PICTURES), "smb://server/pictures/");
  EXPECT_EQ(GetDefaultSource(MediaSection::FILES), "smb://server/files/");
  EXPECT_EQ(GetDefaultSource(MediaSection::PROGRAMS), "smb://server/programs/");
}

TEST_F(TestMediaSourceSettingsDevicePath, UpdateSourcePath)
{
  CMediaSource source;
  source.FromNameAndPaths("Disc", {"D:\\"});
  auto& sources = GetSources(MediaSection::VIDEO);
  sources.push_back(source);

  ASSERT_TRUE(UpdateSource(MediaSection::VIDEO, "Disc", "path", "E:\\"));
#ifdef TARGET_WINDOWS
  EXPECT_EQ(sources.front().strDevicePath, "E:");
#else
  EXPECT_TRUE(sources.front().strDevicePath.empty());
#endif

  ASSERT_TRUE(UpdateSource(MediaSection::VIDEO, "Disc", "path", "smb://server/share/"));
  EXPECT_TRUE(sources.front().strDevicePath.empty());
  EXPECT_EQ(sources.front().m_iDriveType, SourceType::REMOTE);
}

TEST_F(TestMediaSourceSettingsDevicePath, ReloadDriveSource)
{
  CMediaSource source;
  source.FromNameAndPaths("Disc", {"E:\\"});
  GetSources(MediaSection::VIDEO).push_back(source);

  XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
  ASSERT_NE(file, nullptr);
  const std::string xmlfile = XBMC_TEMPFILEPATH(file);
  file->Close();

  EXPECT_TRUE(Save(xmlfile));
  Clear();
  EXPECT_TRUE(Load(xmlfile));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));

  const auto& sources = GetSources(MediaSection::VIDEO);
  ASSERT_EQ(sources.size(), 1);
#ifdef TARGET_WINDOWS
  EXPECT_EQ(sources.front().strDevicePath, "E:");
#else
  EXPECT_TRUE(sources.front().strDevicePath.empty());
#endif
}

TEST(TestMediaSourceSettings, LoadString)
{
  CMediaSourceSettings& ms = CMediaSourceSettings::GetInstance();
  EXPECT_TRUE(ms.Load(XBMC_REF_FILE_PATH("/xbmc/settings/test/test-MediaSources.xml")));

  EXPECT_EQ(ms.GetSources(MediaSection::PROGRAMS).size(), 0);
  EXPECT_EQ(ms.GetSources(MediaSection::FILES).size(), 0);
  EXPECT_EQ(ms.GetSources(MediaSection::MUSIC).size(), 2);
  EXPECT_EQ(ms.GetSources(MediaSection::VIDEO).size(), 4);
  EXPECT_EQ(ms.GetSources(MediaSection::PICTURES).size(), 1);
  EXPECT_EQ(ms.GetSources(MediaSection::GAMES).size(), 0);
}

TEST(TestMediaSourceSettings, SaveString)
{
  CMediaSourceSettings& ms = CMediaSourceSettings::GetInstance();
  EXPECT_TRUE(ms.Load(XBMC_REF_FILE_PATH("/xbmc/settings/test/test-MediaSources.xml")));

  int refprograms = ms.GetSources(MediaSection::PROGRAMS).size();
  int reffiles = ms.GetSources(MediaSection::FILES).size();
  int refmusic = ms.GetSources(MediaSection::MUSIC).size();
  int refvideo = ms.GetSources(MediaSection::VIDEO).size();
  int refpictures = ms.GetSources(MediaSection::PICTURES).size();
  int refgames = ms.GetSources(MediaSection::GAMES).size();

  XFILE::CFile* file;
  file = XBMC_CREATETEMPFILE(".xml");
  std::string xmlfile = XBMC_TEMPFILEPATH(file);
  file->Close();

  EXPECT_TRUE(ms.Save(xmlfile));
  ms.Clear();
  EXPECT_TRUE(ms.Load(xmlfile));
  const auto& progsources = ms.GetSources(MediaSection::PROGRAMS);
  EXPECT_EQ(progsources.size(), refprograms);
  const auto& filessources = ms.GetSources(MediaSection::FILES);
  EXPECT_EQ(filessources.size(), reffiles);
  const auto& musicsources = ms.GetSources(MediaSection::MUSIC);
  EXPECT_EQ(musicsources.size(), refmusic);
  const auto& videosources = ms.GetSources(MediaSection::VIDEO);
  EXPECT_EQ(videosources.size(), refvideo);
  const auto& picturessources = ms.GetSources(MediaSection::PICTURES);
  EXPECT_EQ(picturessources.size(), refpictures);
  const auto& gamessources = ms.GetSources(MediaSection::GAMES);
  EXPECT_EQ(gamessources.size(), refgames);

  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
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
