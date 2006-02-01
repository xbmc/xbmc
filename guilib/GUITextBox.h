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
  CGUITextBox(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
              DWORD dwSpinWidth, DWORD dwSpinHeight,
              const CStdString& strUp, const CStdString& strDown,
              const CStdString& strUpFocus, const CStdString& strDownFocus,
              DWORD dwSpinColor, int iSpinX, int iSpinY,
              const CLabelInfo &labelInfo);
  virtual ~CGUITextBox(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  virtual bool OnMessage(CGUIMessage& message);

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  virtual void SetPulseOnSelect(bool pulse);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetLabelInfo().textColor;};
  int GetSpinX() const { return m_iSpinPosX;};
  int GetSpinY() const { return m_iSpinPosY;};
  void SetText(const wstring &strText);
  virtual bool HitTest(int iPosX, int iPosY) const;
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseWheel();

protected:
  void OnPageUp();
  void OnPageDown();

  int m_iSpinPosX;
  int m_iSpinPosY;
  int m_iOffset;
  int m_iItemsPerPage;
  int m_iItemHeight;
  int m_iMaxPages;

  CLabelInfo m_label;
  CGUISpinControl m_upDown;
  vector<CGUIListItem> m_vecItems;
  typedef vector<CGUIListItem> ::iterator ivecItems;
};
#endif
