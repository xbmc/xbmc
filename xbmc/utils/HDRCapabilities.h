/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CHDRCapabilities
{
public:
  CHDRCapabilities() = default;
  ~CHDRCapabilities() = default;

  bool SupportsHDR10() const { return m_hdr10; }
  bool SupportsHLG() const { return m_hlg; }
  bool SupportsHDR10Plus() const { return m_hdr10_plus; }
  bool SupportsDolbyVision() const { return m_dolby_vision; }

  void SetHDR10() { m_hdr10 = true; }
  void SetHLG() { m_hlg = true; }
  void SetHDR10Plus() { m_hdr10_plus = true; }
  void SetDolbyVision() { m_dolby_vision = true; }

private:
  bool m_hdr10 = false;
  bool m_hlg = false;
  bool m_hdr10_plus = false;
  bool m_dolby_vision = false;
};
