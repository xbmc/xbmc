/*!
\file GUIFadeLabelControl.h
\brief
*/

#ifndef GUILIB_GUIFADELABELCONTROL_H
#define GUILIB_GUIFADELABELCONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
class CGUIFadeLabelControl : public CGUIControl
{
public:
  CGUIFadeLabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, unsigned int timeToDelayAtEnd, bool resetOnLabelChange);
  CGUIFadeLabelControl(const CGUIFadeLabelControl &from);
  virtual ~CGUIFadeLabelControl(void);
  virtual CGUIFadeLabelControl *Clone() const { return new CGUIFadeLabelControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  void SetInfo(const std::vector<CGUIInfoLabel> &vecInfo);

protected:
  virtual bool UpdateColors();
  virtual CStdString GetDescription() const;
  void AddLabel(const std::string &label);

  /*! \brief retrieve the current label for display

   The fadelabel has multiple labels which it cycles through. This routine retrieves the current label.
   It first checks the current label and if non-empty returns it. Otherwise it will iterate through all labels
   until it has a non-empty label to return.

   \return the label that should be displayed.  If empty, there is no label available.
   */
  CStdString GetLabel();

  std::vector< CGUIInfoLabel > m_infoLabels;
  unsigned int m_currentLabel;
  unsigned int m_lastLabel;

  CLabelInfo m_label;

  bool m_scrollOut;   // true if we scroll the text all the way to the left before fading in the next label
  bool m_shortText;   // true if the text we have is shorter than the width of the control

  CScrollInfo m_scrollInfo;
  CGUITextLayout m_textLayout;
  CAnimation m_fadeAnim;
  TransformMatrix m_fadeMatrix;
  unsigned int m_scrollSpeed;
  bool m_resetOnLabelChange;
};
#endif
