/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoHdrBitstream.h"

#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/HevcSei.h"

#include <algorithm>
#include <array>
#include <memory>

extern "C"
{
#include <libavcodec/packet.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mem.h>
#include <libavutil/version.h>
}

using namespace KODI::UTILS::DOLBYVISION;

namespace
{
// The pictures from the keyframe that must agree. The decoder outputs one of them first, which
// one depending on their order: an HEVC decoder has output its first by the 16th it decodes, as
// at most 15 may wait to be reordered (sps_max_num_reorder_pics). AV1 and VP9 output theirs sooner.
constexpr int PICTURES{16};

constexpr uint8_t NAL_RASL_N{8};
constexpr uint8_t NAL_RASL_R{9};
constexpr uint8_t NAL_VCL_END{32};
constexpr uint8_t NAL_SEI_PREFIX{39};
constexpr uint8_t NAL_UNSPEC62{62};

constexpr uint8_t OBU_FRAME_HEADER{3};
constexpr uint8_t OBU_METADATA{5};
constexpr uint8_t OBU_FRAME{6};
constexpr uint64_t METADATA_TYPE_ITUT_T35{4};

struct Nal
{
  std::span<const uint8_t> data;
  uint8_t type;
  uint8_t layer;
  uint8_t temporalId;
};

/*!
 * \brief Splits an HEVC packet into the NAL units FFmpeg's decoder reads, as
 * ff_h2645_packet_split() does: framed by start codes when lengthSize is 0, else by sizes of that
 * many bytes.
 * \return false where FFmpeg fails to split it
 */
bool SplitHevc(std::span<const uint8_t> data, int lengthSize, std::vector<Nal>& nals)
{
  // The position of the first start code from pos that ends before last
  const auto findStartCode{
      [data](size_t pos, size_t last)
      {
        constexpr std::array<uint8_t, 3> START_CODE{0, 0, 1};
        const uint8_t* const end{data.data() + last};
        return static_cast<size_t>(
            std::search(data.data() + pos, end, START_CODE.begin(), START_CODE.end()) -
            data.data());
      }};

  nals.clear();
  size_t pos{0};
  while (data.size() - pos >= 4)
  {
    std::span<const uint8_t> nal;
    if (lengthSize > 0)
    {
      if (data.size() - pos <= static_cast<size_t>(lengthSize))
        return false;
      size_t size{0};
      for (int i = 0; i < lengthSize; ++i)
        size = (size << 8) | data[pos++];
      if (size == 0 || size > data.size() - pos)
        return false;
      nal = data.subspan(pos, size);
      pos += size;
    }
    else
    {
      // A start code with at least a byte after it, and the unit up to the next one
      const size_t start{findStartCode(pos, data.size() - 1)};
      if (start == data.size() - 1)
        return !nals.empty();
      const size_t end{findStartCode(start + 3, data.size())};
      nal = data.subspan(start + 3, end - start - 3);
      pos = end;
    }

    // FFmpeg skips a unit of zeros, of a single byte, or with a header it rejects
    if (nal.size() < 2 || std::ranges::all_of(nal, [](uint8_t byte) { return byte == 0; }))
      continue;
    const bool forbiddenZero{(nal[0] & 0x80) != 0};
    const auto layer{static_cast<uint8_t>(((nal[0] & 0x01) << 5) | (nal[1] >> 3))};
    const int temporalIdPlus1{nal[1] & 0x07};
    if (forbiddenZero || layer == 63 || temporalIdPlus1 == 0)
      continue;
    nals.push_back({nal, static_cast<uint8_t>((nal[0] >> 1) & 0x3F), layer,
                    static_cast<uint8_t>(temporalIdPlus1 - 1)});
  }
  return true;
}

std::span<const uint8_t> WithoutTrailingZeros(std::span<const uint8_t> data)
{
  while (!data.empty() && data.back() == 0)
    data = data.first(data.size() - 1);
  return data;
}

enum class T35
{
  OTHER,
  HDR10PLUS,
  INVALID, //!< FFmpeg fails to read the message
};

/*!
 * \brief Tells what FFmpeg's decoders make of an ITU-T T.35 message (libavcodec/itut35.c): HDR10+
 * metadata when it is SMPTE ST 2094-40 application 4 from Samsung, and libavutil parses it.
 * \param message the message after its country code
 */
T35 ReadT35(uint8_t countryCode, std::span<const uint8_t> message)
{
  constexpr uint8_t UNITED_KINGDOM{0xB4};
  constexpr uint8_t UNITED_STATES{0xB5};
  constexpr uint8_t CHINA{0x26};
  constexpr uint8_t EXTENDED{0xFF};

  size_t pos{0};
  const auto left{[&] { return message.size() - pos; }};
  const auto read16{
      [&]
      {
        const auto value{static_cast<uint16_t>((message[pos] << 8) | message[pos + 1])};
        pos += 2;
        return value;
      }};

  bool hdr10Plus{false};
  if (countryCode == UNITED_STATES)
  {
    if (left() < 2)
      return T35::INVALID;
    switch (read16())
    {
      case 0x0031: // ATSC
      {
        if (left() < 4)
          return T35::INVALID;
        const uint32_t high{read16()};
        const uint32_t identifier{(high << 16) | read16()};
        if (identifier == 0x44544731) // DTG1: active format description
        {
          if (left() < 2)
            return T35::INVALID;
          if (!(message[pos++] & 0x40))
            return T35::OTHER;
        }
        else if (identifier != 0x47413934) // GA94: closed captions
          return T35::OTHER;
        break;
      }
      case 0x5890: // AOM
        if (left() < 1)
          return T35::INVALID;
        if (message[pos++] != 0x01)
          return T35::OTHER;
        break;
      case 0x003C: // Samsung
        if (left() < 3)
          return T35::INVALID;
        if (read16() != 0x0001 || message[pos++] != 4)
          return T35::OTHER;
        hdr10Plus = true;
        break;
      case 0x003B: // Dolby
        if (left() < 4)
          return T35::INVALID;
        if (read16() != 0x0000 || read16() != 0x0800)
          return T35::OTHER;
        break;
      case 0x0090: // SMPTE
        if (left() < 2)
          return T35::INVALID;
        if (read16() != 0x0001)
          return T35::OTHER;
        break;
      default:
        return T35::OTHER;
    }
  }
  else if (countryCode == UNITED_KINGDOM)
  {
    if (left() < 3)
      return T35::INVALID;
    ++pos;
    if (read16() != 0x5000) // V-Nova
      return T35::OTHER;
  }
  else if (countryCode == CHINA)
  {
    if (left() < 2)
      return T35::INVALID;
    if (read16() != 0x0004) // HDR Vivid
      return T35::OTHER;
    if (left() < 2)
      return T35::INVALID;
    if (read16() != 0x0005)
      return T35::OTHER;
  }
  else if (countryCode == EXTENDED)
    return left() < 1 ? T35::INVALID : T35::OTHER;
  else
    return T35::OTHER;

  if (left() == 0)
    return T35::INVALID;
  if (!hdr10Plus)
    return T35::OTHER;

  size_t size{0};
  const std::unique_ptr<AVDynamicHDRPlus, decltype(&av_free)> metadata{
      av_dynamic_hdr_plus_alloc(&size), &av_free};
  if (!metadata || av_dynamic_hdr_plus_from_t35(metadata.get(), message.data() + pos, left()) < 0)
    return T35::INVALID;
  return T35::HDR10PLUS;
}

// dav1d_get_uleb128()
bool ReadLeb128(std::span<const uint8_t> data, size_t& pos, size_t& value)
{
  uint64_t result{0};
  for (int i = 0; i < 8; ++i)
  {
    if (pos >= data.size())
      return false;
    const uint8_t byte{data[pos++]};
    result |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
    if (!(byte & 0x80))
    {
      if (result > UINT32_MAX)
        return false;
      value = static_cast<size_t>(result);
      return true;
    }
  }
  return false;
}

/*!
 * \brief Reads the HDR10+ metadata of an AV1 temporal unit as dav1d and FFmpeg do: from an ITU-T
 * T.35 metadata OBU before the frame the unit shows, its last.
 * \return false when they can't be followed, or when dav1d's versions read them apart
 */
bool ReadAv1(std::span<const uint8_t> data, bool& hdr10Plus)
{
  hdr10Plus = false;
  bool frame{false};
  // Metadata dav1d holds for the next frame it shows
  bool pending{false};
  bool t35{false};
  size_t pos{0};
  while (pos < data.size())
  {
    const uint8_t header{data[pos++]};
    const uint8_t type{static_cast<uint8_t>((header >> 3) & 0x0F)};
    const bool extension{(header & 0x04) != 0};
    if (header & 0x80) // obu_forbidden_bit
      return false;
    if (extension && pos++ >= data.size())
      return false;
    size_t size{data.size() - pos};
    if ((header & 0x02) && !ReadLeb128(data, pos, size)) // obu_has_size_field
      return false;
    if (size > data.size() - pos)
      return false;
    const std::span<const uint8_t> obu{data.subspan(pos, size)};
    pos += size;

    if (type == OBU_FRAME_HEADER || type == OBU_FRAME)
    {
      frame = true;
      hdr10Plus = hdr10Plus || pending;
      pending = false;
      continue;
    }
    if (type != OBU_METADATA)
      continue;

    size_t offset{0};
    size_t metadataType{0};
    if (!ReadLeb128(obu, offset, metadataType))
      return false;
    if (metadataType != METADATA_TYPE_ITUT_T35)
      continue;
    // dav1d skips an OBU of a layer it doesn't decode. Kodi builds with dav1d from 1.0.0, which keeps
    // only the last T.35 message before a frame, where later versions keep them all.
    if (extension || t35)
      return false;
    t35 = true;

    // dav1d drops the trailing bits, and the country code which it hands over apart. Later
    // versions drop a message whose last nonzero byte isn't 0x80, which 1.0.0 takes.
    const std::span<const uint8_t> message{WithoutTrailingZeros(obu.subspan(offset))};
    const size_t start{!message.empty() && message[0] == 0xFF ? 2u : 1u};
    if (message.size() <= start + 1 || message.back() != 0x80)
      return false;
    switch (ReadT35(message[0], message.subspan(start, message.size() - 1 - start)))
    {
      case T35::HDR10PLUS:
        pending = true;
        break;
      case T35::INVALID:
        return false;
      default:
        break;
    }
  }
  // Metadata after the shown frame goes with the next unit's
  return frame && !pending;
}
} // namespace

CVideoHdrBitstream::CVideoHdrBitstream(AVCodecID codec,
                                       std::span<const uint8_t> extraData,
                                       bool readEnhancementLayer)
  : m_codec(codec),
    m_readEnhancementLayer(readEnhancementLayer)
{
  const bool readable{
      codec == AV_CODEC_ID_HEVC ||
      ((codec == AV_CODEC_ID_AV1 || codec == AV_CODEC_ID_VP9) && !readEnhancementLayer)};
  if (!readable || (codec == AV_CODEC_ID_HEVC && !ReadHevcExtraData(extraData)))
    m_state = State::UNDECIDED;
}

CVideoHdrBitstream::State CVideoHdrBitstream::Add(const DemuxPacket& packet)
{
  if (m_state != State::MORE)
    return m_state;
  ++m_packets;

  const std::span<const uint8_t> data{packet.pData,
                                      packet.pData ? static_cast<size_t>(packet.iSize) : 0};
  const auto* sideData{static_cast<const AVPacketSideData*>(packet.pSideData)};
  const int sideDataCount{packet.iSideDataElems};
  if (data.empty())
    return m_state = State::UNDECIDED;

  // HDR10+ metadata a demuxer gives with a packet goes to the packet's picture
  Picture picture;
  picture.hdr10Plus =
      av_packet_side_data_get(sideData, sideDataCount, AV_PKT_DATA_DYNAMIC_HDR10_PLUS) != nullptr;
  bool leading{false};

  if (m_codec == AV_CODEC_ID_HEVC)
  {
    const AVPacketSideData* extraData{
        av_packet_side_data_get(sideData, sideDataCount, AV_PKT_DATA_NEW_EXTRADATA)};
    if (extraData && extraData->size > 0 && !ReadHevcExtraData({extraData->data, extraData->size}))
      return m_state = State::UNDECIDED;

    const AVPacketSideData* configuration{
        av_packet_side_data_get(sideData, sideDataCount, AV_PKT_DATA_DOVI_CONF)};
    if (configuration && configuration->size >= sizeof(AVDOVIDecoderConfigurationRecord))
    {
      const auto* record{
          reinterpret_cast<const AVDOVIDecoderConfigurationRecord*>(configuration->data)};
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 30, 100)
      m_rpus.SetConfiguration(record->dv_profile, record->dv_md_compression != 0);
#else
      m_rpus.SetConfiguration(record->dv_profile, false);
#endif
    }

    if (!ReadHevc(data, leading))
      return m_state = State::UNDECIDED;
    picture.hdr10Plus = picture.hdr10Plus || m_hevcHdr10Plus;
    picture.layer = m_layer;
  }
  else if (m_codec == AV_CODEC_ID_AV1)
  {
    bool hdr10Plus{false};
    if (!ReadAv1(data, hdr10Plus))
      return m_state = State::UNDECIDED;
    picture.hdr10Plus = picture.hdr10Plus || hdr10Plus;
  }

  if (!m_first)
    m_first = picture;
  else if (picture != *m_first)
    return m_state = State::UNDECIDED;

  if (!leading)
    ++m_pictures;
  // Every HEVC picture after one with HDR10+ metadata has it too: FFmpeg's decoder keeps the last
  // metadata it read until it is flushed, and gives every picture a reference to it
  // (set_side_data() in libavcodec/hevc/hevcdec.c). The decoder starts at the first packet read,
  // so the picture it outputs first is never one before it.
  if (m_pictures >= PICTURES ||
      (m_codec == AV_CODEC_ID_HEVC && m_hevcHdr10Plus && !m_readEnhancementLayer))
    m_state = State::DONE;
  return m_state;
}

bool CVideoHdrBitstream::ReadHevcExtraData(std::span<const uint8_t> extraData)
{
  // ff_hevc_decode_extradata(): an hvcC record, or NAL units with start codes
  std::vector<Nal> nals;
  if (extraData.size() >= 23 &&
      (extraData[0] == 1 || (extraData[0] == 0 && (extraData[1] != 0 || extraData[2] > 1))))
  {
    m_nalLengthSize = (extraData[21] & 0x03) + 1;
    // Arrays of NAL units, each unit with a two byte size
    size_t pos{23};
    const auto read{[&](size_t bytes)
                    {
                      size_t value{0};
                      for (; bytes > 0; --bytes)
                        value = (value << 8) | (pos < extraData.size() ? extraData[pos++] : 0);
                      return value;
                    }};
    for (size_t arrays = extraData[22]; arrays > 0; --arrays)
    {
      read(1);
      for (size_t count = read(2); count > 0; --count)
      {
        if (extraData.size() - pos < 2)
          return false;
        const size_t size{(size_t{extraData[pos]} << 8 | extraData[pos + 1]) + 2};
        if (extraData.size() - pos < size)
          return false;
        std::vector<Nal> units;
        if (SplitHevc(extraData.subspan(pos, size), 2, units))
          nals.insert(nals.end(), units.begin(), units.end());
        pos += size;
      }
    }
  }
  else
  {
    m_nalLengthSize = 0;
    if (!SplitHevc(extraData, 0, nals))
      nals.clear();
  }

  return std::ranges::all_of(nals, [this](const Nal& nal)
                             { return nal.type != NAL_SEI_PREFIX || ReadHevcSei(nal.data); });
}

bool CVideoHdrBitstream::ReadHevc(std::span<const uint8_t> data, bool& leading)
{
  std::vector<Nal> nals;
  if (!SplitHevc(data, m_nalLengthSize, nals))
    return false;

  // A leading picture an HEVC decoder may skip after a seek
  const auto slice{
      std::ranges::find_if(nals, [](const Nal& nal) { return nal.type < NAL_VCL_END; })};
  leading = slice != nals.end() && (slice->type == NAL_RASL_N || slice->type == NAL_RASL_R);

  for (const Nal& nal : nals)
  {
    if (nal.type == NAL_SEI_PREFIX && !ReadHevcSei(nal.data))
      return false;
  }

  if (!m_readEnhancementLayer)
    return true;

  // The RPU is the last unspecified-62 unit of the base layer, but never the packet's first
  for (size_t i = nals.size(); i-- > 1;)
  {
    const Nal& nal{nals[i]};
    if (nal.type != NAL_UNSPEC62 || nal.data.size() <= 2 || nal.layer != 0 || nal.temporalId != 0)
      continue;

    const std::vector<uint8_t> rpu{HevcRpuToNal(nal.data.data() + 2, nal.data.size() - 2)};
    const std::optional<EnhancementLayer> layer{m_rpus.Read(std::span(rpu).subspan(2))};
    if (!layer)
      return false;
    m_layer = *layer;
    break;
  }
  return true;
}

bool CVideoHdrBitstream::ReadHevcSei(std::span<const uint8_t> nal)
{
  nal = WithoutTrailingZeros(nal);
  m_buffer.clear();
  const std::vector<CHevcSei> messages{
      CHevcSei::ParseSeiRbspUnclearedEmulation(nal.data(), nal.size(), m_buffer)};
  for (const CHevcSei& message : messages)
  {
    // User data registered by ITU-T T.35. CHevcSei keeps 8 bits of the payload type, so check its
    // one byte too.
    if (message.m_payloadType != 4 || m_buffer[message.m_msgOffset] != 4 ||
        message.m_payloadSize == 0)
      continue;

    const std::span<const uint8_t> payload{m_buffer.data() + message.m_payloadOffset,
                                           message.m_payloadSize};
    switch (ReadT35(payload[0], payload.subspan(1)))
    {
      case T35::HDR10PLUS:
        m_hevcHdr10Plus = true;
        break;
      case T35::INVALID:
        return false;
      default:
        break;
    }
  }
  return true;
}
