/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HevcSei.h"

void HevcAddStartCodeEmulationPrevention3Byte(std::vector<uint8_t>& buf)
{
  size_t i = 0;

  while (i < buf.size())
  {
    if (i > 2 && buf[i - 2] == 0 && buf[i - 1] == 0 && buf[i] <= 3)
      buf.insert(buf.begin() + i, 3);

    i += 1;
  }
}

void HevcClearStartCodeEmulationPrevention3Byte(const uint8_t* buf,
                                                const size_t len,
                                                std::vector<uint8_t>& out)
{
  size_t i = 0;

  if (len > 2)
  {
    out.reserve(len);

    out.emplace_back(buf[0]);
    out.emplace_back(buf[1]);

    for (i = 2; i < len; i++)
    {
      if (!(buf[i - 2] == 0 && buf[i - 1] == 0 && buf[i] == 3))
        out.emplace_back(buf[i]);
    }
  }
  else
  {
    out.assign(buf, buf + len);
  }
}

int CHevcSei::ParseSeiMessage(CBitstreamReader& br, std::vector<CHevcSei>& messages)
{
  CHevcSei sei;
  uint8_t lastPayloadTypeByte{0};
  uint8_t lastPayloadSizeByte{0};

  sei.m_msgOffset = br.Position() / 8;

  lastPayloadTypeByte = br.ReadBits(8);
  while (lastPayloadTypeByte == 0xFF)
  {
    lastPayloadTypeByte = br.ReadBits(8);
    sei.m_payloadType += 255;
  }

  sei.m_payloadType += lastPayloadTypeByte;

  lastPayloadSizeByte = br.ReadBits(8);
  while (lastPayloadSizeByte == 0xFF)
  {
    lastPayloadSizeByte = br.ReadBits(8);
    sei.m_payloadSize += 255;
  }

  sei.m_payloadSize += lastPayloadSizeByte;
  sei.m_payloadOffset = br.Position() / 8;

  // Invalid size
  if (sei.m_payloadSize > br.AvailableBits())
    return 1;

  br.SkipBits(sei.m_payloadSize * 8);
  messages.emplace_back(sei);

  return 0;
}

std::vector<CHevcSei> CHevcSei::ParseSeiRbspInternal(const uint8_t* buf, const size_t len)
{
  std::vector<CHevcSei> messages;

  if (len > 4)
  {
    CBitstreamReader br(buf, len);

    // forbidden_zero_bit, nal_type, nuh_layer_id, temporal_id
    // nal_type == SEI_PREFIX should already be verified by caller
    br.SkipBits(16);

    while (true)
    {
      if (ParseSeiMessage(br, messages))
        break;

      if (br.AvailableBits() <= 8)
        break;
    }
  }

  return messages;
}

std::vector<CHevcSei> CHevcSei::ParseSeiRbsp(const uint8_t* buf, const size_t len)
{
  return ParseSeiRbspInternal(buf, len);
}

std::vector<CHevcSei> CHevcSei::ParseSeiRbspUnclearedEmulation(const uint8_t* inData,
                                                               const size_t inDataLen,
                                                               std::vector<uint8_t>& buf)
{
  HevcClearStartCodeEmulationPrevention3Byte(inData, inDataLen, buf);
  return ParseSeiRbsp(buf.data(), buf.size());
}

std::optional<const CHevcSei*> CHevcSei::FindHdr10PlusSeiMessage(
    const std::vector<uint8_t>& buf, const std::vector<CHevcSei>& messages)
{
  for (const CHevcSei& sei : messages)
  {
    // User Data Registered ITU-T T.35
    if (sei.m_payloadType == 4 && sei.m_payloadSize >= 7)
    {
      CBitstreamReader br(buf.data() + sei.m_payloadOffset, sei.m_payloadSize);
      const auto itu_t_t35_country_code = br.ReadBits(8);
      const auto itu_t_t35_terminal_provider_code = br.ReadBits(16);
      const auto itu_t_t35_terminal_provider_oriented_code = br.ReadBits(16);

      // United States, Samsung Electronics America, ST 2094-40
      if (itu_t_t35_country_code == 0xB5 && itu_t_t35_terminal_provider_code == 0x003C &&
          itu_t_t35_terminal_provider_oriented_code == 0x0001)
      {
        const auto application_identifier = br.ReadBits(8);
        const auto application_version = br.ReadBits(8);

        if (application_identifier == 4 && application_version <= 1)
          return &sei;
      }
    }
  }

  return {};
}

std::pair<bool, const std::vector<uint8_t>> CHevcSei::RemoveHdr10PlusFromSeiNalu(
    const uint8_t* inData, const size_t inDataLen)
{
  bool containsHdr10Plus{false};

  std::vector<uint8_t> buf;
  std::vector<CHevcSei> messages = CHevcSei::ParseSeiRbspUnclearedEmulation(inData, inDataLen, buf);

  if (auto res = CHevcSei::FindHdr10PlusSeiMessage(buf, messages))
  {
    auto msg = *res;

    containsHdr10Plus = true;
    if (messages.size() > 1)
    {
      // Multiple SEI messages in NALU, remove only the HDR10+ one
      buf.erase(std::next(buf.begin(), msg->m_msgOffset),
                std::next(buf.begin(), msg->m_payloadOffset + msg->m_payloadSize));
      HevcAddStartCodeEmulationPrevention3Byte(buf);
    }
    else
    {
      // Single SEI message in NALU
      buf.clear();
    }
  }
  else
  {
    // No HDR10+
    buf.clear();
  }

  return std::make_pair(containsHdr10Plus, buf);
}
