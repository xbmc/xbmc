/*!
\file GUIFadeLabelControl.h
\brief 
*/

#ifndef GUILIB_GUIFADELABELCONTROL_H
#define GUILIB_GUIFADELABELCONTROL_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIFadeLabelControl : public CGUIControl
{
public:
  CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, DWORD dwTextColor, DWORD dwTextAlign);
  virtual ~CGUIFadeLabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetAlpha(DWORD dwAlpha);
  DWORD GetTextColor() const { return m_dwTextColor;};
  DWORD GetAlignment() const { return m_dwTextAlign;};
  const char * GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };

  void SetInfo(const vector<int> &vecInfo);
  void SetLabel(const vector<wstring> &vecLabel);
  const vector<int> &GetInfo() const { return m_vecInfo; };
  const vector<wstring> &GetLabel() const { return m_vecLabels; };

protected:
  void RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll );
  CGUIFont* m_pFont;
  vector<wstring> m_vecLabels;
  DWORD m_dwTextColor;
  DWORD m_dwTextAlign;
  int m_iCurrentLabel;
  bool m_bFadeIn;
  int m_iCurrentFrame;
  vector<int> m_vecInfo;
  CScrollInfo m_scrollInfo;
};
#endif
