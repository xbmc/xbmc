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

struct SGUIControlAndOffset
{
  CGUIControl* control{nullptr};
  float offset{0.f};
};

/*!
 \ingroup controls
 \brief list of controls that is scrollable
 */
class CGUIControlGroupList : public CGUIControlGroup
{
public:
  CGUIControlGroupList(int parentID, int controlID, float posX, float posY, float width, float height, float itemGap, int pageControl, ORIENTATION orientation, bool useControlPositions, uint32_t alignment, const CScroller& scroller);
  ~CGUIControlGroupList(void) override;
  CGUIControlGroupList* Clone() const override { return new CGUIControlGroupList(*this); }

  float GetWidth() const override;
  float GetHeight() const override;
  virtual float Size() const;
  void SetInvalid() override;

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  EVENT_RESULT SendMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
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
  // shut up warning about hiding an overloaded virtual function
  using CGUIControlGroup::GetFirstFocusableControl;

protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool IsControlOnScreen(float pos, const CGUIControl* control) const;
  void ValidateOffset();
  void CalculateItemGap();
  inline float Size(const CGUIControl *control) const;
  void ScrollTo(float offset);
  float GetAlignOffset() const;

  int GetNumItems() const;
  int GetSelectedItem() const;
  /*!
   * \brief Returns the position of the specified child control
   * \param control The control to be located.
   * \return Position of the control.
   *         -1 for a control that is not visible or cannot be found.
   */
  float GetControlOffset(const CGUIControl* control) const;
  /*!
   * \brief Locate a visible and focusable child control at/close to the specified position
   * \param target Ideal position of the control
   * \param direction >0: Favor the control at or after the target for direction, otherwise
   *        the control before or at target.
   * \return Best match control & position.
   *         The control may be null and the offset 0 if there aren't any visible focusable controls in the list.
   */
  SGUIControlAndOffset GetFocusableControlAt(float target, int direction) const;
  /*!
   * \brief Scroll the list of controls, update the selected control and the pager.
   * \param pages direction and amount of scrolling.
   *        For example 1 to scroll down/right one page, -0.5 to scroll up/left half a page
   */
  void ScrollPages(float pages);
  /*!
   * \brief Change the selected control and scroll to the specified position.
   * \param control New selected control
   * \param offset Offset of the top of the visible view
   */
  void MoveTo(CGUIControl* control, float offset);
  /*!
   * Return the first visible and focusable child control.
   */
  CGUIControl* GetFirstFocusableControl() const;
  /*!
   * Return the last visible and focusable child control.
   */
  CGUIControl* GetLastFocusableControl() const;

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

