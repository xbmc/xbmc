/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/Album.h"
#include "music/Artist.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
template<typename T>
void CheckNfoVersion(T& details, const char* root)
{
  CXBMCTinyXML saved;
  ASSERT_TRUE(details.Save(&saved, root, "/music/"));
  ASSERT_NE(nullptr, saved.RootElement());
  EXPECT_STREQ(root, saved.RootElement()->Value());
  EXPECT_STREQ("0", saved.RootElement()->Attribute("version"));
  int version = -1;
  EXPECT_EQ(TIXML_SUCCESS, saved.RootElement()->QueryIntAttribute("version", &version));
  EXPECT_EQ(0, version);

  T reloaded;
  ASSERT_TRUE(reloaded.Load(saved.RootElement()));
  TiXmlElement container("musicdb");
  ASSERT_TRUE(reloaded.Save(&container, root, "/music/"));
  EXPECT_EQ(nullptr, container.Attribute("version"));
  EXPECT_TRUE(
      XMLUtils::AreNodesSerializationsEqual(saved.RootElement(), container.FirstChildElement()));

  saved.RootElement()->RemoveAttribute("version");
  T unversioned;
  ASSERT_TRUE(unversioned.Load(saved.RootElement()));
  CXBMCTinyXML unrelated;
  ASSERT_TRUE(unversioned.Save(&unrelated, "details", "/music/"));
  EXPECT_EQ(nullptr, unrelated.RootElement()->Attribute("version"));
  unrelated.RootElement()->SetValue(root);
  EXPECT_TRUE(XMLUtils::AreNodesSerializationsEqual(saved.RootElement(), unrelated.RootElement()));
}
} // namespace

TEST(TestMusicInfoTag, ArtistNfoVersionZero)
{
  CXBMCTinyXML input;
  const std::string document =
      "<artist><name>Test artist</name><biography>Test biography</biography>"
      "<album><title>Test album</title><year>2001</year></album></artist>";
  input.Parse(document);
  CArtist artist;
  ASSERT_TRUE(artist.Load(input.RootElement()));
  EXPECT_EQ("Test artist", artist.strArtist);
  EXPECT_EQ("Test biography", artist.strBiography);
  ASSERT_EQ(1u, artist.discography.size());
  EXPECT_EQ("Test album", artist.discography.front().strAlbum);
  CheckNfoVersion(artist, "artist");

  CXBMCTinyXML saved;
  ASSERT_TRUE(artist.Save(&saved, "artist", ""));
  ASSERT_NE(nullptr, saved.RootElement()->FirstChildElement("album"));
  EXPECT_EQ(nullptr, saved.RootElement()->FirstChildElement("album")->Attribute("version"));
}

TEST(TestMusicInfoTag, AlbumNfoVersionZero)
{
  CXBMCTinyXML input;
  const std::string document = "<album><title>Test album</title><review>Test review</review>"
                               "<rating>7.5</rating><votes>123</votes></album>";
  input.Parse(document);
  CAlbum album;
  ASSERT_TRUE(album.Load(input.RootElement()));
  EXPECT_EQ("Test album", album.strAlbum);
  EXPECT_EQ("Test review", album.strReview);
  EXPECT_FLOAT_EQ(7.5f, album.fRating);
  EXPECT_EQ(123, album.iVotes);
  CheckNfoVersion(album, "album");
}
