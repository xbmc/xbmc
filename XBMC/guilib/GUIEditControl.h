/*!
\file GUIEditControl.h
\brief 
*/

#ifndef GUILIB_GUIEditControl_H
#define GUILIB_GUIEditControl_H

#pragma once

#include "GUILabelControl.h"

/*!
 \ingroup controls
 \brief 
 */

class IEditControlObserver
{
public:
  virtual void OnEditTextComplete(CStdString& strLineOfText) = 0;
};

class CGUIEditControl : public CGUILabelControl
{
public:
  CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CLabelInfo& labelInfo, const string& strLabel);

  virtual ~CGUIEditControl(void);

  virtual void SetObserver(IEditControlObserver* aObserver);
  virtual void OnKeyPress(WORD wKeyId);
  virtual void Render();

protected:
  void RecalcLabelPosition();

protected:
  IEditControlObserver* m_pObserver;
  float m_originalPosX;
};
#endif
