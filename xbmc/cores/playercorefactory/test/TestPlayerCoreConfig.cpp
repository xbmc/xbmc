/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/playercorefactory/PlayerCoreConfig.h"
#include "cores/playercorefactory/PlayerSelectionRule.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

TEST(TestPlayerCoreConfig, CopiesConfig)
{
  std::unique_ptr<CPlayerCoreConfig> config;
  {
    CXBMCTinyXML2 doc;
    ASSERT_TRUE(
        doc.Parse(std::string{R"(<player name="Ext" type="ExternalPlayer" audio="true" )"
                              R"(video="TRUE"><filename>/usr/bin/true</filename></player>)"}));
    config = std::make_unique<CPlayerCoreConfig>("Ext", "external", doc.RootElement());
  }

  EXPECT_EQ("Ext", config->GetName());
  EXPECT_TRUE(config->PlaysAudio());
  EXPECT_TRUE(config->PlaysVideo());
  ASSERT_NE(nullptr, config->m_config);
  std::string filename;
  EXPECT_TRUE(XMLUtils::GetString(config->m_config->RootElement(), "filename", filename));
  EXPECT_EQ("/usr/bin/true", filename);
}

TEST(TestPlayerCoreConfig, NoAudioOrVideo)
{
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(std::string{R"(<player name="Ext" audio="no"/>)"}));
  const CPlayerCoreConfig config("Ext", "external", doc.RootElement());
  EXPECT_FALSE(config.PlaysAudio());
  EXPECT_FALSE(config.PlaysVideo());
}

TEST(TestPlayerCoreConfig, NoConfig)
{
  const CPlayerCoreConfig config("VideoPlayer", "video", nullptr);
  EXPECT_EQ(nullptr, config.m_config);
  EXPECT_FALSE(config.PlaysAudio());
  EXPECT_FALSE(config.PlaysVideo());
}

TEST(TestPlayerSelectionRule, GetPlayers)
{
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(std::string{R"(<rules name="rules">
      <rule name="mkv" filetypes="mkv" player="VideoPlayer"/>
      <rule name="http" protocols="http|https">
        <rule name="http mp3" filetypes="mp3" player="PAPlayer"/>
        <rule name="http missing" player="Missing"/>
        <rule name="http ext" player="Ext"/>
      </rule>
    </rules>)"}));
  CPlayerSelectionRule rule(doc.RootElement());

  std::vector<std::string> validPlayers{"VideoPlayer", "PAPlayer", "Ext"};
  const auto getPlayers = [&](const std::string& path)
  {
    std::vector<std::string> players;
    rule.GetPlayers(CFileItem(path, false), validPlayers, players);
    return players;
  };

  EXPECT_EQ((std::vector<std::string>{"VideoPlayer"}), getPlayers("/movies/movie.mkv"));
  EXPECT_EQ((std::vector<std::string>{"PAPlayer", "Ext"}), getPlayers("http://host/song.mp3"));
  EXPECT_EQ((std::vector<std::string>{"VideoPlayer", "Ext"}), getPlayers("https://host/movie.mkv"));
  EXPECT_TRUE(getPlayers("/music/song.mp3").empty());
}
