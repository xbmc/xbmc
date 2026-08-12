/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/bluray/PlaylistStructure.h"
#include "filesystem/bluray/StreamParser.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;

namespace
{
StreamInformation MakeStream(ENCODING_TYPE coding, unsigned int pid, std::string language)
{
  StreamInformation stream;
  stream.coding = coding;
  stream.packetIdentifier = pid;
  stream.language = std::move(language);
  return stream;
}

// A single clip carrying every stream of the disc's main feature, as the .clpi describes it.
ClipInformation MakeClip(unsigned int clip)
{
  ClipInformation clipInfo{clip, "M2TS"};
  clipInfo.streamsRead = true;

  ProgramInformation program;
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::VIDEO_H264, 0x1011, ""));
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::AUDIO_DTSHD_MASTER, 0x1100, "eng"));
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::AUDIO_AC3, 0x1101, "jpn"));
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::SUB_PG, 0x1200, "eng"));
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::SUB_PG, 0x1201, "fra"));
  program.streams.emplace_back(MakeStream(ENCODING_TYPE::SUB_PG, 0x1202, "jpn"));
  clipInfo.programs.emplace_back(std::move(program));

  return clipInfo;
}

// A playlist of that clip exposing only the streams given, in stream number order.
BlurayPlaylistInformation MakePlaylist(unsigned int playlist,
                                       unsigned int clip,
                                       const std::vector<StreamInformation>& audioStreams,
                                       const std::vector<StreamInformation>& subtitleStreams)
{
  BlurayPlaylistInformation info;
  info.playlist = playlist;
  info.duration = 2h;
  info.clipStreamsRead = true;

  PlayItemInformation playItem;
  playItem.outTime = 2h;
  playItem.angleClips.emplace_back(clip, "M2TS");
  playItem.videoStreams.emplace_back(MakeStream(ENCODING_TYPE::VIDEO_H264, 0x1011, ""));
  playItem.audioStreams = audioStreams;
  playItem.presentationGraphicStreams = subtitleStreams;
  info.playItems.emplace_back(std::move(playItem));

  info.clips.emplace_back(MakeClip(clip));

  return info;
}

std::vector<std::string> AudioLanguagesOf(const PlaylistInformation& p)
{
  std::vector<std::string> languages;
  for (const auto& stream : p.audioStreams)
    languages.emplace_back(stream.language.AsBcp47());
  return languages;
}

std::vector<std::string> SubtitleLanguagesOf(const PlaylistInformation& p)
{
  std::vector<std::string> languages;
  for (const auto& stream : p.pgStreams)
    languages.emplace_back(stream.language.AsBcp47());
  return languages;
}
} // namespace

TEST(TestStreamParser, StreamsComeFromThePlaylistNotTheSharedClip)
{
  // Two playlists of the same feature share a clip but expose different streams - the second is
  // the Japanese cut, starting on Japanese audio and subtitles. Describing them from the clip's
  // program list would give both every stream the m2ts carries, making the versions
  // indistinguishable and reporting the wrong stream as each one's default.
  const std::vector<StreamInformation> allAudio{
      MakeStream(ENCODING_TYPE::AUDIO_DTSHD_MASTER, 0x1100, "eng"),
      MakeStream(ENCODING_TYPE::AUDIO_AC3, 0x1101, "jpn")};
  const std::vector<StreamInformation> allSubtitles{
      MakeStream(ENCODING_TYPE::SUB_PG, 0x1200, "eng"),
      MakeStream(ENCODING_TYPE::SUB_PG, 0x1201, "fra"),
      MakeStream(ENCODING_TYPE::SUB_PG, 0x1202, "jpn")};

  PlaylistInformation english;
  CStreamParser::ConvertBlurayPlaylistInformation(MakePlaylist(33, 30, allAudio, allSubtitles),
                                                  english, {}, StreamDetails::INCLUDE);

  EXPECT_EQ(AudioLanguagesOf(english), (std::vector<std::string>{"en", "ja"}));
  EXPECT_EQ(SubtitleLanguagesOf(english), (std::vector<std::string>{"en", "fr", "ja"}));

  const std::vector<StreamInformation> japaneseAudio{
      MakeStream(ENCODING_TYPE::AUDIO_AC3, 0x1101, "jpn"),
      MakeStream(ENCODING_TYPE::AUDIO_DTSHD_MASTER, 0x1100, "eng")};
  const std::vector<StreamInformation> japaneseSubtitles{
      MakeStream(ENCODING_TYPE::SUB_PG, 0x1202, "jpn"),
      MakeStream(ENCODING_TYPE::SUB_PG, 0x1200, "eng")};

  PlaylistInformation japanese;
  CStreamParser::ConvertBlurayPlaylistInformation(
      MakePlaylist(20, 30, japaneseAudio, japaneseSubtitles), japanese, {}, StreamDetails::INCLUDE);

  EXPECT_EQ(AudioLanguagesOf(japanese), (std::vector<std::string>{"ja", "en"}));
  EXPECT_EQ(SubtitleLanguagesOf(japanese), (std::vector<std::string>{"ja", "en"}));
}

TEST(TestStreamParser, TheFirstStreamOfEachTypeIsFlaggedAsTheDefault)
{
  // A player starts with audio stream number 1 and presentation graphic stream number 1, so those
  // are the streams the disc expects playback to begin with
  const std::vector<StreamInformation> audio{
      MakeStream(ENCODING_TYPE::AUDIO_AC3, 0x1101, "jpn"),
      MakeStream(ENCODING_TYPE::AUDIO_DTSHD_MASTER, 0x1100, "eng")};
  const std::vector<StreamInformation> subtitles{MakeStream(ENCODING_TYPE::SUB_PG, 0x1202, "jpn"),
                                                 MakeStream(ENCODING_TYPE::SUB_PG, 0x1200, "eng")};

  PlaylistInformation p;
  CStreamParser::ConvertBlurayPlaylistInformation(MakePlaylist(20, 30, audio, subtitles), p, {},
                                                  StreamDetails::INCLUDE);

  ASSERT_EQ(p.audioStreams.size(), 2U);
  EXPECT_TRUE(p.audioStreams[0].flags & StreamFlags::FLAG_DEFAULT);
  EXPECT_FALSE(p.audioStreams[1].flags & StreamFlags::FLAG_DEFAULT);

  ASSERT_EQ(p.pgStreams.size(), 2U);
  EXPECT_TRUE(p.pgStreams[0].flags & StreamFlags::FLAG_DEFAULT);
  EXPECT_FALSE(p.pgStreams[1].flags & StreamFlags::FLAG_DEFAULT);
}

TEST(TestStreamParser, SecondaryVideoMarksAPictureInPicturePresentation)
{
  const std::vector<StreamInformation> audio{
      MakeStream(ENCODING_TYPE::AUDIO_DTSHD_MASTER, 0x1100, "eng")};

  BlurayPlaylistInformation feature{MakePlaylist(100, 30, audio, {})};
  PlaylistInformation p;
  CStreamParser::ConvertBlurayPlaylistInformation(feature, p, {}, StreamDetails::INCLUDE);
  EXPECT_FALSE(p.hasSecondaryVideo);

  // An in-movie experience carries a second video stream to show over the film. Kodi does not play
  // it, so it is not added as a stream of the playlist - it only marks what the playlist is.
  BlurayPlaylistInformation inMovieExperience{MakePlaylist(101, 30, audio, {})};
  inMovieExperience.playItems[0].secondaryVideoStreams.emplace_back(
      MakeStream(ENCODING_TYPE::VIDEO_VC1, 0x1b00, ""));

  PlaylistInformation pip;
  CStreamParser::ConvertBlurayPlaylistInformation(inMovieExperience, pip, {},
                                                  StreamDetails::INCLUDE);
  EXPECT_TRUE(pip.hasSecondaryVideo);
  EXPECT_EQ(pip.videoStreams.size(), p.videoStreams.size());
}

TEST(TestStreamParser, PlaylistWithoutAStreamNumberTableFallsBackToTheClip)
{
  // A stream number table is expected of a conforming playlist, but if it is missing the clip's
  // program list is all there is to describe the playlist with
  BlurayPlaylistInformation b{MakePlaylist(33, 30, {}, {})};
  b.playItems[0].videoStreams.clear();

  PlaylistInformation p;
  CStreamParser::ConvertBlurayPlaylistInformation(b, p, {}, StreamDetails::INCLUDE);

  EXPECT_EQ(AudioLanguagesOf(p), (std::vector<std::string>{"en", "ja"}));
  EXPECT_EQ(SubtitleLanguagesOf(p), (std::vector<std::string>{"en", "fr", "ja"}));

  // Nothing has been read when stream details are deferred, so there is nothing to fall back to
  PlaylistInformation deferred;
  CStreamParser::ConvertBlurayPlaylistInformation(b, deferred, {}, StreamDetails::DEFER);
  EXPECT_TRUE(deferred.audioStreams.empty());
  EXPECT_TRUE(deferred.pgStreams.empty());
}
