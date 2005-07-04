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
              const CStdString& strFontName,
              DWORD dwSpinWidth, DWORD dwSpinHeight,
              const CStdString& strUp, const CStdString& strDown,
              const CStdString& strUpFocus, const CStdString& strDownFocus,
              DWORD dwSpinColor, DWORD dwSpinX, DWORD dwSpinY,
              const CStdString& strFont, DWORD dwTextColor);
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
  DWORD GetTextColor() const { return m_dwTextColor;};
  const char* GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
  DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetTextColor();};
  int GetSpinX() const { return m_upDown.GetXPosition();};
  int GetSpinY() const { return m_upDown.GetYPosition();};
  void SetText(const wstring &strText);
  virtual bool HitTest(int iPosX, int iPosY) const;
  virtual void OnMouseOver();
  virtual void OnMouseClick(DWORD dwButton);
  virtual void OnMouseWheel();

protected:
  void OnPageUp();
  void OnPageDown();

  int m_iOffset;
  int m_iItemsPerPage;
  int m_iItemHeight;
  int m_iMaxPages;
  DWORD m_dwTextColor;
  CGUIFont* m_pFont;
  CGUISpinControl m_upDown;
  vector<CGUIListItem> m_vecItems;
  typedef vector<CGUIListItem> ::iterator ivecItems;
};
#endif
