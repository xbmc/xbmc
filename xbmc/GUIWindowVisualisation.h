#pragma once

#include "GUIWindow.h"

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
  void FadeControls(DWORD controlID, bool fadeIn, DWORD length);
  DWORD m_dwInitTimer;
  DWORD m_dwLockedTimer;
  bool m_bShowInfo;
  bool m_bShowPreset;
  CMusicInfoTag m_tag;    // current tag info, for finding when the info manager updates
};
