/*!
\file GUIVideoControl.h
\brief 
*/

#ifndef GUILIB_GUIVIDEOCONTROL_H
#define GUILIB_GUIVIDEOCONTROL_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIVideoControl :
      public CGUIControl
{
public:
  CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIVideoControl(void);
  virtual void Render();
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool CanFocus() const;
  virtual bool CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const;
};
#endif
