/*!
\file GUILabel.h
\brief
*/

#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUITextLayout.h"
#include "GUIInfoTypes.h"
#include "GUIFont.h"
#include "Geometry.h"

class CLabelInfo
{
public:
  CLabelInfo()
  {
    font = NULL;
    align = XBFONT_LEFT;
    offsetX = offsetY = 0;
    width = 0;
    angle = 0;
  };
  void UpdateColors()
  {
    textColor.Update();
    shadowColor.Update();
    selectedColor.Update();
    disabledColor.Update();
    focusedColor.Update();
  };
  
  CGUIInfoColor textColor;
  CGUIInfoColor shadowColor;
  CGUIInfoColor selectedColor;
  CGUIInfoColor disabledColor;
  CGUIInfoColor focusedColor;
  uint32_t align;
  float offsetX;
  float offsetY;
  float width;
  float angle;
  CGUIFont *font;
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
               COLOR_DISABLED };
  
  /*! \brief allowed overflow handling techniques for labels, as defined by the skin
   */
  enum OVER_FLOW { OVER_FLOW_TRUNCATE = 0,
                   OVER_FLOW_SCROLL,
                   OVER_FLOW_WRAP };
  
  CGUILabel(float posX, float posY, float width, float height, const CLabelInfo& labelInfo, OVER_FLOW overflow = OVER_FLOW_TRUNCATE, int scrollSpeed = 0);
  virtual ~CGUILabel(void);

  /*! \brief Render the label on screen
   */
  void Render();
  
  /*! \brief Set the maximal extent of the label
   Sets the maximal size and positioning that the label may render in. Note that any offsets will be computed and applied immediately
   so that subsequent calls to GetMaxRect() may not return the same values.
   \sa GetMaxRect
   */
  void SetMaxRect(float x, float y, float w, float h);

  void SetAlign(uint32_t align);
  
  /*! \brief Set the text to be displayed in the label
   Updates the label control and recomputes final position and size
   \param text CStdString to set as this labels text
   \sa SetTextW
   */
  void SetText(const CStdString &label);

  /*! \brief Set the text to be displayed in the label
   Updates the label control and recomputes final position and size
   \param text CStdStringW to set as this labels text
   \sa SetText
   */
  void SetTextW(const CStdStringW &label);
  
  /*! \brief Set the color to use for the label
   Sets the color to be used for this label.  Takes effect at the next render
   \param color color to be used for the label
   */
  void SetColor(COLOR color);

  /*! \brief Set the final layout of the current text
   Overrides the calculated layout of the current text, forcing a particular size and position
   \param rect CRect containing the extents of the current text
   \sa GetRenderRect, UpdateRenderRect
   */
  void SetRenderRect(const CRect &rect) { m_renderRect = rect; };
  
  /*! \brief Set whether or not this label control should scroll
   \param scrolling true if this label should scroll.
   \param scrollSpeed speed (in pixels per second) at which the label should scroll
   */
  void SetScrolling(bool scrolling, int scrollSpeed = 0);
  
  /*! \brief Set this label invalid.  Forces an update of the control
   */
  void SetInvalid();
  
  /*! \brief Update this labels colors
   */
  void UpdateColors();
  
  /*! \brief Returns the precalculated final layout of the current text
   \return CRect containing the extents of the current text
   \sa SetRenderRect, UpdateRenderRect
   */
  const CRect &GetRenderRect() const { return m_renderRect; };
  
  /*! \brief Returns the precalculated full width of the current text, regardless of layout
   \return full width of the current text
   \sa CalcTextWidth
   */
  float GetTextWidth() const { return m_textLayout.GetTextWidth(); };
  
  /*! \brief Returns the maximal text rect that this label can render into
   \return CRect containing the maximal rectangle that this label can render into.  May differ from
           the sizing given in SetMaxRect as offsets have been applied.
   \sa SetMaxRect
   */
  const CRect &GetMaxRect() const { return m_maxRect; };
  
  /*! \brief Calculates the width of some text
   \param text CStdStringW of text whose width we want
   \return width of the given text
   \sa GetTextWidth
   */
  float CalcTextWidth(const CStdStringW &text) const { return m_textLayout.GetTextWidth(text); };

  const CLabelInfo& GetLabelInfo() const { return m_label; };
  CLabelInfo &GetLabelInfo() { return m_label; };
protected:
  color_t GetColor() const;
  
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
  OVER_FLOW      m_overflowType;
  bool           m_selected;
  CScrollInfo    m_scrollInfo;
  CRect          m_renderRect;   ///< actual sizing of text
  CRect          m_maxRect;      ///< maximum sizing of text
  bool           m_invalid;      ///< if true, the label needs recomputing
  COLOR          m_color;        ///< color to render text \sa SetColor, GetColor
};
