#pragma once

#include "GUIDialog.h"
#ifdef HAS_VISUALISATION
#include "visualizations\Visualisation.h"
#endif

class CGUIDialogVisualisationPresetList :
      public CGUIDialog
{
public:
  CGUIDialogVisualisationPresetList(void);
  virtual ~CGUIDialogVisualisationPresetList(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

protected:
#ifdef HAS_VISUALISATION
  void SetVisualisation(CVisualisation *pVisualisation);
  CVisualisation *m_pVisualisation;
#endif
  CFileItemList m_vecPresets;
  int m_currentPreset;
};
