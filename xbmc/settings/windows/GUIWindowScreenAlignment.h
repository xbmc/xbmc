/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"
#include "threads/CriticalSection.h"
#include "utils/AspectRatioVocabulary.h"
#include "utils/ColorUtils.h"

#include <memory>
#include <optional>
#include <set>
#include <vector>

class CGUIFont;
class CGUITextLayout;

/*!
 * \brief Frames at the aspect ratios Kodi knows, drawn on the raw pixel grid to align optics to.
 *
 * Each enabled ratio draws its largest centred frame at its true shape, so the viewer can move
 * the projector until the frame for the shape of their screen lands on their screen.
 *
 * Not skinnable: it carries no XML, no controls and no skin font. The raster, the overscan
 * calibration and the interface's proportions are all suspended while it is on screen.
 */
class CGUIWindowScreenAlignment : public CGUIWindow
{
public:
  CGUIWindowScreenAlignment();
  ~CGUIWindowScreenAlignment() override;

  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;

  //! \brief The ratios whose frames are being drawn, ascending.
  std::vector<float> ShownRatios() const;

  //! \brief Draw the frames for these ratios and no others. Matched against the vocabulary
  //! exactly, and one it does not hold is dropped - read ShownRatios() back.
  void SetShownRatios(const std::vector<float>& ratios);

private:
  //! \brief The pixel grid, which is the resolution with the overscan calibration undone: a tool
  //! shown inside the calibrated area would be aligned against a previous correction.
  RESOLUTION_INFO RawGrid() const;

  //! \brief The colour of the frame for the entry at \p index in the vocabulary.
  static KODI::UTILS::COLOR::Color FrameColour(size_t index);

  void DrawFrame(const CRect& rect, KODI::UTILS::COLOR::Color colour, float thickness);
  void DrawLabel(float x,
                 float y,
                 uint32_t alignment,
                 KODI::UTILS::COLOR::Color colour,
                 const std::string& text);
  void DrawLegend(const CRect& display, const std::set<int>& shown);

  //! \brief The height of one row of the legend, zero before there is a font to measure it with.
  float RowHeight() const;

  //! \brief Where the legend's \p index row sits on the screen. Every row is the width of the
  //! widest, so the box is a target rather than a fit.
  CRect RowRect(const CRect& display, size_t index) const;

  std::optional<size_t> RowAt(const CPoint& point) const;

  void ToggleRow(size_t index);

  //! \brief Show the shape of the screen the viewer has already stated. Once per session, so a
  //! set chosen by hand or over the API survives leaving the tool.
  void ApplyDefaultRatios();

  std::vector<KODI::UTILS::AspectRatioEntry> m_entries;

  mutable CCriticalSection m_stateSection;
  std::set<int> m_shown;
  bool m_defaulted{false};

  size_t m_cursor{0};

  CGUIFont* m_font{nullptr};
  std::unique_ptr<CGUITextLayout> m_layout;

  //! \brief The widest row, measured once when the font is loaded. Measured with a ticked box, so
  //! ticking one cannot change the width of the column under the pointer.
  float m_rowWidth{0.0f};
};
