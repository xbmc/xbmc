/*!
\file GUILabelControl.h
\brief 
*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once

#include "GUIControl.h"

class CInfoPortion
{
public:
  CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix)
  {
    m_info = info;
    m_prefix = prefix;
    m_postfix = postfix;
    // filter our prefix and postfix for comma's and $$
    m_prefix.Replace("$COMMA", ","); m_prefix.Replace("$$", "$");
    m_postfix.Replace("$COMMA", ","); m_postfix.Replace("$$", "$");
  };

  int m_info;
  CStdString m_prefix;
  CStdString m_postfix;
};

/*!
 \ingroup controls
 \brief 
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const string& strLabel, const CLabelInfo& labelInfo, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void SetAlpha(DWORD dwAlpha);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  const string& GetLabel() const { return m_strLabel; }
  void SetLabel(const string &strLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};
  void SetInfo(int singleInfo);
  int GetInfo() const { return m_singleInfo; };
  void SetWidthControl(bool bScroll);
  bool GetWidthControl() const { return m_ScrollInsteadOfTruncate; };
  void SetTruncate(bool bTruncate);
  void SetWrapMultiLine(bool wrapMultiLine) { m_wrapMultiLine = wrapMultiLine; };
  bool GetWrapMultiLine() const { return m_wrapMultiLine; };
  static void WrapText(CStdString &text, CGUIFont *font, float maxWidth);
  static void WrapText(CStdStringW &utf16Text, CGUIFont *font, float maxWidth);

protected:
  CStdString ShortenPath(const CStdString &path);

protected:
  CLabelInfo m_label;

  string m_strLabel;
  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  DWORD m_dwCounter;
  // stuff for scrolling
  bool m_wrapMultiLine;
  bool m_ScrollInsteadOfTruncate;
  CScrollInfo m_ScrollInfo;

  // multi-info stuff
  int                   m_singleInfo;
  vector<CInfoPortion>  m_multiInfo;
};
#endif
