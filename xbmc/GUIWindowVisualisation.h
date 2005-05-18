#pragma once

#include "GUIWindow.h"
#include "GUIVisualisationControl.h"

class CGUIWindowVisualisation :
      public CGUIWindow
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual void FreeResources();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse();
  virtual void Render();
  virtual void OnWindowLoaded();
protected:
  void SetAlpha(DWORD dwAlpha);
  DWORD m_dwFrameCounter;
  DWORD m_dwInitTimer;
  bool m_bShowInfo;
  bool m_bFadingAtStart;
};
