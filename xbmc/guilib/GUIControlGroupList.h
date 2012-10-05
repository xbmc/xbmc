/*!
\file GUIControlGroupList.h
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

#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief list of controls that is scrollable
 */
class CGUIControlGroupList : public CGUIControlGroup
{
public:
  CGUIControlGroupList(int parentID, int controlID, float posX, float posY, float width, float height, float itemGap, int pageControl, ORIENTATION orientation, bool useControlPositions, uint32_t alignment, const CScroller& scroller);
  virtual ~CGUIControlGroupList(void);
  virtual CGUIControlGroupList *Clone() const { return new CGUIControlGroupList(*this); };

  virtual float GetWidth() const;
  virtual float GetHeight() const;
  virtual float Size() const;

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);

  virtual EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void AddControl(CGUIControl *control, int position = -1);
  virtual void ClearAll();

  virtual bool GetCondition(int condition, int data) const;
  /**
   * Calculate total size of child controls area (including gaps between controls)
   */
  float GetTotalSize() const;
  ORIENTATION GetOrientation() const { return m_orientation; }

  // based on grouplist orientation pick one value as minSize;
  void SetMinSize(float minWidth, float minHeight);
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  bool IsFirstFocusableControl(const CGUIControl *control) const;
  bool IsLastFocusableControl(const CGUIControl *control) const;
  void ValidateOffset();
  inline float Size(const CGUIControl *control) const;
  void ScrollTo(float offset);
  float GetAlignOffset() const;

  float m_itemGap;
  int m_pageControl;

  float m_totalSize;

  CScroller m_scroller;

  bool m_useControlPositions;
  ORIENTATION m_orientation;
  uint32_t m_alignment;

  // for autosizing
  float m_minSize;
};

