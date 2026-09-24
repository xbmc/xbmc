/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DolbyVisionRpu.h"

#include <cstring>

#ifdef HAVE_LIBDOVI
#include <libdovi/rpu_parser.h>
#endif

namespace
{
constexpr uint8_t NAL_HEADER[] = {0x7C, 0x01};

#ifdef HAVE_LIBDOVI
constexpr uint8_t RPU_PREFIX = 0x19;

bool ReadLeb128(const uint8_t* data, size_t size, size_t& pos, uint64_t& value)
{
  value = 0;
  for (int i = 0; i < 8 && pos < size; ++i)
  {
    const uint8_t byte = data[pos++];
    value |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
    if (!(byte & 0x80))
      return true;
  }
  return false;
}

// The T.35 payload carries the RPU in an EMDF container, which libdovi unwraps.
std::vector<uint8_t> ParseAv1T35(const uint8_t* data, size_t size)
{
  DoviRpuOpaque* parsed = dovi_parse_itu_t35_dovi_metadata_obu(data, size);
  const DoviData* written = dovi_rpu_get_error(parsed) ? nullptr : dovi_write_rpu(parsed);
  std::vector<uint8_t> nal;
  if (written && written->len)
  {
    nal.assign(std::begin(NAL_HEADER), std::end(NAL_HEADER));
    if (written->data[0] != RPU_PREFIX)
      nal.push_back(RPU_PREFIX);
    nal.insert(nal.end(), written->data, written->data + written->len);
  }
  if (written)
    dovi_data_free(written);
  dovi_rpu_free(parsed);
  return nal;
}
#endif
} // namespace

namespace KODI::UTILS::DOLBYVISION
{
std::vector<uint8_t> HevcRpuToNal(const uint8_t* data, size_t size)
{
  std::vector<uint8_t> nal(std::begin(NAL_HEADER), std::end(NAL_HEADER));
  nal.reserve(size + sizeof(NAL_HEADER));
  int zeros = 0;
  for (size_t i = 0; i < size; ++i)
  {
    if (zeros >= 2 && data[i] == 0x03)
    {
      zeros = 0;
      continue;
    }
    zeros = data[i] == 0 ? zeros + 1 : 0;
    nal.push_back(data[i]);
  }
  return nal;
}

std::vector<uint8_t> ExtractAv1Rpu([[maybe_unused]] const uint8_t* data,
                                   [[maybe_unused]] size_t size)
{
#ifdef HAVE_LIBDOVI
  constexpr int OBU_METADATA = 5;
  constexpr uint64_t METADATA_TYPE_ITUT_T35 = 4;
  // country code US, provider code Dolby, provider oriented code
  constexpr uint8_t dolbyT35[] = {0xB5, 0x00, 0x3B, 0x00, 0x00, 0x08, 0x00};

  size_t pos = 0;
  while (pos < size)
  {
    const uint8_t header = data[pos++];
    if (header & 0x04) // obu_extension_flag
      ++pos;
    uint64_t obuSize = pos < size ? size - pos : 0;
    if ((header & 0x02) && !ReadLeb128(data, size, pos, obuSize)) // obu_has_size_field
      return {};
    if (pos > size || obuSize > size - pos)
      return {};

    const uint8_t* obu = data + pos;
    pos += obuSize;
    if (((header >> 3) & 0x0F) != OBU_METADATA)
      continue;

    size_t offset = 0;
    uint64_t metadataType = 0;
    if (!ReadLeb128(obu, obuSize, offset, metadataType) || metadataType != METADATA_TYPE_ITUT_T35 ||
        obuSize - offset <= sizeof(dolbyT35) ||
        std::memcmp(obu + offset, dolbyT35, sizeof(dolbyT35)) != 0)
      continue;

    std::vector<uint8_t> nal = ParseAv1T35(obu + offset, obuSize - offset);
    if (!nal.empty())
      return nal;
  }
#endif
  return {};
}
} // namespace KODI::UTILS::DOLBYVISION
