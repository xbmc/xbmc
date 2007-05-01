/*!
\file GUISpinControl.h
\brief 
*/

#ifndef GUILIB_SPINCONTROL_H
#define GUILIB_SPINCONTROL_H

#pragma once

#include "guiImage.h"

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3
#define SPIN_CONTROL_TYPE_PAGE   4

class CRect
{
public:
  CRect() { top = left = width = height = 0;};
  ~CRect() {};

  void SetRect(float l, float t, float w, float h)
  {
    left = l;
    top = t;
    width = w;
    height = h;
  };

  bool PtInRect(float x, float y) const
  {
    if (left <= x && x <= left + width && top <= y && y <= top + height)
      return true;
    return false;
  };

private:
  float top;
  float left;
  float width;
  float height;
};
/*!
 \ingroup controls
 \brief 
 */
class CGUISpinControl : public CGUIControl
{
public:
  CGUISpinControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureUp, const CImage& textureDown, const CImage& textureUpFocus, const CImage& textureDownFocus, const CLabelInfo& labelInfo, int iType);
  virtual ~CGUISpinControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool HitTest(float posX, float posY) const;
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(D3DCOLOR color);
  virtual float GetWidth() const;
  void SetRange(int iStart, int iEnd);
  void SetFloatRange(float fStart, float fEnd);
  void SetValue(int iValue);
  void SetFloatValue(float fValue);
  int GetValue() const;
  float GetFloatValue() const;
  void AddLabel(const string& strLabel, int iValue);
  const string GetLabel() const;
  void SetReverse(bool bOnOff);
  int GetMaximum() const;
  int GetMinimum() const;
  void SetSpinAlign(DWORD align, float offsetX) { m_label.align = align; m_label.offsetX = offsetX; };
  void SetType(int iType) { m_iType = iType; };
  float GetSpinWidth() const { return m_imgspinUp.GetWidth(); };
  float GetSpinHeight() const { return m_imgspinUp.GetHeight(); };
  void SetFloatInterval(float fInterval);
  void SetShowRange(bool bOnoff) ;
  void SetBuddyControlID(DWORD dwBuddyControlID);
  void SetNonProportional(bool bOnOff);
  void SetShowOnePage(bool showOnePage) { m_showOnePage = showOnePage; };
  void Clear();
  virtual CStdString GetDescription() const;
  bool IsFocusedOnUp() const;

  virtual bool IsVisible() const;

protected:
  void PageUp();
  void PageDown();
  bool CanMoveDown(bool bTestReverse = true);
  bool CanMoveUp(bool bTestReverse = true);
  void MoveUp(bool bTestReverse = true);
  void MoveDown(bool bTestReverse = true);
  void SendPageChange();
  int m_iStart;
  int m_iEnd;
  float m_fStart;
  float m_fEnd;
  int m_iValue;
  float m_fValue;
  int m_iType;
  int m_iSelect;
  bool m_bReverse;
  float m_fInterval;
  vector<string> m_vecLabels;
  vector<int> m_vecValues;
  CGUIImage m_imgspinUp;
  CGUIImage m_imgspinDown;
  CGUIImage m_imgspinUpFocus;
  CGUIImage m_imgspinDownFocus;
  CLabelInfo m_label;
  bool m_bShowRange;
  char m_szTyped[10];
  int m_iTypedPos;
  float m_maxTextWidth;
  CRect m_rectHit;   // rect for hit test on the Text

  int m_itemsPerPage;
  int m_numItems;
  bool m_showOnePage;
};
#endif
