/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"
#include "utils/AspectRatioVocabulary.h"
#include "utils/ColorUtils.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class CGUIFont;
class CGUITextLayout;

/*!
 * \brief Frames at the aspect ratios Kodi knows, drawn on the raw pixel grid to align optics
 * to. Each enabled ratio draws its largest centred frame at its true shape.
 *
 * Not skinnable: no XML, no controls, no skin font. The raster, the overscan calibration and
 * the interface's proportions are all suspended while it is on screen.
 */
class CGUIWindowScreenAlignment : public CGUIWindow
{
public:
  CGUIWindowScreenAlignment();
  ~CGUIWindowScreenAlignment() override;

  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void Render() override;

private:
  //! \brief The pixel grid: the resolution with the overscan calibration undone.
  RESOLUTION_INFO RawGrid() const;

  //! \brief Redraw. Nothing here changes on its own, so this is the only thing that asks for a
  //! frame, and the window costs nothing between one keypress and the next.
  static void MarkChanged();

  //! \brief Whether the entry at \p index is drawn.
  bool IsShown(size_t index) const;

  //! \brief Build a layout holding \p text, or null when there is no font.
  std::unique_ptr<CGUITextLayout> MakeLayout(const std::string& text) const;

  void DrawFrame(const CRect& rect, KODI::UTILS::COLOR::Color colour, float thickness);
  void DrawLabel(
      float x, float y, uint32_t alignment, KODI::UTILS::COLOR::Color colour, CGUITextLayout& text);
  void DrawLegend(const CRect& display);

  //! \brief The height of one row of the legend, zero before there is a font to measure it with.
  float RowHeight() const;

  //! \brief Where the legend's \p index row sits. Every row is the width of the widest.
  CRect RowRect(const CRect& display, size_t index) const;

  std::optional<size_t> RowAt(const CPoint& point) const;

  void ToggleRow(size_t index);

  //! \brief Show the frame the tool opens with, once per session.
  void ApplyDefaultRatios();

  std::vector<KODI::UTILS::AspectRatioEntry> m_entries;

  std::set<int> m_shown;
  bool m_defaulted{false};

  size_t m_cursor{0};

  CGUIFont* m_font{nullptr};

  //! \brief One layout per entry, so drawing a frame never re-lays-out its text. The legend's
  //! rows carry a tick and are rebuilt when it changes; the frame labels never change.
  std::vector<std::unique_ptr<CGUITextLayout>> m_rowLayouts;
  std::vector<std::unique_ptr<CGUITextLayout>> m_frameLabels;

  //! \brief The widest row, measured once when the font is loaded, with a ticked box.
  float m_rowWidth{0.0f};
};
