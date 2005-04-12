/*!
\file GUILabelControl.h
\brief 
*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, const wstring& strLabel, DWORD dwTextColor, DWORD dwDisabledColor, DWORD dwTextAlign, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void SetAlpha(DWORD dwAlpha);
  DWORD GetTextColor() const { return m_dwTextColor;}
  DWORD GetDisabledColor() const { return m_dwDisabledColor;}
  const char* GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; }
  const wstring GetLabel() const { return m_strLabel; }
  void SetLabel(const wstring &strLabel);
  void SetText(CStdString aLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};

  DWORD m_dwTextAlign;

  void SetInfo(int info) { m_Info = info; };
  int GetInfo() const { return m_Info; };

protected:
  void ShortenPath();
protected:
  CGUIFont* m_pFont;
  wstring m_strLabel;
  DWORD m_dwTextColor;
  bool m_bHasPath;
  DWORD m_dwDisabledColor;
  bool m_bShowCursor;
  int m_iCursorPos;
  DWORD m_dwCounter;
  int m_Info;
  wstring m_strBackupLabel;
  // stuff for scrolling
  int m_PixelScroll;
  unsigned int m_CharacterScroll;
  int m_ScrollWait;
};
#endif
