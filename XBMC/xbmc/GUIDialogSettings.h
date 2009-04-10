#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialog.h"
#include "GUIEditControl.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "GUISettingsSliderControl.h"

typedef std::vector<CStdString> SETTINGSTRINGS;

class CGUIImage;

class SettingInfo
{
public:
  enum SETTING_TYPE { NONE=0, EDIT, EDIT_NUM, BUTTON, CHECK, CHECK_UCHAR, SPIN, SLIDER, SLIDER_INT, SLIDER_ABS, SEPARATOR };
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

  void AddEdit(unsigned int id, int label, CStdString *str, bool enabled = true);
  void AddNumEdit(unsigned int id, int label, int *current, bool enabled = true);
  void AddButton(unsigned int it, int label, bool bOn=true);
  void AddButton(unsigned int it, int label, CStdString *str, bool bOn=true);
  void AddBool(unsigned int id, int label, bool *on, bool enabled = true);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const SETTINGSTRINGS &entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max, const char* minLabel = NULL);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format = NULL, bool absvalue=false);
  void AddSlider(unsigned int id, int label, int *current, int min, int max);
  void AddSeparator(unsigned int id);

  CGUIEditControl *m_pOriginalEdit;
  CGUIEditControl *m_pOriginalEditNum;
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  std::vector<SettingInfo> m_settings;
};
