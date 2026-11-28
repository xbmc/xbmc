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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CTexture;

/*!
 * \brief Plots the size of each video packet as a dot along the bottom of the
 * player debug overlay, one column per packet, plus a one second average.
 *
 * Column i always shows packet i modulo the width, also in scroll mode, which only
 * moves where the columns are drawn. So a new packet changes a few columns, and only
 * the strips of the texture that hold them are redrawn and uploaded again.
 */
class CDebugFrameGraph
{
public:
  CDebugFrameGraph();
  ~CDebugFrameGraph();

  //! True at most a few times a second, when new frames should be passed to SetFrames
  bool IsUpdateDue();
  void SetFrames(const DEBUG_INFO_FRAMES& frames);
  std::string GetLegend() const;
  void Render(const CRect& view);
  //! Free the frames, pixels and textures, e.g. when the overlay is closed
  void Reset();

private:
  //! Redraw columns x1 to x2, both included and x1 <= x2
  void Draw(int x1, int x2);
  void DrawChanged();
  unsigned int AverageWindow() const;
  size_t VisibleCount() const;
  double LinearMax() const;
  //! Upper end of the audio scale in Mb/s, 0 if there is no audio to show
  double AudioScaleMax() const;

  DEBUG_INFO_FRAMES m_frames;
  bool m_logScale{true};
  bool m_sweep{true};
  int m_heightPercent{35};
  int m_audioMode{0}; //!< 0 off, 1 overlay, 2 band
  bool m_redrawAll{true};

  std::chrono::steady_clock::time_point m_lastUpdate;
  uint64_t m_drawnCount{0}; //!< m_frames.count when the graph was last drawn
  double m_drawnLinearMax{0.0};
  double m_drawnAudioMax{0.0};

  unsigned int m_width{0};
  unsigned int m_height{0};
  std::vector<uint32_t> m_pixels;
  std::vector<std::unique_ptr<CTexture>> m_strips;
  std::vector<bool> m_stripChanged;
};
