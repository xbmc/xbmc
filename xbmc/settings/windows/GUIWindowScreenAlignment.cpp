/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowScreenAlignment.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUITexture.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "windowing/GraphicContext.h"
#include "windowing/GuiGeometry.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cmath>
#include <mutex>

using namespace KODI::UTILS;

namespace
{
//! \brief Loaded from Kodi's own font rather than the skin's, so the instrument reads the same
//! whatever is installed. RenderSystem does the same for the splash message.
constexpr const char* FONT_NAME = "__screenalignment__";

//! \brief The frame colours, in the order the vocabulary hands entries out. Each carries some of
//! the other channels, fully saturated primaries clipping on a badly set-up projector.
constexpr KODI::UTILS::COLOR::Color FRAME_COLOURS[] = {
    0xFFFF5050, // red
    0xFF50FF70, // green
    0xFF6090FF, // blue
    0xFFFFE050, // yellow
    0xFFFF70FF, // magenta
    0xFF50E0FF, // cyan
    0xFFFF9840, // orange
    0xFFB080FF, // violet
};

//! \brief 16:9, the frame to start from when the viewer has not said their screen is another
//! shape.
constexpr float DEFAULT_RATIO = 1.78f;

std::string RowText(const KODI::UTILS::AspectRatioEntry& entry, bool enabled)
{
  return StringUtils::Format("{} {}", enabled ? "[x]" : "[ ]",
                             KODI::UTILS::CAspectRatioVocabulary::ChoiceLabel(entry));
}
} // namespace

CGUIWindowScreenAlignment::CGUIWindowScreenAlignment() : CGUIWindow(WINDOW_SCREEN_ALIGNMENT, "")
{
  // Everything is drawn in screen pixels. There is no layout to scale, and scaling is what this
  // window exists to be free of.
  m_needsScaling = false;
}

CGUIWindowScreenAlignment::~CGUIWindowScreenAlignment() = default;

bool CGUIWindowScreenAlignment::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

      CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();
      context.SetCalibrating(true);

      m_entries = CAspectRatioVocabulary::Entries();
      if (!m_entries.empty())
        m_cursor = std::min(m_cursor, m_entries.size() - 1);
      ApplyDefaultRatios();

      const RESOLUTION_INFO grid = RawGrid();

      // A fraction of the raster, not a size: this window takes no skin scaling, so a fixed
      // one would be half as tall again at 2160 as at 1080. Floored to stay legible in a window.
      const int size = std::max(12, static_cast<int>(std::lround(grid.iHeight / 36.0f)));
      m_font = g_fontManager.LoadTTF(FONT_NAME, "arial.ttf", 0xFFFFFFFF, 0, size, FONT_STYLE_NORMAL,
                                     false, 1.0f, 1.0f, &grid);
      if (m_font)
        m_layout = std::make_unique<CGUITextLayout>(m_font, false, 0.0f);

      m_rowWidth = 0.0f;
      if (m_layout)
      {
        for (const AspectRatioEntry& entry : m_entries)
        {
          float width = 0.0f;
          float height = 0.0f;
          m_layout->Update(RowText(entry, true));
          m_layout->GetTextExtent(width, height);
          m_rowWidth = std::max(m_rowWidth, width);
        }
      }

      // The interface behind is laid out against the raster, which has just been suspended, so it
      // has to measure itself again before it is seen on the way out.
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
          GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_ALIGNMENT, 0, GUI_MSG_WINDOW_RESIZE);
      return true;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      m_layout.reset();
      if (m_font)
      {
        g_fontManager.Unload(FONT_NAME);
        m_font = nullptr;
      }

      CServiceBroker::GetWinSystem()->GetGfxContext().SetCalibrating(false);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
          GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_ALIGNMENT, 0, GUI_MSG_WINDOW_RESIZE);
      break;
    }

    default:
      break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowScreenAlignment::OnAction(const CAction& action)
{
  if (m_entries.empty())
    return CGUIWindow::OnAction(action);

  switch (action.GetID())
  {
    case ACTION_MOVE_UP:
      m_cursor = m_cursor == 0 ? m_entries.size() - 1 : m_cursor - 1;
      return true;

    case ACTION_MOVE_DOWN:
      m_cursor = (m_cursor + 1) % m_entries.size();
      return true;

    case ACTION_SELECT_ITEM:
      ToggleRow(m_cursor);
      return true;

    // Caught here rather than left to the base class, which would hunt for a control to deliver
    // it to and this window has none. The pointer is in screen coordinates, as the legend is.
    case ACTION_MOUSE_LEFT_CLICK:
    {
      const std::optional<size_t> row = RowAt({action.GetAmount(0), action.GetAmount(1)});
      if (!row)
        return true;

      m_cursor = *row;
      ToggleRow(*row);
      return true;
    }

    case ACTION_MOUSE_MOVE:
    {
      const std::optional<size_t> row = RowAt({action.GetAmount(0), action.GetAmount(1)});
      if (row)
        m_cursor = *row;
      return true;
    }

    default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowScreenAlignment::ToggleRow(size_t index)
{
  if (index >= m_entries.size())
    return;

  const int key = CAspectRatioVocabulary::Key(m_entries[index].ratio);

  std::unique_lock lock(m_stateSection);
  if (!m_shown.erase(key))
    m_shown.insert(key);
}

float CGUIWindowScreenAlignment::RowHeight() const
{
  if (!m_font)
    return 0.0f;

  return m_font->GetLineHeight() * 1.4f;
}

CRect CGUIWindowScreenAlignment::RowRect(const CRect& display, size_t index) const
{
  const float rowHeight = RowHeight();
  const float padding = rowHeight * 0.18f;

  // Centred: the frames are all at the edges, so the middle is the one part none reaches.
  const float centreX = (display.x1 + display.x2) * 0.5f;
  const float top =
      (display.y1 + display.y2) * 0.5f - rowHeight * m_entries.size() * 0.5f + rowHeight * index;

  return {centreX - m_rowWidth * 0.5f - padding, top, centreX + m_rowWidth * 0.5f + padding,
          top + rowHeight};
}

std::optional<size_t> CGUIWindowScreenAlignment::RowAt(const CPoint& point) const
{
  if (m_entries.empty() || RowHeight() <= 0.0f)
    return std::nullopt;

  const RESOLUTION_INFO grid = RawGrid();
  const CRect display{0.0f, 0.0f, static_cast<float>(grid.iWidth),
                      static_cast<float>(grid.iHeight)};

  for (size_t i = 0; i < m_entries.size(); ++i)
  {
    if (RowRect(display, i).PtInRect(point))
      return i;
  }

  return std::nullopt;
}

void CGUIWindowScreenAlignment::DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // The pattern is static until something touches it, and the API can change it from another
  // thread between two frames. Marked every frame, as the calibration screen does.
  MarkDirtyRegion();
  CGUIWindow::DoProcess(currentTime, dirtyregions);
}

void CGUIWindowScreenAlignment::Render()
{
  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();

  // Unscaled, so a coordinate here is a pixel on the display: no skin resolution, no interface
  // zoom, and no overscan inset between what is computed and what is lit.
  context.SetRenderingResolution(context.GetResInfo(), false);

  const RESOLUTION_INFO grid = RawGrid();
  const CRect display{0.0f, 0.0f, static_cast<float>(grid.iWidth),
                      static_cast<float>(grid.iHeight)};

  CGUITexture::DrawQuad(display, 0xFF000000);

  // Two pixels at 1080, and never one: a single-pixel line at an odd position is resampled by
  // the display into something a viewer would align against.
  const float thickness = std::max(2.0f, std::round(grid.iHeight / 540.0f));

  std::set<int> shown;
  {
    std::unique_lock lock(m_stateSection);
    shown = m_shown;
  }

  for (size_t i = 0; i < m_entries.size(); ++i)
  {
    const AspectRatioEntry& entry = m_entries[i];
    if (!shown.contains(CAspectRatioVocabulary::Key(entry.ratio)))
      continue;

    // The same arithmetic the raster is built with, so a frame and the band Kodi would operate in
    // for that shape are the same rectangle to the pixel.
    const CRect frame = KODI::WINDOWING::ComputeRasterRect(grid, entry.ratio);
    const KODI::UTILS::COLOR::Color colour = FrameColour(i);

    DrawFrame(frame, colour, thickness);
    DrawLabel(frame.x1 + thickness * 2.0f, frame.y1 + thickness * 2.0f, XBFONT_LEFT, colour,
              CAspectRatioVocabulary::ChoiceLabel(entry));
  }

  DrawLegend(display, shown);

  CGUIWindow::Render();
}

void CGUIWindowScreenAlignment::DrawFrame(const CRect& rect,
                                          KODI::UTILS::COLOR::Color colour,
                                          float thickness)
{
  // Drawn inward from the edges: a frame whose line straddled the boundary would put half its
  // width outside the shape it claims to be, which is the one thing being measured here.
  CGUITexture::DrawQuad({rect.x1, rect.y1, rect.x2, rect.y1 + thickness}, colour);
  CGUITexture::DrawQuad({rect.x1, rect.y2 - thickness, rect.x2, rect.y2}, colour);
  CGUITexture::DrawQuad({rect.x1, rect.y1 + thickness, rect.x1 + thickness, rect.y2 - thickness},
                        colour);
  CGUITexture::DrawQuad({rect.x2 - thickness, rect.y1 + thickness, rect.x2, rect.y2 - thickness},
                        colour);
}

void CGUIWindowScreenAlignment::DrawLabel(
    float x, float y, uint32_t alignment, KODI::UTILS::COLOR::Color colour, const std::string& text)
{
  if (!m_layout)
    return;

  m_layout->Update(text);
  m_layout->Render(x, y, 0.0f, colour, 0, alignment, 0.0f);
}

void CGUIWindowScreenAlignment::DrawLegend(const CRect& display, const std::set<int>& shown)
{
  if (!m_layout || m_entries.empty() || RowHeight() <= 0.0f)
    return;

  for (size_t i = 0; i < m_entries.size(); ++i)
  {
    const AspectRatioEntry& entry = m_entries[i];
    const CRect row = RowRect(display, i);

    if (i == m_cursor)
      CGUITexture::DrawQuad(row, 0x40FFFFFF);

    DrawLabel((row.x1 + row.x2) * 0.5f, row.y1, XBFONT_CENTER_X, FrameColour(i),
              RowText(entry, shown.contains(CAspectRatioVocabulary::Key(entry.ratio))));
  }
}

void CGUIWindowScreenAlignment::ApplyDefaultRatios()
{
  std::unique_lock lock(m_stateSection);
  if (m_defaulted)
    return;

  m_defaulted = true;

  int key = 0;
  if (const auto settings = CServiceBroker::GetSettingsComponent())
  {
    if (const auto values = settings->GetSettings())
      key = values->GetInt(CSettings::SETTING_VIDEOSCREEN_RASTERASPECT);
  }

  if (CAspectRatioVocabulary::RatioForKey(key) <= 0.0f)
    key = CAspectRatioVocabulary::Key(DEFAULT_RATIO);

  m_shown.insert(key);
}

RESOLUTION_INFO CGUIWindowScreenAlignment::RawGrid() const
{
  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();

  RESOLUTION_INFO grid = context.GetResInfo();
  context.ResetOverscan(grid);
  grid.guiInsets = {};

  return grid;
}

KODI::UTILS::COLOR::Color CGUIWindowScreenAlignment::FrameColour(size_t index)
{
  return FRAME_COLOURS[index % std::size(FRAME_COLOURS)];
}

std::vector<float> CGUIWindowScreenAlignment::ShownRatios() const
{
  std::unique_lock lock(m_stateSection);

  std::vector<float> ratios;
  ratios.reserve(m_shown.size());
  for (const int key : m_shown)
    ratios.push_back(CAspectRatioVocabulary::RatioForKey(key));

  return ratios;
}

void CGUIWindowScreenAlignment::SetShownRatios(const std::vector<float>& ratios)
{
  std::unique_lock lock(m_stateSection);

  // A set stated over the API is the whole set, so an installer stepping through presets names
  // one ratio rather than remembering what it turned on last.
  m_shown.clear();
  m_defaulted = true;

  for (const float ratio : ratios)
  {
    const int key = CAspectRatioVocabulary::Key(ratio);
    if (CAspectRatioVocabulary::RatioForKey(key) > 0.0f)
      m_shown.insert(key);
  }
}
