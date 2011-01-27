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

#include "guilib/GUIDialog.h"
#include "dialogs/GUIDialogSlider.h"

class CGUISpinControlEx;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUIImage;

typedef CStdString (*FORMATFUNCTION) (float value, float min);

class SettingInfo
{
public:
  enum SETTING_TYPE { NONE=0, BUTTON, CHECK, CHECK_UCHAR, SPIN, SLIDER, SEPARATOR };
  SettingInfo()
  {
    id = 0;
    data = NULL;
    type = NONE;
    enabled = true;
    min = 0;
    max = 0;
    interval = 0;
    formatFunction = NULL;
  };
  SETTING_TYPE type;
  CStdString name;
  unsigned int id;
  void *data;
  float min;
  float max;
  float interval;
  FORMATFUNCTION formatFunction;
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
  virtual bool OnAction(const CAction& action);
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings() {};
  void UpdateSetting(unsigned int setting);
  void EnableSettings(unsigned int setting, bool enabled);
  virtual void OnSettingChanged(SettingInfo &setting) {};
  void FreeControls();
  void OnClick(int iControlID);

  void AddSetting(SettingInfo &setting, float width, int iControlID);

  void AddButton(unsigned int id, int label, float *current = NULL, float min = 0, float interval = 0, float max = 0, FORMATFUNCTION function = NULL);
  void AddBool(unsigned int id, int label, bool *on, bool enabled = true);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max, const char* minLabel = NULL);
  void AddSpin(unsigned int id, int label, int *current, std::vector<std::pair<int, CStdString> > &values);
  void AddSpin(unsigned int id, int label, int *current, std::vector<std::pair<int, int> > &values);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, FORMATFUNCTION formatFunction, bool allowPopup = true);
  void AddSeparator(unsigned int id);

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  std::vector<SettingInfo> m_settings;

  bool m_usePopupSliders;
};
