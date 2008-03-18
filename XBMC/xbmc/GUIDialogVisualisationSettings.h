#pragma once

#include "GUIDialog.h"
#include "visualizations/Visualisation.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"

class CGUIDialogVisualisationSettings :
      public CGUIDialog
{
public:
  CGUIDialogVisualisationSettings(void);
  virtual ~CGUIDialogVisualisationSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
protected:
  virtual void OnInitWindow();
  void SetVisualisation(CVisualisation *pVisualisation);
  virtual void SetupPage();
  void UpdateSettings();
  virtual void FreeControls();
  void OnClick(int iControl);
  void AddSetting(VisSetting &setting, float width, int iControlID);

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;

  CVisualisation *m_pVisualisation;
  std::vector<VisSetting> *m_pSettings;
};
