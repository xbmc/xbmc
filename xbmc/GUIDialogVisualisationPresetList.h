#pragma once

#include "GUIDialog.h"
#include "visualizations/Visualisation.h"

class CGUIDialogVisualisationPresetList :
      public CGUIDialog
{
public:
  CGUIDialogVisualisationPresetList(void);
  virtual ~CGUIDialogVisualisationPresetList(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

protected:
  void SetVisualisation(CVisualisation *pVisualisation);
  CVisualisation *m_pVisualisation;
  CFileItemList m_vecPresets;
  int m_currentPreset;
};
