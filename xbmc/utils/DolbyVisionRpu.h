/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

class CBitReader;

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

//! The enhancement layer of a picture, as Kodi's FFmpeg decoder tells it from the picture's RPU.
enum class EnhancementLayer
{
  NONE,
  MEL, //!< a minimal enhancement layer: its residual is quantised as the identity
  FEL, //!< a full enhancement layer
};

/*!
 * \brief Reads a stream's RPUs in turn as FFmpeg's decoder does (libavcodec/dovi_rpudec.c), to tell
 * the enhancement layer of the picture each comes with.
 *
 * A picture whose RPU resamples a residual (el_spatial_resampling_filter_flag 1,
 * disable_residual_flag 0) has a MEL, or a FEL when the nonlinear quantisation of any component
 * differs from the identity. The data mappings a later RPU may refer back to are kept.
 */
class CRpuReader
{
public:
  /*!
   * \brief Sets the configuration record the decoder has for the stream.
   * \param profile 0 while it has none, which makes it guess the profile from each RPU
   */
  void SetConfiguration(uint8_t profile, bool metadataCompression);

  /*!
   * \param rpu An RPU without emulation prevention bytes: the payload of an unspecified-62 NAL
   * unit after its two byte header, from the 0x19 prefix on.
   * \return The enhancement layer of the RPU's picture, or nothing when FFmpeg would reject or
   * ignore the RPU, reads it in a way not followed here, or its CRC doesn't match. The reader then
   * no longer follows the decoder.
   */
  std::optional<EnhancementLayer> Read(std::span<const uint8_t> rpu);

private:
  struct Quantisation
  {
    uint16_t offset{0};
    uint64_t vdrInMax{0};
    uint64_t deadzoneSlope{0};
    uint64_t deadzoneThreshold{0};
  };
  using Mapping = std::array<Quantisation, 3>;

  bool ReadExtensionBlocks(CBitReader& br, int version, int& dynamicBlocks);

  std::array<std::optional<Mapping>, 16> m_mappings;
  int m_staticBlocks{0};
  uint8_t m_profile{0};
  bool m_metadataCompression{false};
};
} // namespace KODI::UTILS::DOLBYVISION
