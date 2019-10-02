/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIControlGroupList.h
\brief
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
  ~CGUIControlGroupList(void) override;
  CGUIControlGroupList *Clone() const override { return new CGUIControlGroupList(*this); };

  float GetWidth() const override;
  float GetHeight() const override;
  virtual float Size() const;
  void SetInvalid() override;

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnMessage(CGUIMessage& message) override;

  EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event) override;
  void UnfocusFromPoint(const CPoint &point) override;

  void AddControl(CGUIControl *control, int position = -1) override;
  void ClearAll() override;

  virtual std::string GetLabel(int info) const;
  bool GetCondition(int condition, int data) const override;
  /**
   * Calculate total size of child controls area (including gaps between controls)
   */
  float GetTotalSize() const;
  ORIENTATION GetOrientation() const { return m_orientation; }

  // based on grouplist orientation pick one value as minSize;
  void SetMinSize(float minWidth, float minHeight);
protected:
  EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) override;
  bool IsControlOnScreen(float pos, const CGUIControl* control) const;
  bool IsFirstFocusableControl(const CGUIControl *control) const;
  bool IsLastFocusableControl(const CGUIControl *control) const;
  void ValidateOffset();
  void CalculateItemGap();
  inline float Size(const CGUIControl *control) const;
  void ScrollTo(float offset);
  float GetAlignOffset() const;

  int GetNumItems() const;
  int GetSelectedItem() const;

  float m_itemGap;
  int m_pageControl;
  int m_focusedPosition;

  float m_totalSize;

  CScroller m_scroller;
  int m_lastScrollerValue;

  bool m_useControlPositions;
  ORIENTATION m_orientation;
  uint32_t m_alignment;

  // for autosizing
  float m_minSize;
};

