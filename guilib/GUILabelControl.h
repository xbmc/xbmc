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
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const wstring& strLabel, const CLabelInfo& labelInfo, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void SetAlpha(DWORD dwAlpha);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  const wstring GetLabel() const { return m_strLabel; }
  void SetLabel(const wstring &strLabel);
  void SetText(CStdString aLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};
  void SetInfo(const vector<int> &vecInfo);
  const vector<int> &GetInfo() const { return m_vecInfo; };
  void SetWidthControl(bool bScroll);
  bool GetWidthControl() const { return m_ScrollInsteadOfTruncate; };
  void SetTruncate(bool bTruncate);
  void SetWrapMultiLine(bool wrapMultiLine) { m_wrapMultiLine = wrapMultiLine; };
  bool GetWrapMultiLine() const { return m_wrapMultiLine; };
protected:
  void ShortenPath();
  void WrapText(CStdString &text);

protected:
  CLabelInfo m_label;

  wstring m_strLabel;
  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  DWORD m_dwCounter;
  vector<int> m_vecInfo;
  // stuff for scrolling
  bool m_wrapMultiLine;
  bool m_ScrollInsteadOfTruncate;
  CScrollInfo m_ScrollInfo;
};
#endif
