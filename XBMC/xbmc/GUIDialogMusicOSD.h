#pragma once

#include "GUIDialog.h"
#include "visualizations/Visualisation.h"

class CGUIDialogMusicOSD :
      public CGUIDialog
{
public:
  CGUIDialogMusicOSD(void);
  virtual ~CGUIDialogMusicOSD(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
protected:
  virtual void OnInitWindow();
  CVisualisation *m_pVisualisation;
};
