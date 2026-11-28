/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CTexture;

/*!
 * \brief CPU usage per core for the player debug overlay, as a text line and
 * optionally as a graphic in the top right corner.
 *
 * The values come from CCPUInfo, so on Android they are Kodi's own usage and on
 * other platforms the whole system's.
 */
class CDebugCoreUsage
{
public:
  enum class Graph
  {
    OFF,
    BARS, //!< one bar per core with its current usage
    HISTORY, //!< usage of each core over the last minute
  };

  CDebugCoreUsage();
  ~CDebugCoreUsage();

  //! Take a sample if the last one is a second old. Call while the overlay is shown.
  void Update();
  //! e.g. "#0 78%  #1 64%  (total 71%)", empty if not available
  std::string GetSummary() const;
  void Render(const CRect& view, Graph graph);
  //! Free the samples, pixels and texture, e.g. when the overlay is closed
  void Reset();

private:
  void Draw(Graph graph, float scale);

  std::chrono::steady_clock::time_point m_lastSample;
  //! Per core, oldest first, at most one minute
  std::vector<std::vector<float>> m_history;
  int m_total{0};
  bool m_changed{false};

  Graph m_drawnGraph{Graph::OFF};
  unsigned int m_width{0};
  unsigned int m_height{0};
  std::vector<uint32_t> m_pixels;
  std::unique_ptr<CTexture> m_texture;
};
