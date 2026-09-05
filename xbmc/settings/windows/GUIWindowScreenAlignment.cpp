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
#include "utils/StringUtils.h"
#include "windowing/GraphicContext.h"
#include "windowing/GuiGeometry.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cmath>

using namespace KODI::UTILS;

namespace
{
constexpr const char* FONT_NAME = "__screenalignment__";

//! \brief The frame colours, in the order ratios are enumerated. None is a saturated primary.
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

//! \brief The frame the tool opens with.
constexpr float DEFAULT_RATIO = 1.78f;

std::string RowText(const KODI::UTILS::AspectRatioEntry& entry, bool enabled)
{
  return StringUtils::Format("{} {}", enabled ? "[x]" : "[ ]",
                             KODI::UTILS::CAspectRatioVocabulary::ChoiceLabel(entry));
}
} // namespace

CGUIWindowScreenAlignment::CGUIWindowScreenAlignment() : CGUIWindow(WINDOW_SCREEN_ALIGNMENT, "")
{
  // Everything in this window is drawn in screen pixels.
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

      // A fraction of the raster, not a size, with a floor.
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

    // The pointer is in screen coordinates, as the legend is.
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
  MarkDirtyRegion();
  CGUIWindow::DoProcess(currentTime, dirtyregions);
}

void CGUIWindowScreenAlignment::Render()
{
  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();

  // Unscaled, so a coordinate here is a pixel on the display.
  context.SetRenderingResolution(context.GetResInfo(), false);

  const RESOLUTION_INFO grid = RawGrid();
  const CRect display{0.0f, 0.0f, static_cast<float>(grid.iWidth),
                      static_cast<float>(grid.iHeight)};

  CGUITexture::DrawQuad(display, 0xFF000000);

  const float thickness = std::max(2.0f, std::round(grid.iHeight / 540.0f));

  for (size_t i = 0; i < m_entries.size(); ++i)
  {
    const AspectRatioEntry& entry = m_entries[i];
    if (!m_shown.contains(CAspectRatioVocabulary::Key(entry.ratio)))
      continue;

    const CRect frame = KODI::WINDOWING::ComputeRasterRect(grid, entry.ratio);
    const KODI::UTILS::COLOR::Color colour = FrameColour(i);

    DrawFrame(frame, colour, thickness);
    DrawLabel(frame.x1 + thickness * 2.0f, frame.y1 + thickness * 2.0f, XBFONT_LEFT, colour,
              CAspectRatioVocabulary::ChoiceLabel(entry));
  }

  DrawLegend(display, m_shown);

  CGUIWindow::Render();
}

void CGUIWindowScreenAlignment::DrawFrame(const CRect& rect,
                                          KODI::UTILS::COLOR::Color colour,
                                          float thickness)
{
  // Drawn inward from the edges.
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
  if (m_defaulted)
    return;

  m_defaulted = true;
  m_shown.insert(CAspectRatioVocabulary::Key(DEFAULT_RATIO));
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
