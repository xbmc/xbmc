#pragma once

#include "GUIDialog.h"
#ifdef HAS_VISUALISATION
#include "visualizations\Visualisation.h"
#endif
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
#ifdef HAS_VISUALISATION
  void SetVisualisation(CVisualisation *pVisualisation);
#endif
  virtual void SetupPage();
  void UpdateSettings();
  virtual void FreeControls();
  void OnClick(int iControl);
#ifdef HAS_VISUALISATION
  void AddSetting(VisSetting &setting, float posX, float posY, float width, int iControlID);
#endif
  int m_iCurrentPage;
  int m_iNumPages;
  int m_iNumPerPage;

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;

#ifdef HAS_VISUALISATION
  CVisualisation *m_pVisualisation;
  vector<VisSetting> *m_pSettings;
#endif
};
