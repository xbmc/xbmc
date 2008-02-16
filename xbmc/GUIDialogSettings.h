#pragma once

#include "GUIDialog.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "GUISettingsSliderControl.h"

class SettingInfo
{
public:
  enum SETTING_TYPE { NONE=0, BUTTON, CHECK, CHECK_UCHAR, SPIN, SLIDER, SLIDER_INT, SEPARATOR };
  SettingInfo()
  {
    id = 0;
    data = NULL;
    type = NONE;
    enabled = true;
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
  bool enabled;
};

class CGUIDialogSettings :
      public CGUIDialog
{
public:
  CGUIDialogSettings(DWORD id, const char *xmlFile);
  virtual ~CGUIDialogSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
protected:
  virtual void OnOkay() {};
  virtual void OnCancel() {};
  virtual bool OnAction(const CAction& action);
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings() {};
  void UpdateSetting(unsigned int setting);
  void EnableSettings(unsigned int setting, bool enabled);
  virtual void OnSettingChanged(unsigned int setting) {};
  void FreeControls();
  void OnClick(int iControlID);

  void AddSetting(SettingInfo &setting, float width, int iControlID);

  void AddButton(unsigned int it, int label, bool bOn=true);
  void AddBool(unsigned int id, int label, bool *on, bool enabled = true);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max, const char* minLabel = NULL);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format = NULL);
  void AddSlider(unsigned int id, int label, int *current, int min, int max);
  void AddSeparator(unsigned int id);

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  std::vector<SettingInfo> m_settings;
};
