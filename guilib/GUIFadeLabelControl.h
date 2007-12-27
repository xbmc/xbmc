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
  CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, DWORD timeToDelayAtEnd);
  virtual ~CGUIFadeLabelControl(void);
  virtual void DoRender(DWORD currentTime);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

  void SetInfo(const vector<int> &vecInfo);
  void SetLabel(const vector<string> &vecLabel);

protected:
  void AddLabel(const string &label);

  vector< vector<CInfoPortion> > m_infoLabels;
  unsigned int m_currentLabel;
  unsigned int m_lastLabel;

  CLabelInfo m_label;

  bool m_scrollOut;

  CScrollInfo m_scrollInfo;
  CGUITextLayout m_textLayout;
  CAnimation *m_fadeAnim;
  DWORD m_renderTime;
};
#endif
