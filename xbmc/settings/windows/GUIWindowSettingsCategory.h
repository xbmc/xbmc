#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <vector>

#include "GUIControlSettings.h"
#include "guilib/GUIWindow.h"
#include "settings/lib/SettingDependency.h"
#include "settings/lib/SettingSection.h"
#include "settings/Settings.h"
#include "settings/lib/SettingsManager.h"
#include "threads/Timer.h"

typedef boost::shared_ptr<CGUIControlBaseSetting> BaseSettingControlPtr;

class CGUIWindowSettingsCategory
  : public CGUIWindow,
    protected ITimerCallback,
    protected ISettingCallback
{
public:
  CGUIWindowSettingsCategory(void);
  virtual ~CGUIWindowSettingsCategory(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual int GetID() const { return CGUIWindow::GetID() + m_iSection; };

protected:
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  
  virtual void SetupControls(bool createSettings = true);
  virtual void FreeControls();
  void FreeSettingsControls();

  virtual void OnTimeout();
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingPropertyChanged(const CSetting *setting, const char *propertyName);
  
  void CreateSettings();
  void UpdateSettings();
  void SetDescription(const CVariant &label);
  CGUIControl* AddSetting(CSetting *pSetting, float width, int &iControlID);
  CGUIControl* AddSeparator(float width, int &iControlID);
  CGUIControl* AddSettingControl(CGUIControl *pControl, BaseSettingControlPtr pSettingControl, float width, int &iControlID);
  
  /*!
    \brief A setting control has been interacted with by the user

    This method is called when the user manually interacts (clicks,
    edits) with a setting control. It contains handling for both
    delayed and undelayed settings and either starts the delay timer
    or triggers the setting change which, on success, results in a
    callback to OnSettingChanged().

    \param pSettingControl Setting control that has been interacted with
   */
  virtual void OnClick(BaseSettingControlPtr pSettingControl);

  CSettingSection* GetSection(int windowID) const;
  BaseSettingControlPtr GetSettingControl(const std::string &setting);
  BaseSettingControlPtr GetSettingControl(int controlId);
  
  CSettings& m_settings;
  SettingCategoryList m_categories;
  std::vector<BaseSettingControlPtr> m_settingControls;

  int m_iSetting;
  int m_iCategory;
  int m_iSection;
  CSettingAction *m_resetSetting;
  CSettingCategory *m_dummyCategory;
  
  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalCategoryButton;
  CGUIButtonControl *m_pOriginalButton;
  CGUIEditControl *m_pOriginalEdit;
  CGUIImage *m_pOriginalImage;
  bool newOriginalEdit;
  
  BaseSettingControlPtr m_delayedSetting; ///< Current delayed setting \sa CBaseSettingControl::SetDelayed()
  CTimer m_delayedTimer;                  ///< Delayed setting timer

  bool m_returningFromSkinLoad; // true if we are returning from loading the skin
};
