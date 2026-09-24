/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/*!
 * Both functions return the RPU as an HEVC unspecified-62 NAL unit: NAL header, RPU prefix and
 * the RPU with emulation prevention bytes removed.
 */
namespace KODI::UTILS::DOLBYVISION
{
/*!
 * \brief Converts the RPU exported by FFmpeg (AV_FRAME_DATA_DOVI_RPU_BUFFER), which lacks the NAL
 * header and still contains emulation prevention bytes.
 */
std::vector<uint8_t> HevcRpuToNal(const uint8_t* data, size_t size);

/*!
 * \brief Extracts the RPU from the ITU-T T.35 metadata OBU of an AV1 temporal unit.
 * \return empty if there is none, or without libdovi
 */
std::vector<uint8_t> ExtractAv1Rpu(const uint8_t* data, size_t size);
} // namespace KODI::UTILS::DOLBYVISION
