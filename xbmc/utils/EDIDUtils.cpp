/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EDIDUtils.h"

#include "utils/log.h"

#include <bitset>
#include <cmath>
#include <map>
#include <tuple>

using namespace KODI::UTILS;

namespace
{
constexpr int EDID_PAGE_SIZE = 128;
constexpr int EDID_CEA_EXT_ID = 0x02;
constexpr int EDID_CEA_TAG_EXTENDED = 0x07;

/* CEA-861-G new EDID blocks for HDR */
constexpr int EDID_CEA_TAG_COLORIMETRY = 0x05;
constexpr int EDID_CEA_EXT_TAG_STATIC_METADATA = 0x06;

constexpr std::array<const char*, 4> eotfStrings = {
    "Traditional gamma - SDR luminance range",
    "Traditional gamma - HDR luminance range",
    "SMPTE ST2084",
    "Hybrid Log-Gamma",
};

// names must match that defined in linux/drivers/gpu/drm/drm_connector.c
// order must be exactly this in order to check supported bits
//! @todo: find a better way to translate colorimetry values
// clang-format off
constexpr std::array<const char*, 8> colorimetryStrings = {
  "XVYCC_601",
  "XVYCC_709",
  "SYCC_601",
  "opYCC_601",
  "opRGB",
  "BT2020_CYCC",
  "BT2020_YCC",
  "BT2020_RGB"
};
// clang-format on

constexpr std::array<uint8_t, 8> edidHeader = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

} // namespace

void CEDIDUtils::SetEDID(std::vector<uint8_t> edid)
{
  if ((edid.size() % EDID_PAGE_SIZE) != 0)
    return;

  if (!std::equal(edid.begin(), edid.begin() + 8, edidHeader.data()))
    return;

  m_edid = edid;
}

const uint8_t* CEDIDUtils::FindCEAExtentionBlock()
{
  int blocks = m_edid.size() / EDID_PAGE_SIZE;
  for (int block = 0; block < blocks; block++)
  {
    const uint8_t* extension = &m_edid[EDID_PAGE_SIZE * (block + 1)];

    if (extension[0] == EDID_CEA_EXT_ID)
      return extension;
  }

  return nullptr;
}

std::vector<uint8_t> CEDIDUtils::FindExtendedDataBlock(uint32_t blockTag)
{
  auto block = FindCEAExtentionBlock();

  if (!block)
    return std::vector<uint8_t>();

  const uint8_t* start = block + 4;
  const uint8_t* end = block + block[2] - 1;

  uint8_t length{0};
  for (const uint8_t* db = start; db < end; db += (length + 1))
  {
    length = db[0] & 0x1F;
    if ((db[0] >> 5) != EDID_CEA_TAG_EXTENDED)
      continue;

    if (db[1] == blockTag)
      return std::vector<uint8_t>(db + 2, db + 2 + length - 1);
  }

  return std::vector<uint8_t>();
}

void CEDIDUtils::LogSupportedColorimetry()
{
  if (m_edid.empty())
    return;

  auto block = FindExtendedDataBlock(EDID_CEA_TAG_COLORIMETRY);

  if (block.size() < 2)
    return;

  std::bitset<colorimetryStrings.size()> supportedColorimetryTypes{block[0]};

  std::string colorStr;
  for (size_t i = 0; i < colorimetryStrings.size(); i++)
  {
    if (supportedColorimetryTypes[i])
      colorStr.append(StringUtils::Format("\n%s", colorimetryStrings[i]));
  }

  if (block[1] & 0x80)
    colorStr.append("\nDCI-P3");

  if (block[1] & 0x40)
    colorStr.append("\nICtCp");

  CLog::Log(LOGDEBUG, "CEDIDUtils:{} - supported connector colorimetry:{}", __FUNCTION__, colorStr);
}

bool CEDIDUtils::SupportsColorimetry(const std::string& colorimetry)
{
  if (m_edid.empty())
    return false;

  auto block = FindExtendedDataBlock(EDID_CEA_TAG_COLORIMETRY);

  if (block.size() < 2)
    return false;

  std::bitset<colorimetryStrings.size()> supportedColorimetryTypes{block[0]};

  for (size_t i = 0; i < colorimetryStrings.size(); i++)
  {
    if (colorimetryStrings[i] == colorimetry)
    {
      if (supportedColorimetryTypes[i])
        return true;

      CLog::Log(LOGDEBUG, "CEDIDUtils::{} - edid does not support requested colorimetry: {}",
                __FUNCTION__, colorimetryStrings[i]);
    }
  }

  return false;
}

void CEDIDUtils::LogSupportedEOTF()
{
  if (m_edid.empty())
    return;

  auto block = FindExtendedDataBlock(EDID_CEA_EXT_TAG_STATIC_METADATA);

  if (block.size() < 2)
    return;

  constexpr size_t maxStaticMetadataTypes{8};
  std::bitset<maxStaticMetadataTypes> supportedMetadataTypes{block[1]};
  for (size_t i = 0; i < maxStaticMetadataTypes; i++)
  {
    if (supportedMetadataTypes[i])
      CLog::Log(LOGDEBUG, "CEDIDUtils:{} - supported static metadata type {}", __FUNCTION__, i + 1);
  }

  std::string eotfStr;
  constexpr size_t maxEotfs{6};
  std::bitset<maxEotfs> supportedEotfs{block[0]};
  for (size_t i = 0; i < maxEotfs; i++)
  {
    if (supportedEotfs[i])
    {
      if (i < eotfStrings.size())
        eotfStr.append(StringUtils::Format("\n%s", eotfStrings[i]));
      else
        eotfStr.append("\n unknown eotf");
    }
  }

  CLog::Log(LOGDEBUG, "CEDIDUtils:{} - supported connector eotf:{}", __FUNCTION__, eotfStr);
}

bool CEDIDUtils::SupportsEOTF(uint8_t eotf)
{
  if (m_edid.empty())
    return false;

  auto block = FindExtendedDataBlock(EDID_CEA_EXT_TAG_STATIC_METADATA);

  if (block.size() < 2)
    return false;

  constexpr size_t maxEotfs{4};
  std::bitset<maxEotfs> supportedEotfs{block[0]};
  if (supportedEotfs[eotf])
    return true;

  CLog::Log(LOGDEBUG, "CEDIDUtils:{} - edid does not support requested eotf: {}", __FUNCTION__,
            eotfStrings[eotf]);

  return false;
}

void CEDIDUtils::LogSupportedLuminance()
{
  if (m_edid.empty())
    return;

  auto block = FindExtendedDataBlock(EDID_CEA_EXT_TAG_STATIC_METADATA);

  if (block.size() >= 3)
    CLog::Log(LOGDEBUG, "CEDIDUtils:{} - max luminance: {} ({} cd/m^2)", __FUNCTION__,
              static_cast<int>(block[2]), static_cast<int>(50.0 * pow(2, block[2] / 32.0)));

  if (block.size() >= 4)
    CLog::Log(LOGDEBUG, "CEDIDUtils:{} - maxFALL: {} ({} cd/m^2)", __FUNCTION__,
              static_cast<int>(block[3]), static_cast<int>(50.0 * pow(2, block[3] / 32.0)));

  if (block.size() >= 5)
    CLog::Log(
        LOGDEBUG, "CEDIDUtils:{} - min luminance: {} ({} cd/m^2)", __FUNCTION__,
        static_cast<int>(block[5]),
        static_cast<int>((50.0 * pow(2, block[2] / 32.0)) * pow(block[4] / 255.0, 2) / 100.0));
}

void CEDIDUtils::ClampLuminance(std::tuple<int, int, int>& luminance)
{
  if (!m_edid.empty())
    return;

  auto block = FindExtendedDataBlock(EDID_CEA_EXT_TAG_STATIC_METADATA);

  int max{0};
  if (block.size() >= 3)
    max = std::min(static_cast<int>(50.0 * pow(2, block[2] / 32.0)), std::get<0>(luminance));

  int avg{0};
  if (block.size() >= 4)
    avg = std::min(static_cast<int>(50.0 * pow(2, block[3] / 32.0)), std::get<1>(luminance));

  int min{0};
  if (block.size() >= 5)
    min =
        std::max(static_cast<int>(max * pow(block[4] / 255.0, 2) / 100.0), std::get<2>(luminance));

  luminance = {max, avg, min};
}

void CEDIDUtils::LogInfo()
{
  if (m_edid.empty())
    return;

  std::stringstream make;
  make << static_cast<char>(((m_edid[0x08 + 0] & 0x7C) >> 2) + '@');
  make << static_cast<char>(((m_edid[0x08 + 0] & 0x03) << 3) + ((m_edid[0x08 + 1] & 0xE0) >> 5) +
                            '@');
  make << static_cast<char>((m_edid[0x08 + 1] & 0x1F) + '@');

  int model = static_cast<int>(m_edid[0x0A] + (m_edid[0x0B] << 8));

  CLog::Log(LOGINFO, "CEDIDUtils:{} - manufacturer '{}' model '{:x}'", __FUNCTION__, make.str(),
            model);

  LogSupportedColorimetry();
  LogSupportedEOTF();
  LogSupportedLuminance();
}
