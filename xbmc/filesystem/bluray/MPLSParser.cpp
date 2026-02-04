/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MPLSParser.h"

#include "BitReader.h"
#include "PlaylistStructure.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <libbluray/bluray.h>

namespace XFILE
{
namespace
{
constexpr unsigned int OFFSET_STREAM_TYPE = 1;
constexpr unsigned int OFFSET_PLAYITEM_PACKET_IDENTIFIER = 2;
constexpr unsigned int OFFSET_SUBPATH_SUBPATH_ID = 2;
constexpr unsigned int OFFSET_SUBPATH_SUBCLIP_ID = 3;
constexpr unsigned int OFFSET_SUBPATH_PACKET_IDENTIFIER = 4;
constexpr unsigned int OFFSET_DV_PIP_SUBPATH_ID = 2;
constexpr unsigned int OFFSET_DV_PIP_PACKET_IDENTIFIER = 3;
constexpr unsigned int OFFSET_STREAM_CODING = 1;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_1 = 2;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_2 = 3;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_3 = 4;
constexpr unsigned int OFFSET_STREAM_AUDIO_FLAG_1 = 2;
constexpr unsigned int OFFSET_STREAM_AUDIO_LANGUAGE = 3;
constexpr unsigned int OFFSET_STREAM_SUB_LANGUAGE = 2;
constexpr unsigned int OFFSET_STREAM_TEXT_ENCODING = 2;
constexpr unsigned int OFFSET_STREAM_TEXT_LANGUAGE = 3;
constexpr unsigned int OFFSET_STREAM_SECONDARY_REFERENCES = 2;

StreamInformation ParseStream(const std::span<std::byte> buffer,
                              unsigned int& offset,
                              STREAM_TYPE streamType)
{
  StreamInformation streamInformation;
  unsigned int length{GetByte(buffer, offset)};

  const BLURAY_STREAM_TYPE type{GetByte(buffer, offset + OFFSET_STREAM_TYPE)};
  streamInformation.type = type;
  switch (type)
  {
    using enum BLURAY_STREAM_TYPE;
    case PLAYITEM: // a stream of the clip used by the PlayItem
    {
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_PLAYITEM_PACKET_IDENTIFIER);
      break;
    }
    case SUBPATH: // a stream of the clip used by a SubPath (types 2, 3, 4, 5, 6, 8 or 9)
    {
      streamInformation.subpathId = GetWord(buffer, offset + OFFSET_SUBPATH_SUBPATH_ID);
      streamInformation.subclipId = GetWord(buffer, offset + OFFSET_SUBPATH_SUBCLIP_ID);
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_SUBPATH_PACKET_IDENTIFIER);
      break;
    }
    case SUBPATH_INMUX_SYNCHRONOUS_PIP: // a stream of the clip used by a SubPath (type 7)
    case SUBPATH_DOLBY_VISION_LAYER: // a stream of the clip used by a SubPath (type 10)
    {
      streamInformation.subpathId = GetWord(buffer, offset + OFFSET_DV_PIP_SUBPATH_ID);
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_DV_PIP_PACKET_IDENTIFIER);
      break;
    }
  }
  offset += length + 1;

  length = GetByte(buffer, offset);
  const ENCODING_TYPE coding{GetByte(buffer, offset + OFFSET_STREAM_CODING)};
  streamInformation.coding = coding;
  switch (coding)
  {
    using enum ENCODING_TYPE;
    case VIDEO_MPEG2:
    case VIDEO_H264:
    case VIDEO_HEVC:
    case VIDEO_VC1:
    {
      unsigned int flag{GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_1)};
      streamInformation.format = GetBits(flag, 8, 4);
      streamInformation.rate = GetBits(flag, 4, 4);
      if (coding == VIDEO_HEVC)
      {
        flag = GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_2);
        streamInformation.dynamicRangeType = GetBits(flag, 8, 4);
        streamInformation.colorSpace = GetBits(flag, 4, 4);
        flag = GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_3);
        streamInformation.copyRestricted = GetBits(flag, 8, 1);
        streamInformation.HDRPlus = GetBits(flag, 7, 1);
      }
      break;
    }
    case AUDIO_LPCM:
    case AUDIO_AC3:
    case AUDIO_DTS:
    case AUDIO_TRUHD:
    case AUDIO_AC3PLUS:
    case AUDIO_DTSHD:
    case AUDIO_DTSHD_MASTER:
    case AUDIO_AC3PLUS_SECONDARY:
    case AUDIO_DTSHD_SECONDARY:
    {
      const unsigned int flag{GetByte(buffer, offset + OFFSET_STREAM_AUDIO_FLAG_1)};
      streamInformation.format = GetBits(flag, 8, 4);
      streamInformation.rate = GetBits(flag, 4, 4);
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_AUDIO_LANGUAGE, 3);
      break;
    }
    case SUB_PG:
    case SUB_IG:
    {
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_SUB_LANGUAGE, 3);
      break;
    }
    case SUB_TEXT:
    {
      streamInformation.characterEncoding = GetByte(buffer, offset + OFFSET_STREAM_TEXT_ENCODING);
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_TEXT_LANGUAGE, 3);
      break;
    }
    default:
      break;
  }
  offset += length + 1;

  switch (streamType)
  {
    using enum STREAM_TYPE;
    case SECONDARY_AUDIO_STREAM:
    {
      const unsigned int numAudioReferences{GetByte(buffer, offset)};
      streamInformation.secondaryAudio_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryAudio_audioReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numAudioReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numAudioReferences % 2); // next word boundary
      break;
    }
    case SECONDARY_VIDEO_STREAM:
    {
      const unsigned int numAudioReferences{GetByte(buffer, offset)};
      streamInformation.secondaryVideo_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryVideo_audioReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numAudioReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numAudioReferences % 2); // next word boundary
      const unsigned int numPGReferences{GetByte(buffer, offset)};
      streamInformation.secondaryVideo_presentationGraphicReferences.reserve(numPGReferences);
      for (unsigned int i = 0; i < numPGReferences; ++i)
      {
        streamInformation.secondaryVideo_presentationGraphicReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numPGReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numPGReferences % 2); // next word boundary
      break;
    }
    case VIDEO_STREAM:
    case AUDIO_STREAM:
    case PRESENTATION_GRAPHIC_STREAM:
    case INTERACTIVE_GRAPHIC_STREAM:
    case PICTURE_IN_PICTURE_SUBTITLE_STREAM:
    case DOLBY_VISION_STREAM:
      break;
  }

  CLog::LogFC(LOGDEBUG, LOGBLURAY, "  Stream - type {}, coding 0x{}",
              static_cast<unsigned int>(streamInformation.type),
              fmt::format("{:02x}", static_cast<int>(streamInformation.coding)));

  return streamInformation;
}

constexpr unsigned int CLPI_HEADER_SIZE = 28;
constexpr unsigned int OFFSET_CLPI_HEADER = 0;
constexpr unsigned int OFFSET_CLPI_VERSION = 4;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION = 12;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_PROGRAMS = 5;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_SEQUENCE_START = 0;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_PROGRAM_ID = 4;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_STREAMS = 6;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_GROUPS = 7;
constexpr unsigned int OFFSET_CLPI_STREAM_INFORMATION_CODING = 1;
constexpr unsigned int OFFSET_CLPI_STREAM_VIDEO_FLAGS = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_AUDIO_FLAGS = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_AUDIO_LANGUAGE = 3;
constexpr unsigned int OFFSET_CLPI_STREAM_SUB_LANGUAGE = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_TEXT_ENCODING = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_TEXT_LANGUAGE = 3;

void ParseProgramInformation(std::vector<std::byte>& buffer,
                             unsigned int& offset,
                             ProgramInformation& programInformation)
{
  programInformation.spnProgramSequenceStart =
      GetDWord(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_SEQUENCE_START);
  programInformation.programId =
      GetWord(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_PROGRAM_ID);
  const unsigned int numStreams{
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_STREAMS)};
  programInformation.numGroups =
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_GROUPS);
  offset += 8;

  programInformation.streams.reserve(numStreams);
  for (unsigned int j = 0; j < numStreams; ++j)
  {
    StreamInformation& streamInformation = programInformation.streams.emplace_back();

    streamInformation.packetIdentifier = GetWord(buffer, offset);
    offset += 2;

    const unsigned int streamLength{GetByte(buffer, offset)};
    const ENCODING_TYPE coding{GetByte(buffer, offset + OFFSET_CLPI_STREAM_INFORMATION_CODING)};
    streamInformation.coding = coding;
    switch (coding)
    {
      using enum ENCODING_TYPE;
      case VIDEO_MPEG2:
      case VIDEO_H264:
      case VIDEO_HEVC:
      case VIDEO_VC1:
      {
        const unsigned int flag{GetDWord(buffer, offset + OFFSET_CLPI_STREAM_VIDEO_FLAGS)};
        streamInformation.format = GetBits(flag, 32, 4);
        streamInformation.rate = GetBits(flag, 28, 4);
        streamInformation.aspect = static_cast<ASPECT_RATIO>(GetBits(flag, 24, 4));
        streamInformation.outOfMux = GetBits(flag, 18, 1);
        if (coding == VIDEO_HEVC)
        {
          streamInformation.copyRestricted = GetBits(flag, 17, 1);
          streamInformation.dynamicRangeType = GetBits(flag, 16, 4);
          streamInformation.colorSpace = GetBits(flag, 12, 4);
          streamInformation.HDRPlus = GetBits(flag, 8, 1);
        }
        break;
      }
      case AUDIO_LPCM:
      case AUDIO_AC3:
      case AUDIO_DTS:
      case AUDIO_TRUHD:
      case AUDIO_AC3PLUS:
      case AUDIO_DTSHD:
      case AUDIO_DTSHD_MASTER:
      case AUDIO_AC3PLUS_SECONDARY:
      case AUDIO_DTSHD_SECONDARY:
      {
        const unsigned int flag{GetByte(buffer, offset + OFFSET_CLPI_STREAM_AUDIO_FLAGS)};
        streamInformation.format = GetBits(flag, 8, 4);
        streamInformation.rate = GetBits(flag, 4, 4);
        streamInformation.language =
            GetString(buffer, offset + OFFSET_CLPI_STREAM_AUDIO_LANGUAGE, 3);
        break;
      }
      case SUB_PG:
      case SUB_IG:
      {
        streamInformation.language = GetString(buffer, offset + OFFSET_CLPI_STREAM_SUB_LANGUAGE, 3);
        break;
      }
      case SUB_TEXT:
      {
        streamInformation.characterEncoding =
            GetByte(buffer, offset + OFFSET_CLPI_STREAM_TEXT_ENCODING);
        streamInformation.language =
            GetString(buffer, offset + OFFSET_CLPI_STREAM_TEXT_LANGUAGE, 3);
        break;
      }
      default:
        break;
    }

    CLog::LogFC(LOGDEBUG, LOGBLURAY, "  Stream - coding 0x{:02x}",
                static_cast<int>(streamInformation.coding));

    offset += streamLength + 1;
  }
}

bool ParseCLPI(std::vector<std::byte>& buffer, ClipInformation& clipInformation, unsigned int clip)
{
  // Check size
  if (buffer.size() < CLPI_HEADER_SIZE)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid CLPI - header too small");
    return false;
  }

  // Check header
  const std::string header{GetString(buffer, OFFSET_CLPI_HEADER, 4)};
  const std::string version{GetString(buffer, OFFSET_CLPI_VERSION, 4)};
  if (header != "HDMV")
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid CLPI header");
    return false;
  }
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Valid CLPI header for clip {} header version {}", clip,
              version);

  clipInformation.version = version;
  clipInformation.clip = clip;

  const unsigned int programInformationStartAddress{
      GetDWord(buffer, OFFSET_CLPI_PROGRAM_INFORMATION)};
  unsigned int offset{programInformationStartAddress};
  if (const unsigned int length{GetDWord(buffer, offset)}; buffer.size() < length + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid CLPI - too small for Program Information");
    return false;
  }

  const unsigned int numPrograms{
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_PROGRAMS)};
  offset += 6;
  clipInformation.programs.reserve(numPrograms);
  for (unsigned int i = 0; i < numPrograms; ++i)
  {
    ProgramInformation& programInformation = clipInformation.programs.emplace_back();
    CLog::LogFC(LOGDEBUG, LOGBLURAY, " Program {}", i);
    ParseProgramInformation(buffer, offset, programInformation);
  }

  return true;
}

bool ReadCLPI(const CURL& url, unsigned int clip, ClipInformation& clipInformation)
{
  const std::string& path{url.GetHostName()};
  const std::string clipFile{
      URIUtils::AddFileToFolder(path, "BDMV", "CLIPINF", fmt::format("{:05}.clpi", clip))};
  CFile file;
  if (!file.Open(clipFile))
    return false;

  const int64_t size{file.GetLength()};
  std::vector<std::byte> buffer;
  buffer.resize(size);
  ssize_t read = file.Read(buffer.data(), size);
  file.Close();

  if (read == size)
    return ParseCLPI(buffer, clipInformation, clip);
  return false;
}

constexpr unsigned int PLAYITEM_HEADER_SIZE = 18;
constexpr unsigned int OFFSET_PLAYITEM_CLIP_ID = 2;
constexpr unsigned int OFFSET_PLAYITEM_CODEC_ID = 7;
constexpr unsigned int OFFSET_PLAYITEM_FLAGS = 11;
constexpr unsigned int OFFSET_PLAYITEM_IN_TIME = 14;
constexpr unsigned int OFFSET_PLAYITEM_OUT_TIME = 18;
constexpr unsigned int OFFSET_PLAYITEM_FLAGS_2 = 30;
constexpr unsigned int OFFSET_PLAYITEM_STILL_MODE = 31;
constexpr unsigned int OFFSET_PLAYITEM_STILL_TIME = 32;
constexpr unsigned int OFFSET_PLAYITEM_NUM_ANGLES = 34;
constexpr unsigned int OFFSET_ANGLE_CODEC_ID = 5;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_VIDEO_STREAMS = 4;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_AUDIO_STREAMS = 5;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_PG_STREAMS = 6;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_IG_STREAMS = 7;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_SECONDARY_VIDEO_STREAMS = 8;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_SECONDARY_AUDIO_STREAMS = 9;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_PIP_STREAMS = 10;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_DV_STREAMS = 11;

bool ParsePlayItem(std::vector<std::byte>& buffer,
                   unsigned int& offset,
                   PlayItemInformation& playItem)
{
  const unsigned int saveOffset{offset};
  const unsigned int length{GetWord(buffer, offset)};
  if (length < PLAYITEM_HEADER_SIZE)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - Playitem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for Playitem");
    return false;
  }

  std::string clipId{GetString(buffer, offset + OFFSET_PLAYITEM_CLIP_ID, 5)};
  std::string codecId{GetString(buffer, offset + OFFSET_PLAYITEM_CODEC_ID, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - invalid PlayItem codec identifier - {}",
                codecId);
    return false;
  }

  unsigned int flags{GetWord(buffer, offset + OFFSET_PLAYITEM_FLAGS)};
  const bool isMultiAngle{static_cast<bool>(GetBits(flags, 5, 1))};
  const BLURAY_CONNECTION connectionCondition{GetBits(flags, 4, 4)};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - invalid PlayItem connection condition - {}",
                static_cast<unsigned int>(connectionCondition));
    return false;
  }
  playItem.isMultiAngle = isMultiAngle;
  playItem.connectionCondition = connectionCondition;

  const std::chrono::milliseconds inTime{GetDWord(buffer, offset + OFFSET_PLAYITEM_IN_TIME) /
                                         45}; // 45KHz clock
  const std::chrono::milliseconds outTime{GetDWord(buffer, offset + OFFSET_PLAYITEM_OUT_TIME) /
                                          45}; // 45KHz clock
  playItem.inTime = inTime;
  playItem.outTime = outTime;

  flags = GetByte(buffer, offset + OFFSET_PLAYITEM_FLAGS_2);
  const bool randomAccessFlag{static_cast<bool>(GetBits(flags, 8, 1))};
  const unsigned int stillMode{GetByte(buffer, offset + OFFSET_PLAYITEM_STILL_MODE)};
  std::chrono::milliseconds stillTime{0ms};
  if (stillMode == BLURAY_STILL_TIME)
    stillTime = std::chrono::milliseconds(GetWord(buffer, offset + OFFSET_PLAYITEM_STILL_TIME));
  playItem.randomAccessFlag = randomAccessFlag;
  playItem.stillMode = stillMode;
  playItem.stillTime = stillTime;

  unsigned int angleCount{1};
  if (isMultiAngle)
  {
    angleCount = GetByte(buffer, offset + OFFSET_PLAYITEM_NUM_ANGLES);
    offset += 36;
  }
  else
    offset += 34;

  CLog::LogFC(LOGDEBUG, LOGBLURAY,
              "PlayItem entry - clip id {}, codec id {}, in time {}, out time {}, still mode "
              "{} (still time {}), angles {}",
              clipId, codecId, fmt::format("{:%H:%M:%S}", inTime),
              fmt::format("{:%H:%M:%S}", outTime), stillMode, stillTime, angleCount);

  // First/only angle
  ClipInformation angleInformation;
  angleInformation.clip = std::stoi(clipId);
  angleInformation.codec = codecId;
  playItem.angleClips.emplace_back(angleInformation);

  // Get additional angles (if any)
  for (unsigned int j = 1; j < angleCount; ++j)
  {
    ClipInformation additionalAngleInformation;
    const std::string angleClipId{GetString(buffer, offset, 5)};
    const std::string angleCodecId{GetString(buffer, offset + OFFSET_ANGLE_CODEC_ID, 4)};
    if (angleCodecId != "M2TS" && angleCodecId != "FMTS")
    {
      CLog::LogFC(LOGDEBUG, LOGBLURAY,
                  "Invalid MPLS - invalid PlayItem angle {} codec identifier - {}", j,
                  angleCodecId);
      return false;
    }
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "  Additional angle {} - clip id {}, codec id {}", j,
                angleClipId, angleCodecId);

    additionalAngleInformation.clip = std::stoi(angleClipId);
    additionalAngleInformation.codec = angleCodecId;
    playItem.angleClips.emplace_back(additionalAngleInformation);
    offset += 10;
  }

  // Parse stream number table
  if (const unsigned int stnLength{GetWord(buffer, offset)}; buffer.size() < stnLength + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for Stream Number Table");
    return false;
  }

  const unsigned int numVideoStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_VIDEO_STREAMS)};
  const unsigned int numAudioStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_AUDIO_STREAMS)};
  const unsigned int numPresentationGraphicStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_PG_STREAMS)};
  const unsigned int numInteractiveGraphicStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_IG_STREAMS)};
  const unsigned int numSecondaryVideoStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_SECONDARY_VIDEO_STREAMS)};
  const unsigned int numSecondaryAudioStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_SECONDARY_AUDIO_STREAMS)};
  const unsigned int numPictureInPictureSubtitleStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_PIP_STREAMS)};
  const unsigned int numDolbyVisionStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_DV_STREAMS)};

  CLog::LogFC(LOGDEBUG, LOGBLURAY,
              " Stream number table - video {}, audio {}, presentation graphic (subtitle) {}, "
              "interactive graphic {}, "
              "secondary video {}, secondary audio {}, PIP subtitle {}, dolby vision {}",
              numVideoStreams, numAudioStreams, numPresentationGraphicStreams,
              numInteractiveGraphicStreams, numSecondaryVideoStreams, numSecondaryAudioStreams,
              numPictureInPictureSubtitleStreams, numDolbyVisionStreams);

  offset += 16;

  playItem.videoStreams.reserve(numVideoStreams);
  for (unsigned int k = 0; k < numVideoStreams; ++k)
  {
    playItem.videoStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::VIDEO_STREAM));
  }

  playItem.audioStreams.reserve(numAudioStreams);
  for (unsigned int k = 0; k < numAudioStreams; ++k)
  {
    playItem.audioStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::AUDIO_STREAM));
  }

  playItem.presentationGraphicStreams.reserve(numPresentationGraphicStreams);
  for (unsigned int k = 0; k < numPresentationGraphicStreams; ++k)
  {
    playItem.presentationGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::PRESENTATION_GRAPHIC_STREAM));
  }

  playItem.interactiveGraphicStreams.reserve(numInteractiveGraphicStreams);
  for (unsigned int k = 0; k < numInteractiveGraphicStreams; ++k)
  {
    playItem.interactiveGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::INTERACTIVE_GRAPHIC_STREAM));
  }

  playItem.secondaryAudioStreams.reserve(numSecondaryAudioStreams);
  for (unsigned int k = 0; k < numSecondaryAudioStreams; ++k)
  {
    playItem.secondaryAudioStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_AUDIO_STREAM));
  }

  playItem.secondaryVideoStreams.reserve(numSecondaryVideoStreams);
  for (unsigned int k = 0; k < numSecondaryVideoStreams; ++k)
  {
    playItem.secondaryVideoStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_VIDEO_STREAM));
  }

  playItem.dolbyVisionStreams.reserve(numDolbyVisionStreams);
  for (unsigned int k = 0; k < numDolbyVisionStreams; ++k)
  {
    playItem.dolbyVisionStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::DOLBY_VISION_STREAM));
  }

  offset = saveOffset + length + 2;
  return true;
}

constexpr unsigned int SUBPLAYITEM_HEADER_SIZE = 24;
constexpr unsigned int OFFSET_SUBPLAYITEM_CLIP_ID = 2;
constexpr unsigned int OFFSET_SUBPLAYITEM_CODEC_ID = 7;
constexpr unsigned int OFFSET_SUBPLAYITEM_FLAGS = 11;
constexpr unsigned int OFFSET_SUBPLAYITEM_IN_TIME = 16;
constexpr unsigned int OFFSET_SUBPLAYITEM_OUT_TIME = 20;
constexpr unsigned int OFFSET_SUBPLAYITEM_SYNC_PLAYITEM_ID = 24;
constexpr unsigned int OFFSET_SUBPLAYITEM_NUM_CLIPS = 26;
constexpr unsigned int OFFSET_SUBPLAYITEM_CLIP_CODEC_ID = 5;

bool ParseSubPlayItem(std::vector<std::byte>& buffer,
                      unsigned int& offset,
                      SubPlayItemInformation& subPlayItemInformation)
{
  const unsigned int saveOffset{offset};
  unsigned int length{GetWord(buffer, offset)};
  if (length < SUBPLAYITEM_HEADER_SIZE)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - SubPlayItem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for SubPlayItem");
    return false;
  }

  std::string clipId{GetString(buffer, offset + OFFSET_SUBPLAYITEM_CLIP_ID, 5)};
  std::string codecId{GetString(buffer, offset + OFFSET_SUBPLAYITEM_CODEC_ID, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - invalid PlayItem codec identifier - {}",
                codecId);
    return false;
  }

  unsigned int flags{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_FLAGS)};
  const bool isMultiClip{static_cast<bool>(GetBits(flags, 1, 1))};
  const BLURAY_CONNECTION connectionCondition{GetBits(flags, 5, 4)};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - invalid PlayItem connection condition - {}",
                static_cast<unsigned int>(connectionCondition));
    return false;
  }
  subPlayItemInformation.isMultiClip = isMultiClip;
  subPlayItemInformation.connectionCondition = connectionCondition;

  const std::chrono::milliseconds inTime{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_IN_TIME) /
                                         45}; // 45KHz clock
  const std::chrono::milliseconds outTime{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_OUT_TIME) /
                                          45}; // 45KHz clock
  subPlayItemInformation.inTime = inTime;
  subPlayItemInformation.outTime = outTime;

  const unsigned int syncPlayItemId{GetWord(buffer, offset + OFFSET_SUBPLAYITEM_SYNC_PLAYITEM_ID)};
  subPlayItemInformation.syncPlayItemId = syncPlayItemId;

  unsigned int numClips{1};
  if (isMultiClip)
  {
    numClips = GetByte(buffer, offset + OFFSET_SUBPLAYITEM_NUM_CLIPS);
    offset += 27;
  }
  else
    offset += 26;

  CLog::LogFC(LOGDEBUG, LOGBLURAY,
              " SubPlayItem entry - clip id {}, codec id {}, in time {}, out time {}, clips {}",
              clipId, codecId, fmt::format("{:%H:%M:%S}", inTime),
              fmt::format("{:%H:%M:%S}", outTime), numClips);

  // First/only clip
  ClipInformation clipInformation;
  clipInformation.clip = std::stoi(clipId);
  clipInformation.codec = codecId;
  subPlayItemInformation.clips.emplace_back(clipInformation);

  // Get additional clips (if any)
  for (unsigned int j = 1; j < numClips; ++j)
  {
    ClipInformation additionalClipInformation;
    const std::string additionalClipId{GetString(buffer, offset, 5)};
    const std::string additionalCodecId{
        GetString(buffer, offset + OFFSET_SUBPLAYITEM_CLIP_CODEC_ID, 4)};
    if (additionalCodecId != "M2TS" && additionalCodecId != "FMTS")
    {
      CLog::LogFC(LOGDEBUG, LOGBLURAY,
                  "Invalid MPLS - invalid SubPlayItem clip {} codec identifier - {}", j,
                  additionalCodecId);
      return false;
    }
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "  Additional clip {} - clip id {}, codec id {}", j,
                additionalClipId, additionalCodecId);

    additionalClipInformation.clip = std::stoi(additionalClipId);
    additionalClipInformation.codec = additionalCodecId;
    subPlayItemInformation.clips.emplace_back(additionalClipInformation);
    offset += 10;
  }

  offset = saveOffset + length + 2;

  return true;
}

// Structure of an MPLS File:
//
// Header: Contains general information about the playlist.
//
// PlayItems: Each PlayItem corresponds to a specific .m2ts file and includes details like
// In Time, Out Time, and System Time Clock ID for synchronization.
//
// SubPath: Optional, used for features like Picture-in-Picture (PiP) or secondary audio.

constexpr unsigned int MPLS_HEADER_SIZE = 40;
constexpr unsigned int MPLS_PLAYLISTMARK_SIZE = 14;
constexpr unsigned int OFFSET_MPLS_HEADER = 0;
constexpr unsigned int OFFSET_MPLS_VERSION = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLIST_POSITION = 8;
constexpr unsigned int OFFSET_MPLS_PLAYLIST_MARK_POSITION = 12;
constexpr unsigned int OFFSET_MPLS_EXTENSION_DATA_POSITION = 16;
constexpr unsigned int OFFSET_MPLS_APP_INFO_PLAYLIST = 40;
constexpr unsigned int OFFSET_MPLS_APP_INFO_PLAYBACK_TYPE = 5;
constexpr unsigned int OFFSET_MPLS_NUM_PLAYITEMS = 6;
constexpr unsigned int OFFSET_MPLS_NUM_SUBPATHS = 8;
constexpr unsigned int OFFSET_MPLS_SUBPATH_NUM_SUBPLAYITEMS = 9;
constexpr unsigned int OFFSET_MPLS_NUM_PLAYLISTMARKS = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_TYPE = 1;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_PLAYITEM = 2;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_TIME = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_PACKET_IDENTIFIER = 8;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_DURATION = 10;

bool ProcessClips(const CURL& url,
                  BlurayPlaylistInformation& playlistInformation,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  for (const auto& playItem : playlistInformation.playItems)
  {
    for (const auto& clip : playItem.angleClips)
    {
      if (const auto& it = clipCache.find(clip.clip); it != clipCache.end())
      {
        // In local cache
        playlistInformation.clips.push_back(it->second);
        continue;
      }

      // Not in local cache
      ClipInformation& clipInformation = playlistInformation.clips.emplace_back();
      if (!ReadCLPI(url, clip.clip, clipInformation))
      {
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Cannot read clip {} information", clip.clip);
        playlistInformation.clips.pop_back();
        return false;
      }
      playlistInformation.clips.emplace_back(clipInformation);
      clipCache[clip.clip] = clipInformation;
    }
  }
  return true;
}

bool ParseSubPath(std::vector<std::byte>& buffer,
                  unsigned int& offset,
                  std::vector<SubPlayItemInformation>& subPlayItems)
{
  if (const unsigned int subPathSize{GetDWord(buffer, offset)};
      buffer.size() < offset + subPathSize)
    return false;

  const unsigned int numSubPlayItems{
      GetByte(buffer, offset + OFFSET_MPLS_SUBPATH_NUM_SUBPLAYITEMS)};
  offset += 10;

  if (numSubPlayItems == 0)
    return true;

  subPlayItems.reserve(numSubPlayItems);
  for (unsigned int j = 0; j < numSubPlayItems; ++j)
  {
    SubPlayItemInformation& subPlayItem = subPlayItems.emplace_back();
    if (!ParseSubPlayItem(buffer, offset, subPlayItem))
    {
      subPlayItems.pop_back();
      return false;
    }
  }

  return true;
}

bool ParsePlaylist(const CURL& url,
                   std::vector<std::byte>& buffer,
                   unsigned int& offset,
                   BlurayPlaylistInformation& playlistInformation,
                   std::map<unsigned int, ClipInformation>& clipCache)
{
  if (const unsigned int playlistSize{GetDWord(buffer, offset)};
      buffer.size() < playlistSize + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for Playlist");
    return false;
  }
  const unsigned int numPlayItems{GetWord(buffer, offset + OFFSET_MPLS_NUM_PLAYITEMS)};
  const unsigned int numSubPaths{GetWord(buffer, offset + OFFSET_MPLS_NUM_SUBPATHS)};
  offset += 10;

  if (numPlayItems > 0)
  {
    playlistInformation.playItems.reserve(numPlayItems);
    for (unsigned int i = 0; i < numPlayItems; ++i)
    {
      PlayItemInformation& playItem = playlistInformation.playItems.emplace_back();
      if (!ParsePlayItem(buffer, offset, playItem))
      {
        playlistInformation.playItems.pop_back();
        return false;
      }
    }

    // Calculate duration
    std::chrono::milliseconds duration{0ms};
    for (const auto& playItem : playlistInformation.playItems)
      duration += playItem.outTime - playItem.inTime;
    playlistInformation.duration = duration;
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Playlist duration {}", fmt::format("{:%H:%M:%S}", duration));

    // Process clips
    if (!ProcessClips(url, playlistInformation, clipCache))
      return false;
  }

  if (numSubPaths > 0)
    for (unsigned int i = 0; i < numSubPaths; ++i)
      ParseSubPath(buffer, offset, playlistInformation.subPlayItems);

  return true;
}

bool ParsePlaylistMark(std::vector<std::byte>& buffer,
                       unsigned int& offset,
                       BlurayPlaylistInformation& playlistInformation)
{
  if (const unsigned int playlistMarkSize{GetDWord(buffer, offset)};
      buffer.size() < playlistMarkSize + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  const unsigned int numPlaylistMarks{GetWord(buffer, offset + OFFSET_MPLS_NUM_PLAYLISTMARKS)};
  if (buffer.size() < (numPlaylistMarks * MPLS_PLAYLISTMARK_SIZE) + offset + 6)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  offset += 6;
  playlistInformation.playlistMarks.reserve(numPlaylistMarks);
  for (unsigned int i = 0; i < numPlaylistMarks; ++i)
  {
    PlaylistMarkInformation playlistMark{
        .markType =
            static_cast<BLURAY_MARK_TYPE>(GetByte(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_TYPE)),
        .playItemReference = GetWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_PLAYITEM),
        .time = std::chrono::milliseconds(GetDWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_TIME) /
                                          45), // 45KHz clock
        .elementaryStreamPacketIdentifier =
            GetWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_PACKET_IDENTIFIER),
        .duration = std::chrono::milliseconds(
            GetDWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_DURATION))};
    playlistInformation.playlistMarks.emplace_back(playlistMark);
    offset += MPLS_PLAYLISTMARK_SIZE;
  }

  return true;
}

bool ParseExtensionData(std::vector<std::byte>& buffer,
                        unsigned int& offset,
                        BlurayPlaylistInformation& playlistInformation)
{
  const unsigned int extensionDataPosition{offset};
  const unsigned int extensionDataSize{GetDWord(buffer, offset)};
  if (extensionDataSize == 0)
    return true;
  if (buffer.size() < offset + extensionDataSize)
    return false;

  const unsigned int numberOfExtDataEntries{GetByte(buffer, offset + 11)};
  offset += 12;
  for (unsigned int i = 0; i < numberOfExtDataEntries; ++i)
  {
    const unsigned int extDataType{GetWord(buffer, offset)};
    const unsigned int extDataVersion{GetWord(buffer, offset + 2)};
    if (extDataType == 2 && extDataVersion == 2)
    {
      const unsigned int extDataStartAddress{GetDWord(buffer, offset + 4)};
      const unsigned int extDataLength{GetDWord(buffer, offset + 8)};
      if (extensionDataPosition + extDataStartAddress + extDataLength <= buffer.size())
      {
        unsigned int subPathOffset{extensionDataPosition + extDataStartAddress};
        const unsigned int numSubPathItems{GetWord(buffer, subPathOffset + 4)};
        subPathOffset += 6;

        for (unsigned int j = 0; j < numSubPathItems; ++j)
          if (!ParseSubPath(buffer, subPathOffset, playlistInformation.extensionSubPlayItems))
            return false;
      }
    }
    offset += 12;
  }

  return true;
}

void DeriveChaptersAndTimings(BlurayPlaylistInformation& playlistInformation)
{
  // Update clip timings from corresponding playItems
  std::chrono::milliseconds startTime{0ms};
  for (unsigned int i = 0; i < playlistInformation.playItems.size(); ++i)
  {
    const auto& playItem = playlistInformation.playItems[i];
    auto& clip = playlistInformation.clips[i];
    clip.duration = playItem.outTime - playItem.inTime;
    clip.time = startTime;
    startTime += clip.duration;
  }

  // Update playMark timings
  int prevChapter{-1};
  for (int i = 0; i < static_cast<int>(playlistInformation.playlistMarks.size()); ++i)
  {
    auto& playlistMark{playlistInformation.playlistMarks[i]};

    // Get referenced playItem and clip to calculate playMark time
    const auto& clip = playlistInformation.clips[playlistMark.playItemReference];
    const auto& playItem = playlistInformation.playItems[playlistMark.playItemReference];
    playlistMark.time = clip.time + playlistMark.time - playItem.inTime;

    if (playlistMark.markType != BLURAY_MARK_TYPE::ENTRY) // chapter
      continue;

    if (prevChapter >= 0)
    {
      auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
      if (prevMark.duration == 0ms)
        prevMark.duration = playlistMark.time - prevMark.time;
    }
    prevChapter = i;
  }
  if (prevChapter >= 0 && playlistInformation.playlistMarks[prevChapter].duration == 0ms)
  {
    auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
    prevMark.duration = playlistInformation.duration - prevMark.time;
  }

  // Derive chapters from playMarks
  unsigned int chapter{1};
  std::chrono::milliseconds start{0ms};
  for (const auto& playlistMark : playlistInformation.playlistMarks)
  {
    if (playlistMark.markType == BLURAY_MARK_TYPE::ENTRY)
    {
      ChapterInformation chapterInformation{
          .chapter = chapter, .start = start, .duration = playlistMark.duration};
      playlistInformation.chapters.emplace_back(chapterInformation);
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Chapter {} start {} duration {} end {}", chapter,
                  fmt::format("{:%H:%M:%S}", start),
                  fmt::format("{:%H:%M:%S}", chapterInformation.duration),
                  fmt::format("{:%H:%M:%S}", start + chapterInformation.duration));
      start += chapterInformation.duration;
      ++chapter;
    }
  }
}

bool ParseMPLS(const CURL& url,
               std::vector<std::byte>& buffer,
               BlurayPlaylistInformation& playlistInformation,
               unsigned int playlist,
               std::map<unsigned int, ClipInformation>& clipCache)
{
  // Check size
  if (buffer.size() < MPLS_HEADER_SIZE)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - header too small");
    return false;
  }

  // Check header
  const std::string header{GetString(buffer, OFFSET_MPLS_HEADER, 4)};
  const std::string version{GetString(buffer, OFFSET_MPLS_VERSION, 4)};
  if (header != "MPLS")
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS header");
    return false;
  }
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "*** Valid MPLS header for playlist {} version {}", playlist,
              version);

  playlistInformation.playlist = playlist;
  playlistInformation.version = version;

  const unsigned int playlistPosition{GetDWord(buffer, OFFSET_MPLS_PLAYLIST_POSITION)};
  const unsigned int playlistMarkPosition{GetDWord(buffer, OFFSET_MPLS_PLAYLIST_MARK_POSITION)};
  const unsigned int extensionDataPosition{GetDWord(buffer, OFFSET_MPLS_EXTENSION_DATA_POSITION)};

  // AppInfoPlayList
  unsigned int offset{OFFSET_MPLS_APP_INFO_PLAYLIST};
  if (const unsigned int appInfoSize{GetDWord(buffer, offset)};
      buffer.size() < appInfoSize + offset)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Invalid MPLS - too small for AppInfoPlaylist");
    return false;
  }

  BLURAY_PLAYBACK_TYPE playbackType{GetByte(buffer, offset + OFFSET_MPLS_APP_INFO_PLAYBACK_TYPE)};
  unsigned int playbackCount{0};
  if (playbackType == BLURAY_PLAYBACK_TYPE::RANDOM || playbackType == BLURAY_PLAYBACK_TYPE::SHUFFLE)
    playbackCount = GetWord(buffer, offset + 6);
  playlistInformation.playbackType = playbackType;
  playlistInformation.playbackCount = playbackCount;

  // Parse Playlist
  offset = playlistPosition;
  if (!ParsePlaylist(url, buffer, offset, playlistInformation, clipCache))
    return false;

  // Parse PlayListMark
  offset = playlistMarkPosition;
  if (!ParsePlaylistMark(buffer, offset, playlistInformation))
    return false;

  // Parse extension data
  offset = extensionDataPosition;
  if (offset != 0 && !ParseExtensionData(buffer, offset, playlistInformation))
    return false;

  DeriveChaptersAndTimings(playlistInformation);

  return true;
}
} // namespace

bool CMPLSParser::ReadMPLS(const CURL& url,
                           unsigned int playlist,
                           BlurayPlaylistInformation& playlistInformation,
                           std::map<unsigned int, ClipInformation>& clipCache)
{
  try
  {
    const std::string& path{url.GetHostName()};
    const std::string playlistFile{
        URIUtils::AddFileToFolder(path, "BDMV", "PLAYLIST", fmt::format("{:05}.mpls", playlist))};
    CFile file;
    if (!file.Open(playlistFile))
      return false;

    const int64_t size{file.GetLength()};
    std::vector<std::byte> buffer;
    buffer.resize(size);
    ssize_t read{file.Read(buffer.data(), size)};
    file.Close();

    if (read == size)
      return ParseMPLS(url, buffer, playlistInformation, playlist, clipCache);
    return false;
  }
  catch (const std::out_of_range& e)
  {
    CLog::LogF(LOGERROR, "MPLS parsing failed - error {}", e.what());
    return false;
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "MPLS parsing failed - error {}", e.what());
    return false;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "MPLS parsing failed");
    return false;
  }
}
} // namespace XFILE
