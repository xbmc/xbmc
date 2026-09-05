/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "test/TestUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto GAME_PATH = "/roms/my_game.m3u";

void CleanupStateFile()
{
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);
  XFILE::CFile::Delete(xmlPath);

  // State files now live in a per-game subdirectory. Remove the empty subdirectory
  // so each test starts from the same clean slate regardless of save order.
  std::string stateSubdirectory = URIUtils::GetDirectory(xmlPath);
  URIUtils::RemoveSlashAtEnd(stateSubdirectory);
  if (!stateSubdirectory.empty() && XFILE::CDirectory::Exists(stateSubdirectory))
    XFILE::CDirectory::Remove(stateSubdirectory);
}

void EnsureStateSubdirectory()
{
  const std::string xmlDirectory =
      URIUtils::GetDirectory(CGameClientDiscXML::GetXMLPath(GAME_PATH));
  ASSERT_TRUE(XFILE::CDirectory::Create(xmlDirectory));
}

std::string ReadStateXml()
{
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  if (!file.Open(xmlPath))
    return "";

  std::string xml;
  xml.resize(static_cast<size_t>(file.GetLength()));
  if (!xml.empty())
    file.Read(xml.data(), xml.size());

  file.Close();
  return xml;
}

} // namespace

TEST(TestGameClientDiscXML, SaveLoadRoundtripPreservesSlotTypes)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd", "Disc One");
  savedModel.AddRemovedSlot();
  savedModel.AddRemovedSlot();

  ASSERT_TRUE(savedModel.SetSelectedDiscByIndex(0));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 3U);
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(2));
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetLabelByIndex(0), "Disc One");

  ASSERT_TRUE(loadedModel.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*loadedModel.GetSelectedDiscIndex(), 0U);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveLoadSelectedNonePreserved)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.SetSelectedNoDisc();

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_TRUE(loadedModel.IsSelectedNoDisc());
  EXPECT_FALSE(loadedModel.GetSelectedDiscIndex().has_value());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, MissingXmlIsNonErrorAndLeavesEmptyModel)
{
  CleanupStateFile();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscXML, MalformedXmlFailsAndClearsModel)
{
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);
  EnsureStateSubdirectory();

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char malformed[] = "<discstate><slots><slot type=\"disc\"></slots>";
  ASSERT_EQ(file.Write(malformed, sizeof(malformed) - 1), sizeof(malformed) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  loadedModel.AddDisc("/roms/placeholder.chd");

  EXPECT_FALSE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveWritesEjectedTrue)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"true\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveWritesEjectedFalse)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"false\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedState)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedFalseState)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadMissingEjectedDefaultsToFalse)
{
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);
  EnsureStateSubdirectory();

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char xml[] =
      "<discstate><slots><slot type=\"disc\" path=\"/roms/disc1.chd\"/></slots>"
      "<selected type=\"none\"/></discstate>";
  ASSERT_EQ(file.Write(xml, sizeof(xml) - 1), sizeof(xml) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, GetXMLPathUsesPerGameDirectoryAndExtensionlessBaseName)
{
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  EXPECT_EQ(URIUtils::GetFileName(xmlPath), "my_game.xml");
  EXPECT_EQ(URIUtils::GetExtension(xmlPath), ".xml");
  EXPECT_EQ(xmlPath.find("my_game.m3u.xml"), std::string::npos);

  std::string xmlDirectoryName = URIUtils::GetDirectory(xmlPath);
  URIUtils::RemoveSlashAtEnd(xmlDirectoryName);
  xmlDirectoryName = URIUtils::GetFileName(xmlDirectoryName);

  EXPECT_TRUE(StringUtils::StartsWith(xmlDirectoryName, "my_game.m3u_"));
}

TEST(TestGameClientDiscXML, SaveCreatesPerGameStateFile)
{
  // Save should create any missing state directories before writing XML files.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  EXPECT_TRUE(CFileUtils::Exists(CGameClientDiscXML::GetXMLPath(GAME_PATH)));

  CleanupStateFile();
}

class TestGameClientDiscXMLInvalidSelection : public testing::TestWithParam<const char*>
{
protected:
  void SetUp() override { CleanupStateFile(); }
  void TearDown() override { CleanupStateFile(); }
};

TEST_P(TestGameClientDiscXMLInvalidSelection, RejectsInvalidSelection)
{
  EnsureStateSubdirectory();
  const std::string xml =
      std::string{"<discstate><slots><slot type=\"disc\" path=\"/roms/disc1.chd\"/>"
                  "<slot type=\"removed\"/></slots>"} +
      GetParam() + "</discstate>";
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(CGameClientDiscXML::GetXMLPath(GAME_PATH), true));
  ASSERT_EQ(file.Write(xml.data(), xml.size()), static_cast<ssize_t>(xml.size()));
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loaded;
  EXPECT_FALSE(discXml.Load(GAME_PATH, loaded));
  EXPECT_TRUE(loaded.Empty());
}

INSTANTIATE_TEST_SUITE_P(InvalidMetadata,
                         TestGameClientDiscXMLInvalidSelection,
                         testing::Values("<selected type=\"disc\" index=\"2\"/>",
                                         "<selected type=\"disc\" index=\"1\"/>",
                                         "<selected type=\"disc\" index=\"-1\"/>",
                                         "<selected type=\"disc\" index=\"abc\"/>",
                                         "<selected type=\"disc\" index=\"0junk\"/>",
                                         "<selected type=\"disc\" index=\"0.5\"/>",
                                         "<selected type=\"disc\" index=\"18446744073709551616\"/>",
                                         "<selected type=\"disc\" index=\"\"/>",
                                         "<selected type=\"disc\"/>",
                                         "<selected index=\"0\"/>",
                                         "<selected type=\"unknown\" index=\"0\"/>",
                                         ""));

class TestGameClientDiscXMLInvalidSlots : public testing::TestWithParam<const char*>
{
protected:
  void SetUp() override { CleanupStateFile(); }
  void TearDown() override { CleanupStateFile(); }
};

TEST_P(TestGameClientDiscXMLInvalidSlots, RejectsMalformedSlotWithoutShiftingSelection)
{
  EnsureStateSubdirectory();
  const std::string xml = std::string{"<discstate><slots>"} + GetParam() +
                          R"(<slot type="disc" path="/roms/disc2.chd"/></slots>)" +
                          R"(<selected type="disc" index="0"/></discstate>)";
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(CGameClientDiscXML::GetXMLPath(GAME_PATH), true));
  ASSERT_EQ(file.Write(xml.data(), xml.size()), static_cast<ssize_t>(xml.size()));
  file.Close();

  CGameClientDiscModel loaded;
  EXPECT_FALSE(CGameClientDiscXML::Load(GAME_PATH, loaded));
  EXPECT_TRUE(loaded.Empty());
}

INSTANTIATE_TEST_SUITE_P(InvalidMetadata,
                         TestGameClientDiscXMLInvalidSlots,
                         testing::Values(R"(<slot type="disc"/>)",
                                         R"(<slot type="disc" path=""/>)",
                                         R"(<slot type="unknown" path="/roms/disc1.chd"/>)",
                                         R"(<slot path="/roms/disc1.chd"/>)"));

class TestGameClientDiscXMLHistory : public testing::Test
{
protected:
  void SetUp() override { CleanupStateFile(); }
  void TearDown() override
  {
    CleanupStateFile();
    for (auto* file : m_files)
      EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
  }

  std::string CreateMedia()
  {
    auto* file = XBMC_CREATETEMPFILE(".chd");
    EXPECT_NE(file, nullptr);
    if (!file)
      return {};
    m_files.push_back(file);
    file->Close();
    return XBMC_TEMPFILEPATH(file);
  }

private:
  std::vector<XFILE::CFile*> m_files;
};

TEST_F(TestGameClientDiscXMLHistory, RemovedAndErasedMediaRemainResolvable)
{
  const std::string first = CreateMedia();
  const std::string second = CreateMedia();
  ASSERT_FALSE(first.empty());
  ASSERT_FALSE(second.empty());
  CGameClientDiscModel model;
  model.AddDisc(first, "One");
  model.AddDisc(second, "Two");
  const auto state = model.GetState();
  ASSERT_TRUE(model.MarkRemovedByIndex(0));
  ASSERT_TRUE(model.EraseDiscByIndex(1));

  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(GAME_PATH, model));
  CGameClientDiscModel loaded;
  ASSERT_TRUE(xml.Load(GAME_PATH, loaded));
  ASSERT_EQ(loaded.Size(), 1U);
  EXPECT_TRUE(loaded.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(loaded.GetPathByIndex(0).empty());
  EXPECT_FALSE(loaded.SetSelectedDiscByIndex(0));

  CGameClientDiscModel restored;
  ASSERT_TRUE(loaded.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), first);
  EXPECT_EQ(restored.GetPathByIndex(1), second);
  EXPECT_EQ(restored.GetLabelByIndex(1), "Two");
}

TEST_F(TestGameClientDiscXMLHistory, ReusedSlotRetainsFormerIdentity)
{
  const std::string former = CreateMedia();
  const std::string replacement = CreateMedia();
  ASSERT_FALSE(former.empty());
  ASSERT_FALSE(replacement.empty());
  CGameClientDiscModel model;
  model.AddDisc(former);
  const auto state = model.GetState();
  ASSERT_TRUE(model.MarkRemovedByIndex(0));
  model.SetDiscs({{GameClientDiscEntry::DiscSlotType::Disc,
                   replacement,
                   CGameClientDiscModel::DeriveBasename(replacement),
                   {}}});

  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(GAME_PATH, model));
  CGameClientDiscModel loaded;
  ASSERT_TRUE(xml.Load(GAME_PATH, loaded));
  EXPECT_EQ(loaded.GetPathByIndex(0), replacement);
  CGameClientDiscModel restored;
  ASSERT_TRUE(loaded.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), former);
  EXPECT_EQ(loaded.Size(), 1U);
}
