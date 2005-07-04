/*!
\file GUIButtonControl.h
\brief 
*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

#pragma once

#include "GUIImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId,
                    int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                    const CStdString& strTextureFocus, const CStdString& strTextureNoFocus,
                    DWORD dwTextXOffset, DWORD dwTextYOffset, DWORD dwAlign = XBFONT_LEFT);

  virtual ~CGUIButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnMouseClick(DWORD dwButton);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual void SetDisabledColor(D3DCOLOR color);
  void SetLabel(const CStdString& strFontName, const wstring& strLabel, D3DCOLOR dwColor);
  void SetLabel(const CStdString& strFontName, const CStdString& strLabel, D3DCOLOR dwColor);
  void SetText(const CStdString &aLabel);
  void SetText(const wstring & aLabel);
  void SetText2(const CStdString &aLabel2);
  void SetText2(const wstring & aLabel2);
  void SetHyperLink(long dwWindowID);
  void SetExecuteAction(const CStdString& strExecuteAction);
  const CStdString& GetTextureFocusName() const { return m_imgFocus.GetFileName(); };
  const CStdString& GetTextureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
  DWORD GetTextOffsetX() const { return m_dwTextOffsetX;};
  DWORD GetTextOffsetY() const { return m_dwTextOffsetY;};
  void SetTextColor(D3DCOLOR dwTextColor) { m_dwTextColor = dwTextColor;};
  DWORD GetTextColor() const { return m_dwTextColor;};
  DWORD GetTextAlign() const { return m_dwTextAlignment;};
  void SetTextAlign(DWORD dwTextAlign) { m_dwTextAlignment = dwTextAlign;};
  DWORD GetDisabledColor() const { return m_dwDisabledColor;};
  const char * GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
  const wstring GetLabel() const { return m_strLabel; };
  DWORD GetHyperLink() const { return m_lHyperLinkWindowID;};
  const CStdString& GetExecuteAction() const { return m_strExecuteAction; };
  void SetTabButton(bool bIsTabButton = TRUE) { m_bTabButton = bIsTabButton; };
  void Flicker(bool bFlicker = TRUE);
  virtual void Update();

protected:

  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;
  DWORD m_dwFocusCounter;
  DWORD m_dwFlickerCounter;
  DWORD m_dwFrameCounter;
  DWORD m_dwTextOffsetX;
  DWORD m_dwTextOffsetY;
  DWORD m_dwTextAlignment;
  wstring m_strLabel;
  wstring m_strLabel2;
  CGUIFont* m_pFont;
  D3DCOLOR m_dwTextColor;
  D3DCOLOR m_dwDisabledColor;
  long m_lHyperLinkWindowID;
  CStdString m_strExecuteAction;
  bool m_bTabButton;
  bool m_bPulsing;
};
#endif
