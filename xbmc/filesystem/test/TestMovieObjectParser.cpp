/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/bluray/MovieObjectParser.h"
#include "utils/URIUtils.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
using Bytes = std::vector<uint8_t>;

void Put16(Bytes& bytes, unsigned int value)
{
  bytes.push_back(static_cast<uint8_t>(value >> 8));
  bytes.push_back(static_cast<uint8_t>(value));
}

void Put32(Bytes& bytes, uint32_t value)
{
  for (int shift = 24; shift >= 0; shift -= 8)
    bytes.push_back(static_cast<uint8_t>(value >> shift));
}

//! A 12 byte navigation command - the first four bytes, then the destination and source
Bytes Command(uint8_t b0, uint8_t b1, uint8_t b3, uint32_t destination, uint32_t source = 0)
{
  Bytes bytes{b0, b1, 0, b3};
  Put32(bytes, destination);
  Put32(bytes, source);
  return bytes;
}

Bytes PlayPlaylist(uint32_t playlist)
{
  return Command(0x22, 0x80, 0, playlist);
}

Bytes PlayPlaylistInRegister(uint32_t gpr)
{
  return Command(0x22, 0x00, 0, gpr);
}

Bytes JumpObject(uint32_t object)
{
  return Command(0x21, 0x80, 0, object);
}

Bytes MoveToRegister(uint32_t gpr, uint32_t value)
{
  return Command(0x50, 0x40, 0x01, gpr, value);
}

//! An index entry. A title of an HDMV object names the object, one of a BD-J object names its file.
Bytes IndexEntry(bool hdmv, unsigned int object)
{
  Bytes bytes{static_cast<uint8_t>(hdmv ? 0x40 : 0x80), 0, 0, 0, 0, 0};
  if (hdmv)
    Put16(bytes, object);
  else
    bytes.insert(bytes.end(), {'0', '0', '0', '0', '1'});
  bytes.resize(12);
  return bytes;
}

void Write(const std::string& path, const Bytes& bytes)
{
  CFile file;
  ASSERT_TRUE(file.OpenForWrite(path, true));
  ASSERT_EQ(file.Write(bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
  file.Close();
}

//! A disc in special://temp whose first playback is object 4, with these titles and objects
void WriteDisc(const std::string& disc,
               const std::vector<Bytes>& titles,
               const std::vector<std::vector<Bytes>>& objects)
{
  CDirectory::RemoveRecursive(disc);
  ASSERT_TRUE(CDirectory::Create(disc));
  ASSERT_TRUE(CDirectory::Create(URIUtils::AddFileToFolder(disc, "BDMV")));

  Bytes index{'I', 'N', 'D', 'X', '0', '2', '0', '0'};
  Put32(index, 16); // indexes start
  Put32(index, 0); // extension data start
  Put32(index, 0); // indexes length
  for (const Bytes& entry : {IndexEntry(true, 4), IndexEntry(true, MOVIE_OBJECT_NONE)})
    index.insert(index.end(), entry.begin(), entry.end());
  Put16(index, static_cast<unsigned int>(titles.size()));
  for (const Bytes& entry : titles)
    index.insert(index.end(), entry.begin(), entry.end());
  Write(URIUtils::AddFileToFolder(disc, "BDMV", "index.bdmv"), index);

  Bytes table;
  Put32(table, 0); // reserved
  Put16(table, static_cast<unsigned int>(objects.size()));
  for (const auto& commands : objects)
  {
    Put16(table, 0); // flags
    Put16(table, static_cast<unsigned int>(commands.size()));
    for (const Bytes& command : commands)
      table.insert(table.end(), command.begin(), command.end());
  }
  Bytes movieObject{'M', 'O', 'B', 'J', '0', '2', '0', '0'};
  movieObject.resize(40);
  Put32(movieObject, static_cast<uint32_t>(table.size()));
  movieObject.insert(movieObject.end(), table.begin(), table.end());
  Write(URIUtils::AddFileToFolder(disc, "BDMV", "MovieObject.bdmv"), movieObject);
}

std::vector<unsigned int> GetTitlePlaylists(const std::string& disc)
{
  CURL url;
  url.SetProtocol("bluray");
  url.SetHostName(disc);
  MovieObjectInformation information;
  EXPECT_TRUE(CMovieObjectParser::GetMovieObject(url, information));
  EXPECT_TRUE(information.movieObjectsRead);
  return information.titlePlaylists;
}
} // namespace

TEST(TestMovieObjectParser, TitlesArePlayedInTheirOrder)
{
  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/MovieObjectParser/")};

  // Title 1 plays the feature, title 2 jumps to an object that plays an extra and the feature,
  // title 3 is BD-J and title 4 plays from a register it set itself
  WriteDisc(disc,
            {IndexEntry(true, 0), IndexEntry(true, 1), IndexEntry(false, 0), IndexEntry(true, 3)},
            {{PlayPlaylist(800)},
             {JumpObject(2)},
             {PlayPlaylist(5), PlayPlaylist(800)},
             {MoveToRegister(1, 7), PlayPlaylistInRegister(1)},
             {PlayPlaylist(99)}});

  EXPECT_EQ(GetTitlePlaylists(disc), (std::vector<unsigned int>{800, 5, 7}));
  EXPECT_TRUE(CDirectory::RemoveRecursive(disc));
}

TEST(TestMovieObjectParser, AnObjectPlaysWhatEachCallerSetsItToPlay)
{
  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/MovieObjectParser/")};

  // A "play all" title has object 1 play each scene in turn from the register it set
  WriteDisc(disc, {IndexEntry(true, 0)},
            {{MoveToRegister(1, 5), JumpObject(1), MoveToRegister(1, 7), JumpObject(1)},
             {PlayPlaylistInRegister(1)},
             {},
             {},
             {}});

  EXPECT_EQ(GetTitlePlaylists(disc), (std::vector<unsigned int>{5, 7}));
  EXPECT_TRUE(CDirectory::RemoveRecursive(disc));
}

TEST(TestMovieObjectParser, AnObjectCountingInALoopEnds)
{
  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/MovieObjectParser/")};

  // Object 1 plays the register, adds one to it and jumps back to itself
  WriteDisc(disc, {IndexEntry(true, 0)},
            {{MoveToRegister(1, 1), JumpObject(1)},
             {PlayPlaylistInRegister(1), Command(0x50, 0x40, 0x03, 1, 1), JumpObject(1)},
             {},
             {},
             {}});

  EXPECT_EQ(GetTitlePlaylists(disc).size(), 16u);
  EXPECT_TRUE(CDirectory::RemoveRecursive(disc));
}
