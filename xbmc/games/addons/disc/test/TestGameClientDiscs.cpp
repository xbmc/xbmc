/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace KODI::GAME
{
class TestGameClientDiscs : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml = R"(<addon id="game.test.discs" name="Disc test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_disc_control>true</supports_disc_control>
        <extensions>chd|m3u</extensions>
      </extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);
    m_client->GetInstanceInterface()->toAddon->SetInitialImage =
        [](const AddonInstance_Game*, unsigned int, const char*) { return GAME_ERROR_NO_ERROR; };
  }

  void TearDown() override
  {
    for (auto* file : m_files)
    {
      const std::string path = XBMC_TEMPFILEPATH(file);
      XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(path));
      XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(path));
      EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
    }
  }

  std::string CreateFile(const std::string& extension, const std::string& contents = {})
  {
    auto* file = XBMC_CREATETEMPFILE(extension);
    EXPECT_NE(file, nullptr);
    if (!file)
      return {};
    m_files.push_back(file);
    if (!contents.empty())
      EXPECT_EQ(file->Write(contents.data(), contents.size()),
                static_cast<ssize_t>(contents.size()));
    file->Close();
    return XBMC_TEMPFILEPATH(file);
  }

  std::unique_ptr<CGameClient> m_client;
  std::vector<XFILE::CFile*> m_files;
};
} // namespace KODI::GAME

TEST_F(TestGameClientDiscs, InvalidPersistedSelectionFallsBackToSource)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(disc);
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("selected")->SetAttribute("index", 5);
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.GetPathByIndex(0), disc);
  EXPECT_EQ(current.GetSelectedDiscIndex(), 0U);
}

TEST_F(TestGameClientDiscs, MalformedPersistedSlotCannotShiftSelectionOntoAnotherDisc)
{
  const std::string first = CreateFile(".chd");
  const std::string second = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", first + "\n" + second + "\n");
  ASSERT_FALSE(first.empty());
  ASSERT_FALSE(second.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(first);
  previous.AddDisc(second);
  ASSERT_TRUE(CGameClientDiscXML::Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("slots")->FirstChildElement("slot")->DeleteAttribute(
      "path");
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.Size(), 2U);
  EXPECT_EQ(current.GetSelectedDiscPath(), first);
}
