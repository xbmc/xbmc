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
