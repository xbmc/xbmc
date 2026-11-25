/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

struct di_info;
struct di_cta_colorimetry_block;
struct di_cta_hdr_static_metadata_block;

namespace KODI
{
namespace UTILS
{

enum class Eotf
{
  TRADITIONAL_SDR,
  TRADITIONAL_HDR,
  PQ,
  HLG,
};

enum class Colorimetry
{
  // DRM order (DRM_MODE_COLORIMETRY_*)
  DEFAULT,
  SMPTE_170M_YCC,
  BT709_YCC,
  XVYCC_601,
  XVYCC_709,
  SYCC_601,
  OPYCC_601,
  OPRGB,
  BT2020_CYCC,
  BT2020_RGB,
  BT2020_YCC,
  DCI_P3_RGB_D65,
  DCI_P3_RGB_THEATER,
  RGB_WIDE_FIXED,
  RGB_WIDE_FLOAT,
  BT601_YCC,
  // CTA-861 only (not in DRM)
  ST2113_RGB,
  ICTCP,
};

class CDisplayInfo
{
public:
  static std::unique_ptr<CDisplayInfo> Create(const std::vector<uint8_t>& edid);

  ~CDisplayInfo();

  bool SupportsHDRStaticMetadataType1() const;

  bool SupportsEOTF(Eotf eotf) const;

  bool SupportsColorimetry(Colorimetry colorimetry) const;

  //! The brightest light, in nits, the display says content should have, or 0
  //! when its HDR static metadata leaves it unset
  float GetMaxLuminance() const;

  //! The lowest and highest refresh rates, in Hz, of the display's HDMI 2.1
  //! variable refresh rate. The lowest is 0 if it has none, the highest 0 if
  //! it has no limit.
  std::pair<int, int> GetVariableRefreshRange() const { return {m_vrrMinHz, m_vrrMaxHz}; }

private:
  explicit CDisplayInfo(const std::vector<uint8_t>& edid);

  bool IsValid() const;
  void Parse();
  void LogInfo() const;

  struct DiInfoDeleter
  {
    void operator()(di_info* p);
  };

  std::unique_ptr<di_info, DiInfoDeleter> m_info;

  std::string m_make;
  std::string m_model;

  const di_cta_colorimetry_block* m_colorimetry = nullptr;
  const di_cta_hdr_static_metadata_block* m_hdr_static_metadata = nullptr;
  int m_vrrMinHz = 0;
  int m_vrrMaxHz = 0;
};

} // namespace UTILS
} // namespace KODI
