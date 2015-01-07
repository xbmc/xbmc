/*!
\file GUILabelControl.h
\brief
*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIControl.h"
#include "GUILabel.h"

/*!
 \ingroup controls
 \brief
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual CGUILabelControl *Clone() const { return new CGUILabelControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual std::string GetDescription() const;
  virtual float GetWidth() const;
  virtual void SetWidth(float width);
  virtual CRect CalcRenderRegion() const;
 
  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); };
  void SetLabel(const std::string &strLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};
  void SetInfo(const CGUIInfoLabel&labelInfo);
  void SetWidthControl(float minWidth, bool bScroll);
  void SetAlignment(uint32_t align);
  void SetHighlight(unsigned int start, unsigned int end);
  void SetSelection(unsigned int start, unsigned int end);

protected:
  bool UpdateColors();
  std::string ShortenPath(const std::string &path);

  /*! \brief Return the maximum width of this label control.
   \return Return the width of the control if available, else the width of the current text.
   */
  float GetMaxWidth() const { return m_width ? m_width : m_label.GetTextWidth(); }

  CGUILabel m_label;

  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  unsigned int m_dwCounter;

  // stuff for autowidth
  float m_minWidth;

  // multi-info stuff
  CGUIInfoLabel m_infoLabel;

  unsigned int m_startHighlight;
  unsigned int m_endHighlight;
  unsigned int m_startSelection;
  unsigned int m_endSelection;
};
#endif
