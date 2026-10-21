/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>

class CDVDStreamInfo;
struct DemuxPacket;

namespace WEBCODECS
{
// How a stream is described to a WebCodecs VideoDecoder.
struct CodecConfiguration
{
  std::string codecString;
  // The stream's AVC or HEVC configuration record is passed as description.
  bool hasDescription{false};
  bool annexB{false};
  // The Annex B extradata holds parameter sets, which a decoder loses on
  // configure() and the stream may not repeat before its next key frame.
  bool prependExtradata{false};
  // Length prefix size of the NAL units when the packets are not Annex B.
  int nalLengthSize{0};
};

std::optional<CodecConfiguration> GetCodecConfiguration(const CDVDStreamInfo& hints);

// False for the profiles that allow chroma other than 4:2:0.
bool IsChroma420Profile(const CDVDStreamInfo& hints);

// WebCodecs needs 'key' to mean "decodable on its own". The demuxer's flag is
// not that: FFmpeg sets AV_PKT_FLAG_KEY on non-IDR I-frames (open GOP), whose
// leading pictures reference frames the decoder never saw, so the bitstream is
// parsed instead.
bool PacketIsKeyFrame(const CDVDStreamInfo& hints, const DemuxPacket& packet, int nalLengthSize);
} // namespace WEBCODECS
