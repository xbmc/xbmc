/*!
\file GUITextBox.h
\brief 
*/

#ifndef GUILIB_GUITEXTBOX_H
#define GUILIB_GUITEXTBOX_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListItem.h"


/*!
 \ingroup controls
 \brief 
 */
class CGUITextBox : public CGUIControl
{
public:
  CGUITextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
              float spinWidth, float spinHeight,
              const CImage& textureUp, const CImage& textureDown,
              const CImage& textureUpFocus, const CImage& textureDownFocus,
              const CLabelInfo& spinInfo, float spinX, float spinY,
              const CLabelInfo &labelInfo);
  virtual ~CGUITextBox(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnRight();
  virtual void OnLeft();
  virtual bool OnMessage(CGUIMessage& message);

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPulseOnSelect(bool pulse);
  virtual void SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right);
  void SetPageControl(DWORD pageControl);
  void SetText(const string &strText);
  virtual bool HitTest(float posX, float posY) const;
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseWheel();

protected:
  void OnPageUp();
  void OnPageDown();
  void UpdatePageControl();

  float m_spinPosX;
  float m_spinPosY;
  int m_iOffset;
  int m_iItemsPerPage;
  float m_itemHeight;
  int m_iMaxPages;

  CStdString m_strText;
  CLabelInfo m_label;
  CGUISpinControl m_upDown;
  vector<CGUIListItem> m_vecItems;
  typedef vector<CGUIListItem> ::iterator ivecItems;

  bool m_wrapText;  // whether we need to wordwrap or not
  DWORD m_pageControl;
};
#endif
