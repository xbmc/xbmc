/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "playlists/SmartPlayList.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace KODI::PLAYLIST;

namespace
{
constexpr std::string_view XSP{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<smartplaylist type="movies">
  <name>Test &amp; Playlist</name>
  <match>all</match>
  <rule field="genre" operator="is">
    <value>Action</value>
    <!-- comment -->
    <value>Comedy</value>
  </rule>
  <rule field="title" operator="contains">Star</rule>
  <rule field="inprogress" operator="true"/>
  <rule field="rating" operator="greaterthan"><!-- comment --><value>5</value></rule>
  <rule operator="is">no field</rule>
  <group mixed="true">genres</group>
  <limit>25</limit>
  <order direction="descending" ignorefolders="true">rating</order>
  <watchedmode>1</watchedmode>
</smartplaylist>)"};

constexpr std::string_view XSP_JSON{
    R"({"group":{"mixed":true,"type":"genres"},"limit":25,"order":{"direction":"descending",)"
    R"("ignorefolders":2,"method":"rating"},"rules":{"and":[{"field":"genre","operator":"is",)"
    R"("value":["Action","Comedy"]},{"field":"title","operator":"contains","value":["Star"]},)"
    R"({"field":"inprogress","operator":"true","value":[]}]},"type":"movies","watchedmode":1})"};

std::string ToJson(const CSmartPlaylist& playlist)
{
  std::string json;
  EXPECT_TRUE(playlist.SaveAsJson(json));
  return json;
}

class TestSmartPlayList : public testing::Test
{
protected:
  void TearDown() override
  {
    if (m_file)
      XBMC_DELETETEMPFILE(m_file);
  }

  std::string CreateFile(std::string_view content)
  {
    m_file = XBMC_CREATETEMPFILE(".xsp");
    m_file->Write(content.data(), content.size());
    m_file->Close();
    return XBMC_TEMPFILEPATH(m_file);
  }

  XFILE::CFile* m_file{nullptr};
};
} // namespace

TEST_F(TestSmartPlayList, LoadFromXml)
{
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.LoadFromXml(std::string{XSP}));
  EXPECT_EQ("Test & Playlist", playlist.GetName());
  EXPECT_EQ("movies", playlist.GetType());
  EXPECT_EQ(XSP_JSON, ToJson(playlist));
}

TEST_F(TestSmartPlayList, LoadFromXmlInvalid)
{
  CSmartPlaylist playlist;
  EXPECT_FALSE(playlist.LoadFromXml(""));
  EXPECT_FALSE(playlist.LoadFromXml("<playlist type=\"movies\"/>"));
  EXPECT_FALSE(playlist.LoadFromXml("<smartplaylist"));
}

TEST_F(TestSmartPlayList, LoadFile)
{
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.Load(CreateFile(XSP)));
  EXPECT_EQ("Test & Playlist", playlist.GetName());
  EXPECT_EQ(XSP_JSON, ToJson(playlist));
}

TEST_F(TestSmartPlayList, LoadFileNameFromPath)
{
  const std::string path{CreateFile("<smartplaylist type=\"songs\"><match>one</match>"
                                    "<rule field=\"genre\" operator=\"is\">Rock</rule>"
                                    "</smartplaylist>")};
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.Load(path));
  std::string name{URIUtils::GetFileName(path)};
  URIUtils::RemoveExtension(name);
  EXPECT_EQ(name, playlist.GetName());
  EXPECT_EQ("songs", playlist.GetType());
}

TEST_F(TestSmartPlayList, LoadFileUnescapedAmpersand)
{
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.Load(CreateFile("<smartplaylist type=\"songs\"><name>Rock & Roll</name>"
                                       "<rule field=\"genre\" operator=\"is\">R&B</rule>"
                                       "</smartplaylist>")));
  // TinyXML1 drops unescaped ampersands when reading from a stream
  EXPECT_EQ("Rock  Roll", playlist.GetName());
  EXPECT_EQ(
      R"({"rules":{"and":[{"field":"genre","operator":"is","value":["RB"]}]},"type":"songs"})",
      ToJson(playlist));
}

TEST_F(TestSmartPlayList, OpenAndReadName)
{
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.OpenAndReadName(CURL(CreateFile(XSP))));
  EXPECT_EQ("Test & Playlist", playlist.GetName());
  EXPECT_EQ("movies", playlist.GetType());
}

TEST_F(TestSmartPlayList, SaveAndReload)
{
  CSmartPlaylist playlist;
  ASSERT_TRUE(playlist.LoadFromXml(std::string{XSP}));

  const std::string path{CreateFile("")};
  ASSERT_TRUE(playlist.Save(path));

  CSmartPlaylist reloaded;
  ASSERT_TRUE(reloaded.Load(path));
  EXPECT_EQ("Test & Playlist", reloaded.GetName());
  EXPECT_EQ(XSP_JSON, ToJson(reloaded));
}

TEST_F(TestSmartPlayList, LoadFromXMLNode)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{R"(<node order="1" type="filter">
    <label>Drama</label>
    <content>movies</content>
    <match>one</match>
    <rule field="genre" operator="is"><value>Drama</value></rule>
    <rule field="playcount" operator="is">0</rule>
    <limit>10</limit>
  </node>)"}));

  CSmartPlaylist playlist;
  playlist.SetType("movies");
  ASSERT_TRUE(playlist.LoadFromXML(doc.RootElement()));
  EXPECT_EQ(R"({"limit":10,"rules":{"or":[{"field":"genre","operator":"is","value":["Drama"]},)"
            R"({"field":"playcount","operator":"is","value":["0"]}]},"type":"movies"})",
            ToJson(playlist));
}
