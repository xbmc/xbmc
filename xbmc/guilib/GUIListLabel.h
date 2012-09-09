/*!
\file GUIListLabel.h
\brief
*/

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
class CGUIListLabel :
      public CGUIControl
{
public:
  CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoLabel &label, bool alwaysScroll);
  virtual ~CGUIListLabel(void);
  virtual CGUIListLabel *Clone() const { return new CGUIListLabel(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const { return false; };
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual void SetFocus(bool focus);
  virtual void SetInvalid();
  virtual void SetWidth(float width);

  void SetLabel(const CStdString &label);
  void SetSelected(bool selected);
  void SetScrolling(bool scrolling);

  static void CheckAndCorrectOverlap(CGUIListLabel &label1, CGUIListLabel &label2)
  {
    CGUILabel::CheckAndCorrectOverlap(label1.m_label, label2.m_label);
  }

  virtual CRect CalcRenderRegion() const;

protected:
  virtual bool UpdateColors();

  CGUILabel     m_label;
  CGUIInfoLabel m_info;
  bool          m_alwaysScroll;
};
