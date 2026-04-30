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
  // Verify roundtripping keeps real and removed slots in their original order.
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
  // Verify explicit "No disc" selection survives XML serialization and load.
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
  // Verify loading with no persisted XML is treated as success with an empty model.
  CleanupStateFile();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscXML, MalformedXmlFailsAndClearsModel)
{
  // Verify malformed XML is rejected and the output model is reset.
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
  // Verify saving with an ejected tray writes the true tray flag.
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
  // Verify saving with an inserted tray writes the false tray flag.
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
  // Verify loading restores a previously persisted ejected tray state.
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
  // Verify loading restores a previously persisted non-ejected tray state.
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
  // Verify older XML without tray metadata defaults to non-ejected.
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);
  EnsureStateSubdirectory();

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char xml[] =
      "<discstate><slots><slot type=\"disc\" path=\"/roms/disc1.chd\"/></slots></discstate>";
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
  // Verify XML save path uses "<base>_<crc>/<base>.xml" and does not keep source extensions.
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
