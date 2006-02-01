#pragma once

#include "GUIDialog.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "GUISettingsSliderControl.h"

class SettingInfo
{
public:
  enum SETTING_TYPE { NONE=0, BUTTON, CHECK, SPIN, SLIDER, SLIDER_INT, SEPARATOR };
  SettingInfo()
  {
    id = 0;
    data = NULL;
    type = NONE;
  };
  SETTING_TYPE type;
  CStdString name;
  unsigned int id;
  void *data;
  float min;
  float max;
  float interval;
  CStdString format;
  std::vector<CStdString> entry;
};

class CGUIDialogSettings :
      public CGUIDialog
{
public:
  CGUIDialogSettings(DWORD id, const char *xmlFile);
  virtual ~CGUIDialogSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
protected:
  virtual void SetupPage();
  virtual void CreateSettings() {};
  void UpdateSetting(unsigned int setting);
  virtual void OnSettingChanged(unsigned int setting) {};
  void FreeControls();
  void OnClick(int iControlID);

  void AddSetting(SettingInfo &setting, int iPosX, int iPosY, int iWidth, int iControlID);

  void AddButton(unsigned int it, int label);
  void AddBool(unsigned int id, int label, bool *on);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format = NULL);
  void AddSlider(unsigned int id, int label, int *current, int min, int max);
  void AddSeparator(unsigned int id);

  int m_iLastControl;
  int m_iPageOffset;  // offset into the settings list of our current page.
  int m_iCurrentPage;
  int m_iNumPages;
  int m_iNumPerPage;

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  vector<SettingInfo> m_settings;
};
