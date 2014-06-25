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

#include "guilib/ISliderCallback.h"
#include <stdlib.h>
#include <string>

class CGUIControl;
class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;

class CSetting;
class CSettingControlSlider;
class CSettingString;
class CSettingPath;

class CFileItemList;
class CVariant;

class CGUIControlBaseSetting
{
public:
  CGUIControlBaseSetting(int id, CSetting *pSetting);
  virtual ~CGUIControlBaseSetting() {}
  
  int GetID() const { return m_id; }
  CSetting* GetSetting() { return m_pSetting; }

  /*!
   \brief Specifies that this setting should update after a delay
   Useful for settings that have options to navigate through
   and may take a while, or require additional input to update
   once the final setting is chosen.  Settings default to updating
   instantly.
   \sa IsDelayed()
   */
  void SetDelayed() { m_delayed = true; }

  /*!
   \brief Returns whether this setting should have delayed update
   \return true if the setting's update should be delayed
   \sa SetDelayed()
   */
  bool IsDelayed() const { return m_delayed; }

  /*!
   \brief Returns whether this setting is enabled or disabled
   This state is independent of the real enabled state of a
   setting control but represents the enabled state of the
   setting itself based on specific conditions.
   \return true if the setting is enabled otherwise false
   \sa SetEnabled()
   */
  bool IsEnabled() const;

  /*!
   \brief Returns whether the setting's value is valid or not
   */
  bool IsValid() const { return m_valid; }

  void SetValid(bool valid) { m_valid = valid; }

  virtual CGUIControl* GetControl() { return NULL; }
  virtual bool OnClick() { return false; }
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() = 0;  ///< Clears the attached control
protected:
  int m_id;
  CSetting* m_pSetting;
  bool m_delayed;
  bool m_valid;
};

class CGUIControlRadioButtonSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlRadioButtonSetting(CGUIRadioButtonControl* pRadioButton, int id, CSetting *pSetting);
  virtual ~CGUIControlRadioButtonSetting();

  void Select(bool bSelect);

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pRadioButton; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pRadioButton = NULL; }
private:
  CGUIRadioButtonControl *m_pRadioButton;
};

class CGUIControlSpinExSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSpinExSetting(CGUISpinControlEx* pSpin, int id, CSetting *pSetting);
  virtual ~CGUIControlSpinExSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pSpin; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pSpin = NULL; }
private:
  void FillControl();
  void FillIntegerSettingControl();
  CGUISpinControlEx *m_pSpin;
};

class CGUIControlListSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlListSetting(CGUIButtonControl* pButton, int id, CSetting *pSetting);
  virtual ~CGUIControlListSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pButton; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pButton = NULL; }
private:
  static bool GetItems(const CSetting *setting, CFileItemList &items);
  static bool GetIntegerItems(const CSetting *setting, CFileItemList &items);
  static bool GetStringItems(const CSetting *setting, CFileItemList &items);

  CGUIButtonControl *m_pButton;
};

class CGUIControlButtonSetting : public CGUIControlBaseSetting, protected ISliderCallback
{
public:
  CGUIControlButtonSetting(CGUIButtonControl* pButton, int id, CSetting *pSetting);
  virtual ~CGUIControlButtonSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pButton; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pButton = NULL; }

  static bool GetPath(CSettingPath *pathSetting);
protected:
  // implementations of ISliderCallback
  virtual void OnSliderChange(void *data, CGUISliderControl *slider);

private:
  CGUIButtonControl *m_pButton;
};

class CGUIControlEditSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlEditSetting(CGUIEditControl* pButton, int id, CSetting *pSetting);
  virtual ~CGUIControlEditSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pEdit; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pEdit = NULL; }
private:
  static bool InputValidation(const std::string &input, void *data);

  CGUIEditControl *m_pEdit;
};

class CGUIControlSliderSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSliderSetting(CGUISettingsSliderControl* pSlider, int id, CSetting *pSetting);
  virtual ~CGUIControlSliderSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pSlider; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pSlider = NULL; }

  static std::string GetText(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);

private:
  CGUISettingsSliderControl *m_pSlider;
};

class CGUIControlRangeSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlRangeSetting(CGUISettingsSliderControl* pSlider, int id, CSetting *pSetting);
  virtual ~CGUIControlRangeSetting();
  
  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pSlider; }
  virtual bool OnClick();
  virtual void Update(bool updateDisplayOnly = false);
  virtual void Clear() { m_pSlider = NULL; }

private:
  CGUISettingsSliderControl *m_pSlider;
};

class CGUIControlSeparatorSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSeparatorSetting(CGUIImage* pImage, int id);
  virtual ~CGUIControlSeparatorSetting();

  virtual CGUIControl* GetControl() { return (CGUIControl*)m_pImage; }
  virtual bool OnClick() { return false; }
  virtual void Update() {}
  virtual void Clear() { m_pImage = NULL; }
private:
  CGUIImage *m_pImage;
};
