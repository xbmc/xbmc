#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#include "dialogs/GUIDialogSlider.h"

class CGUISpinControlEx;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUIEditControl;
class CGUIImage;
class CGUIEditControl;

typedef std::vector<CStdString> SETTINGSTRINGS;
typedef CStdString (*FORMATFUNCTION) (float value, float min);
typedef CStdString (*RANGEFORMATFUNCTION) (float valueLower, float valueUpper, float min);

class SettingInfo
{
public:
  enum SETTING_TYPE { NONE=0, EDIT, EDIT_NUM, BUTTON, BUTTON_DIALOG, CHECK, CHECK_UCHAR, SPIN, SLIDER, SEPARATOR, STRING, RANGE };
  SettingInfo()
  {
    id = 0;
    data = NULL;
    type = NONE;
    enabled = true;
    min = 0;
    max = 0;
    interval = 0;
    formatFunction.standard = NULL;
  };
  SETTING_TYPE type;
  CStdString name;
  unsigned int id;
  void *data;
  float min;
  float max;
  float interval;
  union
  {
    FORMATFUNCTION standard;
    RANGEFORMATFUNCTION range;
  } formatFunction;
  std::vector<std::pair<int, CStdString> > entry;
  bool enabled;
};

class CGUIDialogSettings :
      public CGUIDialog, public ISliderCallback
{
public:
  CGUIDialogSettings(int id, const char *xmlFile);
  virtual ~CGUIDialogSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  virtual void OnSliderChange(void *data, CGUISliderControl *slider);
protected:
  virtual void OnOkay() {};
  virtual void OnCancel() {};
  virtual bool OnBack(int actionID);
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings() {};
  void UpdateSetting(unsigned int setting);
  void EnableSettings(unsigned int setting, bool enabled);
  virtual void OnSettingChanged(SettingInfo &setting) {};
  void FreeControls();
  void OnClick(int iControlID);

  void AddSetting(SettingInfo &setting, float width, int iControlID);

  void AddEdit(unsigned int id, int label, CStdString *str, bool enabled = true);
  void AddNumEdit(unsigned int id, int label, int *current, bool enabled = true);
  void AddButton(unsigned int id, int label, float *current = NULL, float min = 0, float interval = 0, float max = 0, FORMATFUNCTION function = NULL);
  void AddButton(unsigned int it, int label, CStdString *str, bool bOn=true);
  void AddBool(unsigned int id, int label, bool *on, bool enabled = true);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const SETTINGSTRINGS &entries);
  void AddString(unsigned int id, int label, CStdString *current);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max, const char* minLabel = NULL);
  void AddSpin(unsigned int id, int label, int *current, std::vector<std::pair<int, CStdString> > &values);
  void AddSpin(unsigned int id, int label, int *current, std::vector<std::pair<int, int> > &values);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, FORMATFUNCTION formatFunction, bool allowPopup = true);
  void AddRangeSlider(unsigned int id, int label, float *currentLower, float* currentUpper, float min, float interval, float max, RANGEFORMATFUNCTION formatFunction);
  void AddSeparator(unsigned int id);

  CGUIEditControl *m_pOriginalEdit;
  CGUIEditControl *m_pOriginalEditNum;
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  std::vector<SettingInfo> m_settings;

  bool m_usePopupSliders;
};
