/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DebugFrameGraph.h"

#include "ServiceBroker.h"
#include "guilib/GUITexture.h"
#include "guilib/Texture.h"
#include "guilib/TextureFormats.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr uint32_t COLOR_BACKDROP = 0x66000000;
constexpr uint32_t COLOR_GRID = 0x50FFFFFF;
constexpr uint32_t COLOR_FRAME = 0xFF3DDC84;
constexpr uint32_t COLOR_I_FRAME = 0xFFFFB547;
constexpr uint32_t COLOR_B_FRAME = 0xFF3F8CFF;
constexpr uint32_t COLOR_AVERAGE = 0xFFFFFFFF;
constexpr uint32_t COLOR_CURSOR = 0xB0FFFFFF;

constexpr double LOG_MIN = 4e3;
constexpr double LOG_MAX = 4e6;
constexpr unsigned int SWEEP_GAP = 16;
constexpr float GRAPH_HEIGHT = 0.35f;
constexpr float GRAPH_MARGIN = 0.03f;
} // namespace

CDebugFrameGraph::CDebugFrameGraph() = default;

CDebugFrameGraph::~CDebugFrameGraph() = default;

void CDebugFrameGraph::SetFrames(const DEBUG_INFO_FRAMES& frames)
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (settings->m_videoDebugGraphLogScale != m_logScale ||
      settings->m_videoDebugGraphSweep != m_sweep)
  {
    m_logScale = settings->m_videoDebugGraphLogScale;
    m_sweep = settings->m_videoDebugGraphSweep;
    m_dirty = true;
  }

  if (frames.serial == m_frames.serial)
    return;

  m_frames = frames;
  m_dirty = true;
}

unsigned int CDebugFrameGraph::AverageWindow() const
{
  return std::max(
      1u, static_cast<unsigned int>(std::lround(m_frames.fps > 0.0f ? m_frames.fps : 24.0f)));
}

size_t CDebugFrameGraph::VisibleCount() const
{
  if (m_width == 0)
    return m_frames.frames.size();

  // In sweep mode the oldest columns ahead of the cursor stay empty
  const size_t columns = m_sweep && m_width > SWEEP_GAP ? m_width - SWEEP_GAP : m_width;
  return std::min(m_frames.frames.size(), columns);
}

std::string CDebugFrameGraph::GetLegend() const
{
  const auto& frames = m_frames.frames;
  if (frames.empty())
    return {};

  const size_t shown = VisibleCount();
  double peak = 0.0;
  for (size_t i = frames.size() - shown; i < frames.size(); ++i)
    peak = std::max(peak, static_cast<double>(frames[i].size));

  const size_t window = std::min<size_t>(AverageWindow(), frames.size());
  double sum = 0.0;
  for (size_t i = frames.size() - window; i < frames.size(); ++i)
    sum += frames[i].size;
  const double fps = m_frames.fps > 0.0f ? static_cast<double>(m_frames.fps) : 24.0;
  const double rate = sum / static_cast<double>(window) * fps * 8 / 1e6;

  const std::string grid = m_logScale
                               ? "10 kB / 100 kB / 1 MB"
                               : StringUtils::Format("{:.0f} kB steps", peak * 1.05 / 4 / 1e3);

  const bool frameTypes = std::any_of(frames.begin(), frames.end(), [](const auto& frame)
                                      { return frame.type == 'P' || frame.type == 'B'; });

  return StringUtils::Format(
      "Frame sizes ({}, {}): peak {:.2f} MB, 1 s avg {:.1f} Mb/s, grid {}, {}",
      m_logScale ? "log" : "lin", m_sweep ? "sweep" : "scroll", peak / 1e6, rate, grid,
      frameTypes ? "amber I, green P, blue B" : "amber = keyframe");
}

void CDebugFrameGraph::Render(const CRect& view)
{
  if (m_frames.frames.empty() || view.IsEmpty())
    return;

  const unsigned int width =
      std::min(static_cast<unsigned int>(std::max(view.Width(), 1.0f)), DEBUG_INFO_FRAMES_MAX);
  const unsigned int height = std::max(static_cast<unsigned int>(view.Height() * GRAPH_HEIGHT), 1u);

  if (width != m_width || height != m_height || !m_texture)
  {
    m_width = width;
    m_height = height;
    m_pixels.assign(static_cast<size_t>(width) * height, 0);
    m_texture = CTexture::CreateTexture(width, height, XB_FMT_A8R8G8B8);
    m_dirty = true;
  }

  if (!m_texture)
    return;

  if (m_dirty)
  {
    Draw();
    m_texture->Update(width, height, width * 4, XB_FMT_A8R8G8B8,
                      reinterpret_cast<const unsigned char*>(m_pixels.data()), false);
    m_dirty = false;
  }

  const float bottom = view.y2 - view.Height() * GRAPH_MARGIN;
  const CRect rect(view.x1, bottom - static_cast<float>(height), view.x2, bottom);
  // The texture may be padded, e.g. to a power of two
  const CRect texCoords(0.0f, 0.0f,
                        static_cast<float>(m_texture->GetWidth()) /
                            static_cast<float>(m_texture->GetTextureWidth()),
                        static_cast<float>(m_texture->GetHeight()) /
                            static_cast<float>(m_texture->GetTextureHeight()));
  CGUITexture::DrawQuad(rect, 0xFFFFFFFF, m_texture.get(), &texCoords, -1.0f);
}

void CDebugFrameGraph::Draw()
{
  const int w = static_cast<int>(m_width);
  const int h = static_cast<int>(m_height);
  const auto& frames = m_frames.frames;

  std::fill(m_pixels.begin(), m_pixels.end(), COLOR_BACKDROP);

  const auto put = [&](int x, int y, uint32_t color)
  {
    if (x >= 0 && x < w && y >= 0 && y < h)
      m_pixels[static_cast<size_t>(y) * w + x] = color;
  };
  const auto vline = [&](int x, int y1, int y2, uint32_t color)
  {
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
      put(x, y, color);
  };

  const size_t count = VisibleCount();
  const size_t first = frames.size() - count;

  double linearMax = 1.0;
  for (size_t i = first; i < frames.size(); ++i)
    linearMax = std::max(linearMax, static_cast<double>(frames[i].size));
  linearMax *= 1.05;

  const auto toY = [&](double size)
  {
    double ratio = m_logScale ? (std::log(std::max(size, LOG_MIN)) - std::log(LOG_MIN)) /
                                    (std::log(LOG_MAX) - std::log(LOG_MIN))
                              : size / linearMax;
    ratio = std::clamp(ratio, 0.0, 1.0);
    return h - 1 - static_cast<int>(std::lround(ratio * (h - 1)));
  };
  const auto toX = [&](size_t i)
  {
    if (m_sweep)
      return static_cast<int>((m_frames.count - frames.size() + i) % m_width);
    return static_cast<int>(m_width - count + (i - first));
  };

  if (m_logScale)
  {
    for (const double size : {1e4, 1e5, 1e6})
      for (int x = 0; x < w; ++x)
        put(x, toY(size), COLOR_GRID);
  }
  else
  {
    for (int step = 1; step <= 4; ++step)
      for (int x = 0; x < w; ++x)
        put(x, toY(linearMax * step / 4), COLOR_GRID);
  }

  // Draw the frequent small frames first so I-frames stay visible on top
  for (const char type : {'B', 'P', 'I'})
  {
    const int radius = type == 'I' ? 2 : 1;
    const uint32_t color =
        type == 'I' ? COLOR_I_FRAME : (type == 'B' ? COLOR_B_FRAME : COLOR_FRAME);
    for (size_t i = first; i < frames.size(); ++i)
    {
      // Frames of unknown type are drawn like P-frames
      const char frameType = frames[i].type ? frames[i].type : 'P';
      if (frameType != type)
        continue;
      const int x = toX(i);
      const int y = toY(frames[i].size);
      for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx)
          put(x + dx, y + dy, color);
    }
  }

  const size_t window = AverageWindow();
  double sum = 0.0;
  int prevX = -1;
  int prevY = 0;
  for (size_t i = 0; i < frames.size(); ++i)
  {
    sum += frames[i].size;
    if (i >= window)
      sum -= frames[i - window].size;
    if (i < first || i + 1 < window)
      continue;

    const int x = toX(i);
    const int y = toY(sum / static_cast<double>(window));
    const int fromY = prevX >= 0 && x == prevX + 1 ? prevY : y;
    vline(x, fromY, y, COLOR_AVERAGE);
    vline(x, fromY + 1, y + 1, COLOR_AVERAGE);
    prevX = x;
    prevY = y;
  }

  if (m_sweep && count > 0)
    vline((toX(frames.size() - 1) + 1) % w, 0, h - 1, COLOR_CURSOR);
}
