/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace KODI::GAME
{
class TestGameClientExtract : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml =
        R"(<addon id="game.test.extract" name="Extract test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_vfs>false</supports_vfs>
        <extensions>bin|chd|cue|hdm|m3u</extensions>
      </extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);
  }

  std::string Extract(const std::string& pathInArchive)
  {
    const std::string archive =
        XBMC_REF_FILE_PATH("xbmc/games/addons/test/testdata/archived-games.zip");
    return m_client->ExtractGame(
        URIUtils::CreateArchivePath("zip", CURL(archive), pathInArchive).Get());
  }

  static std::string Read(const std::string& path)
  {
    std::vector<uint8_t> data;
    if (XFILE::CFile().LoadFile(path, data) < 0)
      return "";
    return std::string(data.begin(), data.end());
  }

  std::unique_ptr<CGameClient> m_client;
};
} // namespace KODI::GAME

TEST_F(TestGameClientExtract, KeepsFilesOfTheSameNameApart)
{
  const std::string first = Extract("A/disk.hdm");
  const std::string second = Extract("B/disk.hdm");

  EXPECT_NE(first, second);
  EXPECT_EQ(Read(first), "disk A");
  EXPECT_EQ(Read(second), "disk B");
}

TEST_F(TestGameClientExtract, CopiesTheTracksOfACueSheet)
{
  const std::string game = Extract("cd/game.cue");

  ASSERT_TRUE(URIUtils::HasExtension(game, ".cue"));
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(URIUtils::GetDirectory(game), "game.bin")), "track 1");
}

TEST_F(TestGameClientExtract, CopiesTheDisksOfAPlaylist)
{
  const std::string game = Extract("set/game.m3u");

  EXPECT_EQ(Read(game), "disc1.chd\ndisc2.chd\n");
  const std::string folder = URIUtils::GetDirectory(game);
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(folder, "disc1.chd")), "disc 1");
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(folder, "disc2.chd")), "disc 2");
}

TEST_F(TestGameClientExtract, LeavesTheArchivesOwnPlaylistAlone)
{
  const std::string game = Extract("discs/game.cue");

  EXPECT_EQ(Read(game), "game.cue\ngame2.cue\n");
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(URIUtils::GetDirectory(game), "game.m3u")),
            "game2.cue\ngame.cue\n");
}
