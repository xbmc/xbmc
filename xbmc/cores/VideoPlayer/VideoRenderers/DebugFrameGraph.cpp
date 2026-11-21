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
#include <array>
#include <cmath>
#include <string_view>

namespace
{
constexpr uint32_t COLOR_BACKDROP = 0x66000000;
constexpr uint32_t COLOR_GRID = 0x50FFFFFF;
constexpr uint32_t COLOR_GRID_LABEL = 0xC8FFFFFF;
constexpr uint32_t COLOR_BACKDROP_LABEL = 0xB0000000;
constexpr uint32_t COLOR_FRAME = 0xFF3DDC84;
constexpr uint32_t COLOR_I_FRAME = 0xFFFFB547;
constexpr uint32_t COLOR_B_FRAME = 0xFF3F8CFF;
constexpr uint32_t COLOR_AVERAGE = 0xFFFFFFFF;
constexpr uint32_t COLOR_CURSOR = 0xB0FFFFFF;
constexpr uint32_t COLOR_AUDIO = 0xFFFF6AD5;

constexpr double LOG_MIN = 4e3;
constexpr double LOG_MAX = 4e6;
constexpr unsigned int SWEEP_GAP = 16;
constexpr unsigned int STRIP_WIDTH = 256;
constexpr auto UPDATE_INTERVAL = std::chrono::milliseconds(250);
constexpr double AUDIO_BAND_SPLIT = 0.78; //!< share of the height for the video in band mode

//! 5x7 pixel glyphs for the grid labels, one row per entry, bit 4 is the left column
constexpr std::string_view GLYPH_CHARS = "0123456789.kBM";
constexpr int GLYPH_WIDTH = 5;
constexpr int GLYPH_HEIGHT = 7;
constexpr std::array<std::array<uint8_t, GLYPH_HEIGHT>, 14> GLYPHS = {{
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C},
    {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
}};
//! The longest grid label, e.g. "1000 kB"
constexpr int LABEL_CHARS = 8;

//! Size of one glyph pixel, 2 on a 1080p screen
int FontSize(unsigned int width)
{
  return std::max(1, static_cast<int>(width / 960));
}

//! Width of the grid labels and their boxes, including the space before them
int LabelColumns(unsigned int width)
{
  return (LABEL_CHARS * (GLYPH_WIDTH + 1) + 6) * FontSize(width);
}

std::string FormatSize(double bytes)
{
  if (bytes >= 1e6)
    return StringUtils::Format("{:g} MB", std::round(bytes / 1e5) / 10);
  return StringUtils::Format("{:.0f} kB", bytes / 1e3);
}

//! Round up to 1, 2 or 5 times a power of ten
double NiceCeil(double value)
{
  if (value <= 0.0)
    return 1.0;
  const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
  for (const double step : {1.0, 2.0, 5.0, 10.0})
  {
    if (step * magnitude >= value)
      return step * magnitude;
  }
  return 10.0 * magnitude;
}
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
    m_redrawAll = true;
  }
  m_heightPercent = settings->m_videoDebugGraphHeight;
  if (settings->m_videoDebugGraphAudio != m_audioMode)
  {
    m_audioMode = settings->m_videoDebugGraphAudio;
    m_redrawAll = true;
  }

  if (frames.serial == m_frames.serial)
    return;

  // A new stream starts counting again
  if (frames.count < m_frames.count)
    m_redrawAll = true;
  m_frames = frames;
}

bool CDebugFrameGraph::IsUpdateDue()
{
  const auto now = std::chrono::steady_clock::now();
  if (now - m_lastUpdate < UPDATE_INTERVAL)
    return false;

  m_lastUpdate = now;
  return true;
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

double CDebugFrameGraph::AudioScaleMax() const
{
  const auto& rates = m_frames.audioRates;
  if (m_audioMode == 0 || rates.size() != m_frames.frames.size())
    return 0.0;

  double maxRate = -1.0;
  for (size_t i = rates.size() - VisibleCount(); i < rates.size(); ++i)
  {
    if (!std::isnan(rates[i]))
      maxRate = std::max(maxRate, static_cast<double>(rates[i]));
  }
  return maxRate < 0.0 ? 0.0 : NiceCeil(maxRate * 1.1);
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

  const bool frameTypes = std::any_of(frames.begin(), frames.end(), [](const auto& frame)
                                      { return frame.type == 'P' || frame.type == 'B'; });

  std::string audio;
  if (const double scaleMax = AudioScaleMax(); scaleMax > 0.0)
  {
    const auto& rates = m_frames.audioRates;
    const auto latest =
        std::find_if(rates.rbegin(), rates.rend(), [](float rate) { return !std::isnan(rate); });
    audio = StringUtils::Format(", {{\\c&HD56AFF&}}audio {:.1f} Mb/s (0-{:g} Mb/s){{\\c}}",
                                static_cast<double>(*latest), scaleMax);
  }

  // The frame types and the audio are written in the colours of their dots and line
  return StringUtils::Format(
      "Frame sizes ({}, {}): peak {:.2f} MB, 1 s avg {:.1f} Mb/s, {}{}", m_logScale ? "log" : "lin",
      m_sweep ? "sweep" : "scroll", peak / 1e6, rate,
      frameTypes ? "{\\c&H47B5FF&}I {\\c&H84DC3D&}P {\\c&HFF8C3F&}B{\\c} frames"
                 : "{\\c&H47B5FF&}keyframes{\\c}",
      audio);
}

void CDebugFrameGraph::Render(const CRect& view)
{
  if (m_frames.frames.empty() || view.IsEmpty())
    return;

  const unsigned int width =
      std::min(static_cast<unsigned int>(std::max(view.Width(), 1.0f)), DEBUG_INFO_FRAMES_MAX);
  const unsigned int height = std::max(
      static_cast<unsigned int>(view.Height() * static_cast<float>(m_heightPercent) / 100), 1u);

  if (width != m_width || height != m_height || m_strips.empty())
  {
    m_width = width;
    m_height = height;
    // CTexture::Update() reads a padded texture's full width, also from the last row
    m_pixels.assign(static_cast<size_t>(width) * height + STRIP_WIDTH, 0);
    m_strips.clear();
    for (unsigned int x = 0; x < width; x += STRIP_WIDTH)
    {
      m_strips.push_back(
          CTexture::CreateTexture(std::min(STRIP_WIDTH, width - x), height, XB_FMT_A8R8G8B8));
      if (!m_strips.back())
      {
        m_strips.clear();
        return;
      }
    }
    m_stripChanged.assign(m_strips.size(), false);
    m_redrawAll = true;
  }

  DrawChanged();

  for (size_t i = 0; i < m_strips.size(); ++i)
  {
    if (!m_stripChanged[i])
      continue;
    const unsigned int x = static_cast<unsigned int>(i) * STRIP_WIDTH;
    m_strips[i]->Update(m_strips[i]->GetWidth(), height, width * 4, XB_FMT_A8R8G8B8,
                        reinterpret_cast<const unsigned char*>(m_pixels.data() + x), false);
    m_stripChanged[i] = false;
  }

  // In scroll mode the newest column is drawn at the right edge
  const unsigned int shift =
      m_sweep ? 0 : static_cast<unsigned int>((m_width - m_frames.count % m_width) % m_width);
  const float bottom = view.y2;
  const float top = bottom - static_cast<float>(height);

  for (size_t i = 0; i < m_strips.size(); ++i)
  {
    const CTexture& strip = *m_strips[i];
    const unsigned int stripX = static_cast<unsigned int>(i) * STRIP_WIDTH;
    const unsigned int stripWidth = strip.GetWidth();
    // The texture may be padded, e.g. to a power of two
    const float u = static_cast<float>(stripWidth) / static_cast<float>(strip.GetTextureWidth());
    const float v = static_cast<float>(height) / static_cast<float>(strip.GetTextureHeight());

    // A strip that crosses the right edge after shifting continues at the left edge
    const unsigned int x = (stripX + shift) % m_width;
    const unsigned int part = std::min(stripWidth, m_width - x);
    const CRect rect(view.x1 + static_cast<float>(x), top, view.x1 + static_cast<float>(x + part),
                     bottom);
    const CRect texCoords(0.0f, 0.0f, u * static_cast<float>(part) / static_cast<float>(stripWidth),
                          v);
    CGUITexture::DrawQuad(rect, 0xFFFFFFFF, m_strips[i].get(), &texCoords, -1.0f);
    if (part < stripWidth)
    {
      const CRect restRect(view.x1, top, view.x1 + static_cast<float>(stripWidth - part), bottom);
      const CRect restCoords(texCoords.x2, 0.0f, u, v);
      CGUITexture::DrawQuad(restRect, 0xFFFFFFFF, m_strips[i].get(), &restCoords, -1.0f);
    }
  }
}

void CDebugFrameGraph::Reset()
{
  m_frames = DEBUG_INFO_FRAMES{};
  // Assigning {} would keep the capacity
  m_pixels = std::vector<uint32_t>();
  m_strips.clear();
  m_stripChanged.clear();
  m_width = 0;
  m_height = 0;
  m_drawnCount = 0;
  m_redrawAll = true;
}

double CDebugFrameGraph::LinearMax() const
{
  const auto& frames = m_frames.frames;
  double linearMax = 1.0;
  for (size_t i = frames.size() - VisibleCount(); i < frames.size(); ++i)
    linearMax = std::max(linearMax, static_cast<double>(frames[i].size));
  return linearMax * 1.05;
}

void CDebugFrameGraph::DrawChanged()
{
  const int w = static_cast<int>(m_width);
  const uint64_t count = m_frames.count;
  if (!m_redrawAll && count == m_drawnCount)
    return;

  const double linearMax = m_logScale ? 0.0 : LinearMax();
  const double audioMax = AudioScaleMax();

  // A scale change moves everything, and so does more than a screen width of new packets
  if (m_redrawAll || linearMax != m_drawnLinearMax || audioMax != m_drawnAudioMax ||
      count - m_drawnCount + SWEEP_GAP + LabelColumns(m_width) + 5 >= m_width)
  {
    Draw(0, w - 1);
  }
  else
  {
    // The new packets, the previous newest one and the dots reaching into them, and in
    // sweep mode the cursor and the gap ahead of it
    const int first = static_cast<int>((m_drawnCount + m_width - 3) % m_width);
    // In scroll mode also the grid labels, which move with the oldest column
    const int length = static_cast<int>(count - m_drawnCount) + 5 +
                       (m_sweep ? static_cast<int>(SWEEP_GAP) : LabelColumns(m_width));
    const int last = first + length - 1;
    Draw(first, std::min(last, w - 1));
    if (last >= w)
      Draw(0, last - w);
  }

  m_redrawAll = false;
  m_drawnCount = count;
  m_drawnLinearMax = linearMax;
  m_drawnAudioMax = audioMax;
}

void CDebugFrameGraph::Draw(int x1, int x2)
{
  const int w = static_cast<int>(m_width);
  const int h = static_cast<int>(m_height);
  const auto& frames = m_frames.frames;

  for (int y = 0; y < h; ++y)
  {
    auto row = m_pixels.begin() + static_cast<ptrdiff_t>(y) * w;
    std::fill(row + x1, row + x2 + 1, COLOR_BACKDROP);
  }
  for (unsigned int strip = static_cast<unsigned int>(x1) / STRIP_WIDTH;
       strip <= static_cast<unsigned int>(x2) / STRIP_WIDTH; ++strip)
    m_stripChanged[strip] = true;

  const auto put = [&](int x, int y, uint32_t color)
  {
    if (x >= x1 && x <= x2 && y >= 0 && y < h)
      m_pixels[static_cast<size_t>(y) * w + x] = color;
  };
  const auto vline = [&](int x, int y1, int y2, uint32_t color)
  {
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
      put(x, y, color);
  };

  const size_t count = VisibleCount();
  const size_t first = frames.size() - count;

  // In band mode the video uses the upper part and the audio gets its own band below
  const double audioMax = AudioScaleMax();
  const bool band = audioMax > 0.0 && m_audioMode == 2;
  const int videoHeight =
      band ? std::max(1, static_cast<int>(std::lround(h * AUDIO_BAND_SPLIT))) : h;

  const double linearMax = m_logScale ? 0.0 : LinearMax();

  const auto toY = [&](double size)
  {
    double ratio = m_logScale ? (std::log(std::max(size, LOG_MIN)) - std::log(LOG_MIN)) /
                                    (std::log(LOG_MAX) - std::log(LOG_MIN))
                              : size / linearMax;
    ratio = std::clamp(ratio, 0.0, 1.0);
    return videoHeight - 1 - static_cast<int>(std::lround(ratio * (videoHeight - 1)));
  };
  const auto toX = [&](size_t i)
  { return static_cast<int>((m_frames.count - frames.size() + i) % m_width); };
  // Columns next to each other, also where the texture wraps around
  const auto adjacent = [&](int prevX, int x) { return prevX >= 0 && (prevX + 1) % w == x; };

  std::vector<double> gridSizes;
  if (m_logScale)
    gridSizes = {1e4, 1e5, 1e6};
  else
    gridSizes = {linearMax / 4, linearMax / 2, linearMax * 3 / 4, linearMax};

  for (const double size : gridSizes)
  {
    const int y = toY(size);
    for (int x = x1; x <= x2; ++x)
      put(x, y, COLOR_GRID);
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
      for (int dx = -radius; dx <= radius; ++dx)
      {
        // Keep dots within the shown packets, as columns wrap around at the edges
        const auto position = static_cast<ptrdiff_t>(i - first) + dx;
        if (position < 0 || position >= static_cast<ptrdiff_t>(count))
          continue;
        for (int dy = -radius; dy <= radius; ++dy)
          put((x + dx + w) % w, y + dy, color);
      }
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
    const int fromY = adjacent(prevX, x) ? prevY : y;
    vline(x, fromY, y, COLOR_AVERAGE);
    vline(x, fromY + 1, y + 1, COLOR_AVERAGE);
    prevX = x;
    prevY = y;
  }

  if (audioMax > 0.0)
  {
    const int audioTop = band ? videoHeight + 4 : 0;
    if (band)
    {
      for (int x = x1; x <= x2; ++x)
        put(x, videoHeight + 1, COLOR_GRID);
    }

    const auto& rates = m_frames.audioRates;
    int prevAudioX = -1;
    int prevAudioY = 0;
    for (size_t i = first; i < frames.size(); ++i)
    {
      if (std::isnan(rates[i]))
      {
        prevAudioX = -1;
        continue;
      }
      const double ratio = std::clamp(static_cast<double>(rates[i]) / audioMax, 0.0, 1.0);
      const int x = toX(i);
      const int y = h - 1 - static_cast<int>(std::lround(ratio * (h - 1 - audioTop)));
      const int fromY = adjacent(prevAudioX, x) ? prevAudioY : y;
      vline(x, fromY, y, COLOR_AUDIO);
      vline(x, fromY - 1, y - 1, COLOR_AUDIO);
      prevAudioX = x;
      prevAudioY = y;
    }
  }

  // The labels stay at the left edge of the screen, which in scroll mode is the oldest column
  const int font = FontSize(m_width);
  const int labelX = (m_sweep ? 0 : static_cast<int>(m_frames.count % m_width)) + 4 * font;
  for (const double size : gridSizes)
  {
    const int top = toY(size) - (GLYPH_HEIGHT + 2) * font;
    if (top < 0)
      continue;
    // A dark box behind the label keeps it readable over the dots
    const std::string label = FormatSize(size);
    const int labelWidth = static_cast<int>(label.size()) * (GLYPH_WIDTH + 1) * font + font;
    for (int y = top - font; y < top + (GLYPH_HEIGHT + 1) * font; ++y)
      for (int x = labelX - font; x < labelX + labelWidth; ++x)
        put((x + w) % w, y, COLOR_BACKDROP_LABEL);

    int x = labelX;
    for (const char c : label)
    {
      const auto glyph = GLYPH_CHARS.find(c);
      for (int row = 0; glyph != std::string_view::npos && row < GLYPH_HEIGHT; ++row)
        for (int col = 0; col < GLYPH_WIDTH; ++col)
          if (GLYPHS[glyph][row] & (0x10 >> col))
            for (int dy = 0; dy < font; ++dy)
              for (int dx = 0; dx < font; ++dx)
                put((x + col * font + dx) % w, top + row * font + dy, COLOR_GRID_LABEL);
      x += (GLYPH_WIDTH + 1) * font;
    }
  }

  if (m_sweep && count > 0)
    vline((toX(frames.size() - 1) + 1) % w, 0, h - 1, COLOR_CURSOR);
}
