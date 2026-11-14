/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DebugInfo.h"
#include "utils/Geometry.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CTexture;

/*!
 * \brief Plots the size of each video packet as a dot along the bottom of the
 * player debug overlay, one column per packet, plus a one second average.
 */
class CDebugFrameGraph
{
public:
  CDebugFrameGraph();
  ~CDebugFrameGraph();

  void SetFrames(const DEBUG_INFO_FRAMES& frames);
  std::string GetLegend() const;
  void Render(const CRect& view);

private:
  void Draw();
  unsigned int AverageWindow() const;
  size_t VisibleCount() const;

  DEBUG_INFO_FRAMES m_frames;
  bool m_logScale{true};
  bool m_sweep{true};
  int m_heightPercent{35};
  bool m_dirty{false};

  unsigned int m_width{0};
  unsigned int m_height{0};
  std::vector<uint32_t> m_pixels;
  std::unique_ptr<CTexture> m_texture;
};
