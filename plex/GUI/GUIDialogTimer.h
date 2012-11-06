/*
 *  GUIDialogTimer.h
 *  Plex
 *
 *  Created by James Clarke on 18/10/2009.
 *  Copyright 2009 Plex Incorporated. All rights reserved.
 *
 */

#pragma once
#include "guilib/GUIDialog.h"
#include "Stopwatch.h"
#include "XBDateTime.h"

class CGUIDialogTimer: public CGUIDialog
{
private:
  int m_iTime;
  CStdString m_typedString;
  CDateTime m_lastDateTime;
  bool m_bConfirmed, m_bShowFooter, m_bShowFooterLabel;
  CStopWatch m_typingStopWatch;

public:
  CGUIDialogTimer(void);
  virtual ~CGUIDialogTimer(void);
  
  static int ShowAndGetInput(int iHeadLabel, int iInfoLabel, int iTime);

  virtual bool OnAction(const CAction &action);
  virtual void DoModal(int iHeadLabel, int iInfoLabel, int iTime);
  virtual void Render();
  virtual int GetTime() { return m_iTime; }
  
private:
  virtual void SendGUIMessage(CGUIMessage msg);
  virtual void SetControlLabel(int controlId, CStdString str);
  virtual void SetControlLabel(int controlId, int str);
  virtual void ShowControl(int controlId);
  virtual void HideControl(int controlId);
  virtual void ConvertTypedTextToTime();
  virtual void UpdateLabels();

};
