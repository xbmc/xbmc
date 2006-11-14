#pragma once

#include "GUIDialog.h"
#ifdef HAS_VISUALISATION
#include "visualizations\Visualisation.h"
#endif

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
#ifdef HAS_VISUALISATION
  CVisualisation *m_pVisualisation;
#endif
};
