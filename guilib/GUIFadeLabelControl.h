/*!
\file GUIFadeLabelControl.h
\brief 
*/

#ifndef GUILIB_GUIFADELABELCONTROL_H
#define GUILIB_GUIFADELABELCONTROL_H

#pragma once

#include "GUIControl.h"

#include "GUILabelControl.h"  // for CInfoPortion

/*!
 \ingroup controls
 \brief 
 */
class CGUIFadeLabelControl : public CGUIControl
{
public:
  CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo);
  virtual ~CGUIFadeLabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  void SetInfo(const vector<int> &vecInfo);
  void SetLabel(const vector<string> &vecLabel);

protected:
  void AddLabel(const string &label);
  void RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll );

  vector<string> m_stringLabels;
  vector< vector<CInfoPortion> > m_infoLabels;

  CLabelInfo m_label;
  int m_iCurrentLabel;
  bool m_bFadeIn;
  int m_iCurrentFrame;
  vector<int> m_vecInfo;
  CScrollInfo m_scrollInfo;
};
#endif
