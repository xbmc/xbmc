#pragma once

#include "GUIDialog.h"
#include "visualizations\Visualisation.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"

class CGUIDialogMusicOSD :
      public CGUIDialog
{
public:
  CGUIDialogMusicOSD(void);
  virtual ~CGUIDialogMusicOSD(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
protected:
  int m_iLastControl;

  CVisualisation *m_pVisualisation;
};
