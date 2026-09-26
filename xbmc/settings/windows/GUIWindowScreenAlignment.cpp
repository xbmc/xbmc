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

//! \brief The frame the tool opens with.
constexpr float DEFAULT_RATIO = 1.78f;

/*!
 * \brief A distinct colour for the frame at \p index.
 *
 * Walked round the hue circle by the golden angle, so consecutive ratios are far apart and no
 * number of them ever repeats a colour. A fixed list would repeat as soon as a definition file
 * added ratios past the end of it, and two frames the same colour is the one thing this tool
 * cannot afford. Held off full saturation because a saturated primary on black reads as a
 * coloured glow rather than an edge.
 */
KODI::UTILS::COLOR::Color FrameColour(size_t index)
{
  constexpr float GOLDEN_ANGLE = 137.50776f;
  constexpr float SATURATION = 0.68f;

  const float hue = std::fmod(static_cast<float>(index) * GOLDEN_ANGLE, 360.0f) / 60.0f;
  const float rise = SATURATION * (1.0f - std::fabs(std::fmod(hue, 2.0f) - 1.0f));
  const float base = 1.0f - SATURATION;

  float red = base;
  float green = base;
  float blue = base;

  switch (static_cast<int>(hue))
  {
    case 0:
      red += SATURATION;
      green += rise;
      break;
    case 1:
      red += rise;
      green += SATURATION;
      break;
    case 2:
      green += SATURATION;
      blue += rise;
      break;
    case 3:
      green += rise;
      blue += SATURATION;
      break;
    case 4:
      red += rise;
      blue += SATURATION;
      break;
    default:
      red += SATURATION;
      blue += rise;
      break;
  }

  const auto channel = [](float value)
  { return static_cast<uint32_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)); };

  return 0xFF000000 | (channel(red) << 16) | (channel(green) << 8) | channel(blue);
}

std::string RowText(const AspectRatioEntry& entry, bool enabled)
{
  return StringUtils::Format("{} {}", enabled ? "[x]" : "[ ]",
                             CAspectRatioVocabulary::ChoiceLabel(entry));
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

      m_entries = CAspectRatioVocabulary::Entries();
      if (!m_entries.empty())
        m_cursor = std::min(m_cursor, m_entries.size() - 1);
      ApplyDefaultRatios();

      const RESOLUTION_INFO grid = RawGrid();

      // A fraction of the raster, not a size, with a floor.
      const int size = std::max(12, static_cast<int>(std::lround(grid.iHeight / 36.0f)));
      m_font = g_fontManager.LoadTTF(FONT_NAME, "arial.ttf", 0xFFFFFFFF, 0, size, FONT_STYLE_NORMAL,
                                     false, 1.0f, 1.0f, &grid);

      m_rowWidth = 0.0f;
      m_rowLayouts.clear();
      m_frameLabels.clear();
      m_rowLayouts.reserve(m_entries.size());
      m_frameLabels.reserve(m_entries.size());

      for (size_t i = 0; i < m_entries.size(); ++i)
      {
        m_frameLabels.emplace_back(MakeLayout(CAspectRatioVocabulary::ChoiceLabel(m_entries[i])));

        // Measured with a ticked box, the wider of the two, so a row keeps its width as it is
        // ticked and unticked.
        std::unique_ptr<CGUITextLayout> row = MakeLayout(RowText(m_entries[i], true));
        if (row)
        {
          float width = 0.0f;
          float height = 0.0f;
          row->GetTextExtent(width, height);
          m_rowWidth = std::max(m_rowWidth, width);
          row->Update(RowText(m_entries[i], IsShown(i)));
        }
        m_rowLayouts.emplace_back(std::move(row));
      }

      MarkChanged();
      return true;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      m_rowLayouts.clear();
      m_frameLabels.clear();
      if (m_font)
      {
        g_fontManager.Unload(FONT_NAME);
        m_font = nullptr;
      }
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
      MarkChanged();
      return true;

    case ACTION_MOVE_DOWN:
      m_cursor = (m_cursor + 1) % m_entries.size();
      MarkChanged();
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
      if (row && *row != m_cursor)
      {
        m_cursor = *row;
        MarkChanged();
      }
      return true;
    }

    default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowScreenAlignment::MarkChanged()
{
  CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
}

bool CGUIWindowScreenAlignment::IsShown(size_t index) const
{
  return index < m_entries.size() &&
         m_shown.contains(CAspectRatioVocabulary::Key(m_entries[index].ratio));
}

std::unique_ptr<CGUITextLayout> CGUIWindowScreenAlignment::MakeLayout(const std::string& text) const
{
  if (!m_font)
    return nullptr;

  auto layout = std::make_unique<CGUITextLayout>(m_font, false, 0.0f);
  layout->Update(text);
  return layout;
}

void CGUIWindowScreenAlignment::ToggleRow(size_t index)
{
  if (index >= m_entries.size())
    return;

  const int key = CAspectRatioVocabulary::Key(m_entries[index].ratio);

  if (!m_shown.erase(key))
    m_shown.insert(key);

  if (m_rowLayouts[index])
    m_rowLayouts[index]->Update(RowText(m_entries[index], IsShown(index)));

  MarkChanged();
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
    if (!IsShown(i))
      continue;

    const CRect frame =
        KODI::WINDOWING::ComputeAspectRect(display, m_entries[i].ratio, grid.fPixelRatio);
    const KODI::UTILS::COLOR::Color colour = FrameColour(i);

    DrawFrame(frame, colour, thickness);
    if (m_frameLabels[i])
    {
      DrawLabel(frame.x1 + thickness * 2.0f, frame.y1 + thickness * 2.0f, XBFONT_LEFT, colour,
                *m_frameLabels[i]);
    }
  }

  DrawLegend(display);

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
    float x, float y, uint32_t alignment, KODI::UTILS::COLOR::Color colour, CGUITextLayout& text)
{
  text.Render(x, y, 0.0f, colour, 0, alignment, 0.0f);
}

void CGUIWindowScreenAlignment::DrawLegend(const CRect& display)
{
  if (m_entries.empty() || RowHeight() <= 0.0f)
    return;

  for (size_t i = 0; i < m_entries.size(); ++i)
  {
    const CRect row = RowRect(display, i);

    if (i == m_cursor)
      CGUITexture::DrawQuad(row, 0x40FFFFFF);

    if (m_rowLayouts[i])
      DrawLabel((row.x1 + row.x2) * 0.5f, row.y1, XBFONT_CENTER_X, FrameColour(i),
                *m_rowLayouts[i]);
  }
}

void CGUIWindowScreenAlignment::ApplyDefaultRatios()
{
  if (m_defaulted)
    return;

  m_defaulted = true;

  // Only when the vocabulary in force has it: a definition may rename or drop a ratio, and
  // opening on one that draws nothing would read as the tool being broken.
  const int key = CAspectRatioVocabulary::Key(DEFAULT_RATIO);
  const bool known =
      std::ranges::any_of(m_entries, [key](const AspectRatioEntry& entry)
                          { return CAspectRatioVocabulary::Key(entry.ratio) == key; });
  if (known)
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
