/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/DolbyVisionRpu.h"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

extern "C"
{
#include <libavcodec/codec_id.h>
}

struct DemuxPacket;

/*!
 * \brief Reads from a video stream's packets what Kodi's FFmpeg decoder tells of the first picture
 * it outputs from them: whether it has HDR10+ metadata, and the Dolby Vision enhancement layer.
 *
 * The packets are taken as the decoder is fed them after a seek, from a keyframe on. They tell
 * only when every picture up to the one the decoder must have output by has the same: the 16th,
 * not counting the leading pictures an HEVC decoder skips. HEVC, AV1 and VP9 are read, and the
 * enhancement layer only from HEVC.
 */
class CVideoHdrBitstream
{
public:
  enum class State
  {
    MORE, //!< it takes more packets to tell
    DONE, //!< the packets tell
    UNDECIDED, //!< the packets can't tell
  };

  /*!
   * \param extraData the stream's extradata, from which the decoder learns how its NAL units are
   * framed
   * \param readEnhancementLayer whether to read the Dolby Vision enhancement layer too
   */
  CVideoHdrBitstream(AVCodecID codec,
                     std::span<const uint8_t> extraData,
                     bool readEnhancementLayer);

  //! Takes the stream's next packet.
  State Add(const DemuxPacket& packet);

  State GetState() const { return m_state; }
  int GetPackets() const { return m_packets; }

  //! Whether the picture has HDR10+ metadata, once the packets tell.
  bool HasHdr10Plus() const { return m_first && m_first->hdr10Plus; }

  //! The picture's enhancement layer, once the packets tell.
  KODI::UTILS::DOLBYVISION::EnhancementLayer GetEnhancementLayer() const
  {
    return m_first ? m_first->layer : KODI::UTILS::DOLBYVISION::EnhancementLayer::NONE;
  }

private:
  struct Picture
  {
    bool hdr10Plus{false};
    KODI::UTILS::DOLBYVISION::EnhancementLayer layer{
        KODI::UTILS::DOLBYVISION::EnhancementLayer::NONE};

    bool operator==(const Picture& other) const = default;
  };

  bool ReadHevcExtraData(std::span<const uint8_t> extraData);
  bool ReadHevc(std::span<const uint8_t> data, bool& leading);
  bool ReadHevcSei(std::span<const uint8_t> nal);

  const AVCodecID m_codec;
  const bool m_readEnhancementLayer;
  State m_state{State::MORE};
  int m_packets{0};
  int m_pictures{0};
  std::optional<Picture> m_first;

  // 0 for Annex B
  int m_nalLengthSize{0};
  // FFmpeg's HEVC decoder gives the last HDR10+ metadata it read to every picture after it
  bool m_hevcHdr10Plus{false};
  KODI::UTILS::DOLBYVISION::CRpuReader m_rpus;
  KODI::UTILS::DOLBYVISION::EnhancementLayer m_layer{
      KODI::UTILS::DOLBYVISION::EnhancementLayer::NONE};
  std::vector<uint8_t> m_buffer;
};
