/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace XFILE
{
struct BlurayPlaylistInformation;
enum class ASPECT_RATIO : uint8_t;
enum class ENCODING_TYPE : uint8_t;

using namespace std::chrono_literals;

enum class BLURAY_PLAYBACK_TYPE : unsigned int
{
  SEQUENTIAL = 1,
  RANDOM,
  SHUFFLE
};

enum class BLURAY_CONNECTION : unsigned int
{
  SEAMLESS = 1,
  NONSEAMLESS = 5,
  BRANCHING = 6 // Out-of-mux connection
};

enum class BLURAY_SUBPATH_TYPE : unsigned int
{
  PRIMARY_AUDIO_SLIDESHOW = 2,
  INTERACTIVE_GRAPHICS_PRESENTATION_MENU,
  TEXT_SUBTITLE_PRESENTATION,
  OUTOFMUX_SYNCHRONOUS,
  OUTOFMUX_ASYNCHRONOUS_PIP,
  INMUX_SYNCHRONOUS_PIP,
  STEREOSCOPIC_VIDEO,
  STEREOSCOPIC_INTERACTIVE_GRAPHICS_MENU,
  DOLBY_VISION_LAYER
};

enum class BLURAY_STREAM_TYPE : unsigned int
{
  PLAYITEM = 1,
  SUBPATH,
  SUBPATH_INMUX_SYNCHRONOUS_PIP,
  SUBPATH_DOLBY_VISION_LAYER
};

enum class BLURAY_MARK_TYPE : unsigned int
{
  ENTRY = 1,
  LINK
};

enum class STREAM_TYPE : unsigned int
{
  VIDEO_STREAM = 0,
  AUDIO_STREAM,
  PRESENTATION_GRAPHIC_STREAM,
  INTERACTIVE_GRAPHIC_STREAM,
  SECONDARY_VIDEO_STREAM,
  SECONDARY_AUDIO_STREAM,
  PICTURE_IN_PICTURE_SUBTITLE_STREAM,
  DOLBY_VISION_STREAM
};

struct StreamInformation
{
  BLURAY_STREAM_TYPE type{0};
  ENCODING_TYPE coding{0};
  unsigned int packetIdentifier{0};
  unsigned int subpathId{0};
  unsigned int subclipId{0};
  unsigned int format{0};
  unsigned int rate{0};
  ASPECT_RATIO aspect{0}; // CLPI only
  unsigned int dynamicRangeType{0};
  unsigned int colorSpace{0};
  bool copyRestricted{false};
  bool outOfMux{false}; // CLPI only
  bool HDRPlus{false};
  unsigned int characterEncoding{0};
  std::string language;
  std::vector<unsigned int> secondaryAudio_audioReferences;
  std::vector<unsigned int> secondaryVideo_audioReferences;
  std::vector<unsigned int> secondaryVideo_presentationGraphicReferences;
};

struct ProgramInformation
{
  unsigned int spnProgramSequenceStart{0};
  unsigned int programId{0};
  unsigned int numGroups{0};
  std::vector<StreamInformation> streams;
};

struct ClipInformation
{
  // user-defined ctor required for XCode 15.2 and emplace_back
  ClipInformation() = default;
  ClipInformation(unsigned int newClip, std::string&& newCodec);

  unsigned int clip{0};
  std::string version;
  std::string codec;
  std::chrono::milliseconds time{0ms}; // calculated from playItem
  std::chrono::milliseconds duration{0ms}; // calculated from playItem
  std::vector<ProgramInformation> programs;
};

struct PlaylistMarkInformation
{
  // user-defined ctor required for XCode 15.2 and emplace_back
  PlaylistMarkInformation(BLURAY_MARK_TYPE newMarkType,
                          unsigned int newPlayItemReference,
                          std::chrono::milliseconds newTime,
                          unsigned int newElementaryStreamPacketIdentifier,
                          std::chrono::milliseconds newDuration);

  BLURAY_MARK_TYPE markType{0};
  unsigned int playItemReference{0};
  std::chrono::milliseconds time{0ms};
  unsigned int elementaryStreamPacketIdentifier{0};
  std::chrono::milliseconds duration{0ms};
};

struct ChapterInformation
{
  // user-defined ctor required for XCode 15.2 and emplace_back
  ChapterInformation(unsigned int newChapter,
                     std::chrono::milliseconds newStart,
                     std::chrono::milliseconds newDuration);

  unsigned int chapter{0};
  std::chrono::milliseconds start{0ms};
  std::chrono::milliseconds duration{0ms};
};

struct SubPlayItemInformation
{
  BLURAY_CONNECTION connectionCondition{0};
  bool isMultiClip{false};
  std::chrono::milliseconds inTime{0ms};
  std::chrono::milliseconds outTime{0ms};
  unsigned int syncPlayItemId{0};
  std::vector<ClipInformation> clips;
};

struct SubPathInformation
{
  BLURAY_SUBPATH_TYPE type{0};
  bool repeat{false};
  std::vector<SubPlayItemInformation> subPlayItems;
};

struct PlayItemInformation
{
  bool isMultiAngle{false};
  std::vector<ClipInformation> angleClips;
  BLURAY_CONNECTION connectionCondition{0};
  std::chrono::milliseconds inTime{0ms};
  std::chrono::milliseconds outTime{0ms};
  bool randomAccessFlag{false};
  unsigned int stillMode{0};
  std::chrono::milliseconds stillTime{0ms};
  std::vector<StreamInformation> videoStreams;
  std::vector<StreamInformation> audioStreams;
  std::vector<StreamInformation> presentationGraphicStreams;
  std::vector<StreamInformation> interactiveGraphicStreams;
  std::vector<StreamInformation> secondaryAudioStreams;
  std::vector<StreamInformation> secondaryVideoStreams;
  std::vector<StreamInformation> dolbyVisionStreams;
};

class CMPLSParser
{
public:
  static bool ReadMPLS(const CURL& url,
                       unsigned int playlist,
                       BlurayPlaylistInformation& playlistInformation,
                       std::map<unsigned int, ClipInformation>& clipCache);
};
} // namespace XFILE