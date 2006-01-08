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
  CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CLabelInfo& labelInfo);
  virtual ~CGUIFadeLabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetAlpha(DWORD dwAlpha);
  const CLabelInfo& GetLabelInfo() const { return m_label; };

  void SetInfo(const vector<int> &vecInfo);
  void SetLabel(const vector<wstring> &vecLabel);
  const vector<int> &GetInfo() const { return m_vecInfo; };
  const vector<wstring> &GetLabel() const { return m_vecLabels; };

protected:
  void RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll );
  vector<wstring> m_vecLabels;

  CLabelInfo m_label;
  int m_iCurrentLabel;
  bool m_bFadeIn;
  int m_iCurrentFrame;
  vector<int> m_vecInfo;
  CScrollInfo m_scrollInfo;
};
#endif
