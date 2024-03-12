/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUILabel.h
\brief
*/

#include "GUIFont.h"
#include "GUITextLayout.h"
#include "guiinfo/GUIInfoColor.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

class CLabelInfo
{
public:
  CLabelInfo():
    scrollSuffix(" | ")
  {
    font = NULL;
    align = XBFONT_LEFT;
    offsetX = offsetY = 0;
    width = 0;
    angle = 0;
    scrollSpeed = CScrollInfo::defaultSpeed;
  };
  bool UpdateColors()
  {
    bool changed = false;

    changed |= textColor.Update();
    changed |= shadowColor.Update();
    changed |= selectedColor.Update();
    changed |= disabledColor.Update();
    changed |= focusedColor.Update();
    changed |= invalidColor.Update();

    return changed;
  };

  KODI::GUILIB::GUIINFO::CGUIInfoColor textColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor shadowColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor selectedColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor disabledColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor focusedColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor invalidColor;
  uint32_t align;
  float offsetX;
  float offsetY;
  float width;
  float angle;
  CGUIFont *font;
  int scrollSpeed;
  std::string scrollSuffix;
};

/*!
 \ingroup controls, labels
 \brief Class for rendering text labels.  Handles alignment and rendering of text within a control.
 */
class CGUILabel
{
public:
  /*! \brief allowed color categories for labels, as defined by the skin
   */
  enum COLOR { COLOR_TEXT = 0,
               COLOR_SELECTED,
               COLOR_FOCUSED,
               COLOR_DISABLED,
               COLOR_INVALID };

  /*! \brief allowed overflow handling techniques for labels, as defined by the skin
   */
  enum OVER_FLOW
  {
    OVER_FLOW_TRUNCATE = 0, // Truncated text from right (text end with ellipses)
    OVER_FLOW_SCROLL,
    OVER_FLOW_WRAP,
    OVER_FLOW_CLIP,
    OVER_FLOW_TRUNCATE_LEFT // Truncated text from left (text start with ellipses)
  };

  CGUILabel(float posX, float posY, float width, float height, const CLabelInfo& labelInfo, OVER_FLOW overflow = OVER_FLOW_TRUNCATE);
  CGUILabel(const CGUILabel& label);

  virtual ~CGUILabel() = default;

  /*! \brief Process the label
   \return bool stating if process caused control to change
   */
  bool Process(unsigned int currentTime);

  /*! \brief Render the label on screen
   */
  void Render();

  /*! \brief Set the maximal extent of the label
   Sets the maximal size and positioning that the label may render in.  Note that `textwidth>` can override
   this, and `<textoffsetx>` and `<textoffsety>` may also allow the label to be moved outside this rectangle.
   */
  bool SetMaxRect(float x, float y, float w, float h);

  bool SetAlign(uint32_t align);

  /*! \brief Set the text to be displayed in the label
   Updates the label control and recomputes final position and size
   \param text std::string to set as this labels text
   \sa SetTextW, SetStyledText
   */
  bool SetText(const std::string &label);

  /*! \brief Set the text to be displayed in the label
   Updates the label control and recomputes final position and size
   \param text std::wstring to set as this labels text
   \sa SetText, SetStyledText
   */
  bool SetTextW(const std::wstring &label);

  /*! \brief Set styled text to be displayed in the label
   Updates the label control and recomputes final position and size
   \param text styled text to set.
   \param colors colors referenced in the styled text.
   \sa SetText, SetTextW
   */
  bool SetStyledText(const vecText& text, const std::vector<UTILS::COLOR::Color>& colors);

  /*! \brief Set the color to use for the label
   Sets the color to be used for this label.  Takes effect at the next render
   \param color color to be used for the label
   */
  bool SetColor(COLOR color);

  /*! \brief Set the final layout of the current text
   Overrides the calculated layout of the current text, forcing a particular size and position
   \param rect CRect containing the extents of the current text
   \sa GetRenderRect, UpdateRenderRect
   */
  void SetRenderRect(const CRect& rect) { m_renderRect = rect; }

  /*! \brief Set whether or not this label control should scroll
   \param scrolling true if this label should scroll.
   */
  bool SetScrolling(bool scrolling);

  /*! \brief Set max. text scroll count
  */
  void SetScrollLoopCount(unsigned int loopCount) { m_maxScrollLoops = loopCount; }

  /*! \brief Set how this label should handle overflowing text.
   \param overflow the overflow type
   \sa OVER_FLOW
   */
  bool SetOverflow(OVER_FLOW overflow);

  /*! \brief Set this label invalid.  Forces an update of the control
   */
  void SetInvalid();

  /*! \brief Update this labels colors
   */
  bool UpdateColors();

  /*! \brief Returns the precalculated final layout of the current text
   \return CRect containing the extents of the current text
   \sa SetRenderRect, UpdateRenderRect
   */
  const CRect& GetRenderRect() const { return m_renderRect; }

  /*! \brief Returns the precalculated full width of the current text, regardless of layout
   \return full width of the current text
   \sa CalcTextWidth
   */
  float GetTextWidth() const { return m_textLayout.GetTextWidth(); }

  /*! \brief Returns the maximal width that this label can render into
   \return Maximal width that this label can render into. Note that this may differ from the
           amount given in SetMaxRect as offsets and text width overrides have been taken into account.
   \sa SetMaxRect
   */
  float GetMaxWidth() const;

  /*! \brief Calculates the width of some text
   \param text std::wstring of text whose width we want
   \return width of the given text
   \sa GetTextWidth
   */
  float CalcTextWidth(const std::wstring& text) const { return m_textLayout.GetTextWidth(text); }

  const CLabelInfo& GetLabelInfo() const { return m_label; }
  CLabelInfo& GetLabelInfo() { return m_label; }

  /*! \brief Check a left aligned and right aligned label for overlap and cut the labels off so that no overlap occurs

   If a left-aligned label occupies some of the same space on screen as a right-aligned label, then we may be able to
   correct for this by restricting the width of one or both of them. This routine checks two labels to see whether they
   satisfy this assumption and, if so, adjusts the render rect of both labels so that they no longer do so.  The order
   of the two labels is not important, but we do assume that the left-aligned label is also the left-most on screen, and
   that the right-aligned label is the right most on-screen, so that they overlap due to the fact that one or both of
   the labels are longer than anticipated.  In the following diagram, [R...[R  R] refers to the maximal allowed and
   actual space occupied by the right label.  Similarly, [L   L]...L] refers to the maximal and actual space occupied
   by the left label.  | refers to the central cutting point, i.e. the point that would divide the maximal allowed
   overlap perfectly in two.  There are 3 scenarios to consider:

   cut
   [L       [R...[R  L].|..........L]         R]     left label ends to the left of the cut -> just crop the left label.
   [L       [R.....[R   |      L]..L]         R]     both left and right labels occupy more than the cut allows, so crop both.
   [L       [R..........|.[R   L]..L]         R]     right label ends to the right of the cut -> just crop the right label.

   \param label1 First label to check
   \param label2 Second label to check
   */
  static bool CheckAndCorrectOverlap(CGUILabel &label1, CGUILabel &label2);

protected:
  UTILS::COLOR::Color GetColor() const;

  /*! \brief Computes the final layout of the text
   Uses the maximal position and width of the text, as well as the text length
   and alignment to compute the final render rect of the text.
   \sa GetRenderRect, SetRenderRect
   */
  void UpdateRenderRect();

private:
  CLabelInfo     m_label;
  CGUITextLayout m_textLayout;

  bool           m_scrolling;
  OVER_FLOW m_overflowType;
  CScrollInfo    m_scrollInfo;
  CRect          m_renderRect;   ///< actual sizing of text
  CRect          m_maxRect;      ///< maximum sizing of text
  bool m_invalid = true; ///< if true, the label needs recomputing
  COLOR m_color = COLOR_TEXT; ///< color to render text \sa SetColor, GetColor
  unsigned int   m_maxScrollLoops = ~0U;
};
