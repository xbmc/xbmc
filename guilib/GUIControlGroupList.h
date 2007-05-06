/*!
\file GUIControlGroupList.h
\brief 
*/

#pragma once

#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief list of controls that is scrollable
 */
class CGUIControlGroupList : public CGUIControlGroup
{
public:
  CGUIControlGroupList(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float itemGap, DWORD pageControl, ORIENTATION orientation, bool useControlPositions);
  virtual ~CGUIControlGroupList(void);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool CanFocusFromPoint(float posX, float posY, CGUIControl **control) const;
  virtual void UnfocusFromPoint(float posX, float posY);

  virtual void AddControl(CGUIControl *control);
  virtual void ClearAll();

protected:
  void ValidateOffset();
  inline float Size(const CGUIControl *control) const;
  inline float Size() const;
  void ScrollTo(float offset);

  float m_itemGap;
  DWORD m_pageControl;

  float m_offset; // measurement in pixels of our origin
  float m_totalSize;

  float m_scrollSpeed;
  float m_scrollOffset;
  DWORD m_scrollTime;

  bool m_useControlPositions;
  ORIENTATION m_orientation;
};

