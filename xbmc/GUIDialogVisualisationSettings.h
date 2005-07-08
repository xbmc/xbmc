#pragma once

#include "GUIDialog.h"
#include "visualizations\Visualisation.h"
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
  void SetVisualisation(CVisualisation *pVisualisation);
  virtual void SetupPage();
  void UpdateSettings();
  virtual void FreeControls();
  void OnClick(int iControl);
  void AddSetting(VisSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID);
  int m_iLastControl;
  int m_iCurrentPage;
  int m_iNumPages;
  int m_iNumPerPage;

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;

  CVisualisation *m_pVisualisation;
  vector<VisSetting> *m_pSettings;
};
