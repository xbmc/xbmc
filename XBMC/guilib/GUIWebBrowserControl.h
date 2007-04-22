/*!
\file GUIWebBrowserControl.h
\brief 
*/

#ifdef WITH_LINKS_BROWSER

#ifndef GUILIB_GUIWebBrowserCONTROL_H
#define GUILIB_GUIWebBrowserCONTROL_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIWebBrowserControl : public CGUIControl
{
public:
  CGUIWebBrowserControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIWebBrowserControl(void);

  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  
  BOOL m_bKeepSession;

protected:
  bool OnMouseOver();
  bool OnMouseWheel();
  bool OnMouseClick(DWORD dwButton);

  int m_realPosX, m_realPosY, m_realHeight, m_realWidth;
};
#endif

#endif /* WITH_LINKS_BROWSER */