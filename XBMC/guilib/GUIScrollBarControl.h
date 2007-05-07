/*!
\file GUIScrollBar.h
\brief 
*/

#ifndef GUILIB_GUISCROLLBAR_H
#define GUILIB_GUISCROLLBAR_H

#pragma once

#include "guiImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIScrollBar :
      public CGUIControl
{
public:
  CGUIScrollBar(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                       float width, float height,
                       const CImage& backGroundTexture,
                       const CImage& barTexture, const CImage& barTextureFocus,
                       const CImage& nibTexture, const CImage& nibTextureFocus,
                       ORIENTATION orientation, bool showOnePage);
  virtual ~CGUIScrollBar(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetColorDiffuse(D3DCOLOR color);
  virtual void SetRange(int pageSize, int numItems);
  virtual bool OnMessage(CGUIMessage& message);
  void SetValue(int value);
  int GetValue() const;
  virtual bool HitTest(const CPoint &point) const;
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseDrag(const CPoint &offset, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual CStdString GetDescription() const;
  virtual bool IsVisible() const;
protected:
  void UpdateBarSize();
  virtual void Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point);

  CGUIImage m_guiBackground;
  CGUIImage m_guiBarNoFocus;
  CGUIImage m_guiBarFocus;
  CGUIImage m_guiNibNoFocus;
  CGUIImage m_guiNibFocus;

  int m_numItems;
  int m_pageSize;
  int m_offset;

  bool m_showOnePage;
  ORIENTATION m_orientation;
};
#endif
