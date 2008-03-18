/*!
\file GUILabelControl.h
\brief 
*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once

#include "GUIControl.h"
#include "GUITextLayout.h"

class CGUIListItem;

class CGUIInfoLabel
{
public:
  CGUIInfoLabel();
  CGUIInfoLabel(const CStdString &label, const CStdString &fallback = "");

  void SetLabel(const CStdString &label, const CStdString &fallback);
  CStdString GetLabel(DWORD contextWindow, bool preferImage = false) const;
  CStdString GetItemLabel(const CGUIListItem *item, bool preferImage = false) const;
  bool IsConstant() const;
  bool IsEmpty() const;

private:
  void Parse(const CStdString &label);

  class CInfoPortion
  {
  public:
    CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix);
    int m_info;
    CStdString m_prefix;
    CStdString m_postfix;
  };

  CStdString m_fallback;
  std::vector<CInfoPortion> m_info;
};

/*!
 \ingroup controls
 \brief 
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  const CLabelInfo& GetLabelInfo() const { return m_label; };
  void SetLabel(const std::string &strLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};
  void SetInfo(const CGUIInfoLabel&labelInfo);
  void SetWidthControl(bool bScroll);
  void SetTruncate(bool bTruncate);
  void SetAlignment(DWORD align);
  void SetHighlight(unsigned int start, unsigned int end);

protected:
  CStdString ShortenPath(const CStdString &path);

protected:
  CLabelInfo m_label;
  CGUITextLayout m_textLayout;

  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  DWORD m_dwCounter;
  // stuff for scrolling
  bool m_ScrollInsteadOfTruncate;
  CScrollInfo m_ScrollInfo;

  // multi-info stuff
  CGUIInfoLabel m_infoLabel;

  unsigned int m_startHighlight;
  unsigned int m_endHighlight;
};
#endif
