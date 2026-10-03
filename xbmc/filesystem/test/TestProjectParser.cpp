/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/bluray/ProjectParser.h"

#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
constexpr uint16_t RECORD_MARKER = 0x0002;

void AppendWord(std::vector<std::byte>& buffer, uint16_t value)
{
  buffer.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
  buffer.push_back(static_cast<std::byte>(value & 0xFF));
}

void AppendDWord(std::vector<std::byte>& buffer, uint32_t value)
{
  buffer.push_back(static_cast<std::byte>((value >> 24) & 0xFF));
  buffer.push_back(static_cast<std::byte>((value >> 16) & 0xFF));
  buffer.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
  buffer.push_back(static_cast<std::byte>(value & 0xFF));
}

void AppendFloat(std::vector<std::byte>& buffer, float value)
{
  AppendDWord(buffer, std::bit_cast<uint32_t>(value));
}

void AppendString(std::vector<std::byte>& buffer, const std::string& text)
{
  for (char c : text)
    buffer.push_back(static_cast<std::byte>(c));
}

//! Everything a record can carry, so a test only has to override what it cares about.
struct RecordFields
{
  std::string name{"FPL_MainFeature"};
  uint32_t reserved{0};
  uint16_t playlist{800};
  uint16_t marker{RECORD_MARKER};
  std::string presentation{"2D"};
  float frameRate{23.976f};
  float duration{3661.0f}; // matches the default disc playlist's duration, below
  uint32_t playItems{5};
};

//! The disc playlist that RecordFields{} describes, for tests that need only one.
const DiscPlaylistDurations SINGLE_PLAYLIST{{800u, std::chrono::milliseconds{3661000}}};

std::vector<std::byte> MakeRecord(const RecordFields& fields = {})
{
  std::vector<std::byte> buffer;
  AppendWord(buffer, static_cast<uint16_t>(fields.name.size()));
  AppendString(buffer, fields.name);
  AppendDWord(buffer, fields.reserved);
  AppendWord(buffer, fields.playlist);
  AppendWord(buffer, fields.marker);
  AppendString(buffer, fields.presentation);
  AppendFloat(buffer, fields.frameRate);
  AppendFloat(buffer, fields.duration);
  AppendDWord(buffer, fields.playItems);
  return buffer;
}
} // namespace

TEST(TestProjectParser, WellFormedRecordBetweenJunkIsRead)
{
  const DiscPlaylistDurations discPlaylists{{800u, std::chrono::milliseconds{3661500}}};

  std::vector<std::byte> buffer{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
  const auto record{MakeRecord({.name = "FPL_MainFeature",
                                .playlist = 800,
                                .presentation = "2D",
                                .frameRate = 23.976f,
                                .duration = 3661.5f,
                                .playItems = 7})};
  buffer.insert(buffer.end(), record.begin(), record.end());
  buffer.push_back(std::byte{0xFF});
  buffer.push_back(std::byte{0x00});

  ProjectInformation project;
  EXPECT_EQ(CProjectParser::ParseProject(buffer, discPlaylists, project), ProjectReadResult::READ);

  ASSERT_EQ(project.playlists.size(), 1u);
  const auto& information{project.playlists.at(800u)};
  EXPECT_EQ(information.playlist, 800u);
  EXPECT_EQ(information.name, "FPL_MainFeature");
  EXPECT_FLOAT_EQ(information.frameRate, 23.976f);
  EXPECT_EQ(information.duration, std::chrono::milliseconds{3661500});
  EXPECT_EQ(information.presentation, "2D");
  EXPECT_EQ(information.playItems, 7u);
}

TEST(TestProjectParser, EmptyOrShortBufferIsAbsent)
{
  ProjectInformation project;
  EXPECT_EQ(CProjectParser::ParseProject({}, SINGLE_PLAYLIST, project), ProjectReadResult::ABSENT);

  const auto record{MakeRecord()};
  const std::vector<std::byte> shortBuffer(record.begin(), record.begin() + record.size() - 1);
  EXPECT_EQ(CProjectParser::ParseProject(shortBuffer, SINGLE_PLAYLIST, project),
            ProjectReadResult::ABSENT);
}

// A record cut off at any length must never be mistaken for a match, never be reported as a
// parse failure, and never throw - the scanner has to be safe against a file cut off mid-record.
TEST(TestProjectParser, TruncatedRecordNeverFailsReadsOrThrows)
{
  const auto record{MakeRecord()};

  for (size_t length = 0; length < record.size(); ++length)
  {
    SCOPED_TRACE(length);
    const std::vector<std::byte> truncated(record.begin(), record.begin() + length);

    ProjectInformation project;
    ProjectReadResult result{ProjectReadResult::FAILED};
    EXPECT_NO_THROW(result = CProjectParser::ParseProject(truncated, SINGLE_PLAYLIST, project));
    EXPECT_EQ(result, ProjectReadResult::ABSENT);
  }
}

TEST(TestProjectParser, EachRejectionGivesAbsent)
{
  struct Case
  {
    std::string description;
    RecordFields fields;
  };

  const std::vector<Case> cases{
      {"non-zero reserved", RecordFields{.reserved = 1}},
      {"wrong marker", RecordFields{.marker = 0x0003}},
      {"playlist not on disc", RecordFields{.playlist = 999}},
      {"NaN frame rate", RecordFields{.frameRate = std::numeric_limits<float>::quiet_NaN()}},
      {"frame rate out of range", RecordFields{.frameRate = 61.0f}},
      {"negative duration", RecordFields{.duration = -1.0f}},
      {"duration off by more than 2s", RecordFields{.duration = 3665.0f}},
      {"non-printable name", RecordFields{.name = std::string("FPL\x01MainFeature")}},
      {"non-printable presentation", RecordFields{.presentation = std::string("\x01D")}},
  };

  for (const auto& testCase : cases)
  {
    SCOPED_TRACE(testCase.description);
    const auto record{MakeRecord(testCase.fields)};

    ProjectInformation project;
    EXPECT_EQ(CProjectParser::ParseProject(record, SINGLE_PLAYLIST, project),
              ProjectReadResult::ABSENT);
    EXPECT_TRUE(project.playlists.empty());
  }
}

TEST(TestProjectParser, DurationWithinToleranceIsAccepted)
{
  // The disc playlist is 3661.0s; 1.9s off is inside the 2s tolerance
  const auto record{MakeRecord({.duration = 3662.9f})};

  ProjectInformation project;
  EXPECT_EQ(CProjectParser::ParseProject(record, SINGLE_PLAYLIST, project),
            ProjectReadResult::READ);
  ASSERT_EQ(project.playlists.size(), 1u);
}

TEST(TestProjectParser, DuplicatePlaylistKeepsFirstName)
{
  const auto first{MakeRecord({.name = "FPL_MainFeature"})};
  const auto second{MakeRecord({.name = "FPL_Duplicate"})};

  std::vector<std::byte> buffer{first};
  buffer.insert(buffer.end(), second.begin(), second.end());

  ProjectInformation project;
  EXPECT_EQ(CProjectParser::ParseProject(buffer, SINGLE_PLAYLIST, project),
            ProjectReadResult::READ);
  ASSERT_EQ(project.playlists.size(), 1u);
  EXPECT_EQ(project.playlists.at(800u).name, "FPL_MainFeature");
}

// Flips single bytes of a buffer holding several valid records, and separately parses buffers of
// pure noise. Neither should ever be reported as a failure, throw, or name a playlist the disc
// does not have.
TEST(TestProjectParser, RandomCorruptionNeverFailsOrNamesAnUnknownPlaylist)
{
  const DiscPlaylistDurations discPlaylists{{800u, std::chrono::milliseconds{3661000}},
                                            {801u, std::chrono::milliseconds{1000}},
                                            {802u, std::chrono::milliseconds{7200000}}};

  std::vector<std::byte> buffer;
  for (const auto& fields :
       {RecordFields{.name = "FPL_MainFeature", .playlist = 800, .duration = 3661.0f},
        RecordFields{.name = "EPL_01", .playlist = 801, .duration = 1.0f},
        RecordFields{.name = "EPL_02", .playlist = 802, .duration = 7200.0f}})
  {
    const auto record{MakeRecord(fields)};
    buffer.insert(buffer.end(), record.begin(), record.end());
  }

  std::mt19937 rng{12345};
  std::uniform_int_distribution<size_t> byteIndex{0, buffer.size() - 1};
  std::uniform_int_distribution<int> byteValue{0, 255};

  for (int iteration = 0; iteration < 5000; ++iteration)
  {
    std::vector<std::byte> mutated{buffer};
    mutated[byteIndex(rng)] = static_cast<std::byte>(byteValue(rng));

    ProjectInformation project;
    ProjectReadResult result{ProjectReadResult::FAILED};
    EXPECT_NO_THROW(result = CProjectParser::ParseProject(mutated, discPlaylists, project));
    EXPECT_NE(result, ProjectReadResult::FAILED);
    for (const auto& [playlist, information] : project.playlists)
      EXPECT_TRUE(discPlaylists.contains(playlist));
  }

  std::uniform_int_distribution<size_t> sizeDistribution{0, 256};
  for (int iteration = 0; iteration < 2000; ++iteration)
  {
    std::vector<std::byte> randomBuffer(sizeDistribution(rng));
    for (auto& b : randomBuffer)
      b = static_cast<std::byte>(byteValue(rng));

    ProjectInformation project;
    ProjectReadResult result{ProjectReadResult::FAILED};
    EXPECT_NO_THROW(result = CProjectParser::ParseProject(randomBuffer, discPlaylists, project));
    EXPECT_NE(result, ProjectReadResult::FAILED);
    for (const auto& [playlist, information] : project.playlists)
      EXPECT_TRUE(discPlaylists.contains(playlist));
  }
}
