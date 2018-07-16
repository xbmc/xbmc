/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdlib.h>
#include <functional>
#include <memory>
#include <string>

#include "guilib/ISliderCallback.h"
#include "utils/ILocalizer.h"

class CGUIControl;
class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUILabelControl;

class CSetting;
class CSettingControlSlider;
class CSettingString;
class CSettingPath;

class CFileItemList;
class CVariant;

class CGUIControlBaseSetting : protected ILocalizer
{
public:
  CGUIControlBaseSetting(int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlBaseSetting() override = default;

  int GetID() const { return m_id; }
  std::shared_ptr<CSetting> GetSetting() { return m_pSetting; }

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
  // implementation of ILocalizer
  std::string Localize(std::uint32_t code) const override;

  int m_id;
  std::shared_ptr<CSetting> m_pSetting;
  ILocalizer* m_localizer;
  bool m_delayed;
  bool m_valid;
};

class CGUIControlRadioButtonSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlRadioButtonSetting(CGUIRadioButtonControl* pRadioButton, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlRadioButtonSetting() override;

  void Select(bool bSelect);

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pRadioButton); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pRadioButton = NULL; }
private:
  CGUIRadioButtonControl *m_pRadioButton;
};

class CGUIControlSpinExSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSpinExSetting(CGUISpinControlEx* pSpin, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlSpinExSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pSpin); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pSpin = NULL; }
private:
  void FillControl();
  void FillIntegerSettingControl();
  CGUISpinControlEx *m_pSpin;
};

class CGUIControlListSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlListSetting(CGUIButtonControl* pButton, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlListSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pButton); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pButton = NULL; }
private:
  bool GetItems(std::shared_ptr<const CSetting> setting, CFileItemList &items) const;
  bool GetIntegerItems(std::shared_ptr<const CSetting> setting, CFileItemList &items) const;
  bool GetStringItems(std::shared_ptr<const CSetting> setting, CFileItemList &items) const;

  CGUIButtonControl *m_pButton;
};

class CGUIControlButtonSetting : public CGUIControlBaseSetting, protected ISliderCallback
{
public:
  CGUIControlButtonSetting(CGUIButtonControl* pButton, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlButtonSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pButton); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pButton = NULL; }

  static bool GetPath(std::shared_ptr<CSettingPath> pathSetting, ILocalizer* localizer);
protected:
  // implementations of ISliderCallback
  void OnSliderChange(void *data, CGUISliderControl *slider) override;

private:
  CGUIButtonControl *m_pButton;
};

class CGUIControlEditSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlEditSetting(CGUIEditControl* pButton, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlEditSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pEdit); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pEdit = NULL; }
private:
  static bool InputValidation(const std::string &input, void *data);

  CGUIEditControl *m_pEdit;
};

class CGUIControlSliderSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSliderSetting(CGUISettingsSliderControl* pSlider, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlSliderSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pSlider); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pSlider = NULL; }

  static std::string GetText(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum, ILocalizer* localizer);

private:
  CGUISettingsSliderControl *m_pSlider;
};

class CGUIControlRangeSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlRangeSetting(CGUISettingsSliderControl* pSlider, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlRangeSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pSlider); }
  bool OnClick() override;
  void Update(bool updateDisplayOnly = false) override;
  void Clear() override { m_pSlider = NULL; }

private:
  CGUISettingsSliderControl *m_pSlider;
};

class CGUIControlSeparatorSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlSeparatorSetting(CGUIImage* pImage, int id, ILocalizer* localizer);
  ~CGUIControlSeparatorSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pImage); }
  bool OnClick() override { return false; }
  using CGUIControlBaseSetting::Update;
  void Update() {}
  void Clear() override { m_pImage = NULL; }
private:
  CGUIImage *m_pImage;
};

class CGUIControlGroupTitleSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlGroupTitleSetting(CGUILabelControl* pLabel, int id, ILocalizer* localizer);
  ~CGUIControlGroupTitleSetting() override;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pLabel); }
  bool OnClick() override { return false; }
  using CGUIControlBaseSetting::Update;
  void Update() {}
  void Clear() override { m_pLabel = NULL; }
private:
  CGUILabelControl *m_pLabel;
};

class CGUIControlLabelSetting : public CGUIControlBaseSetting
{
public:
  CGUIControlLabelSetting(CGUIButtonControl* pButton, int id, std::shared_ptr<CSetting> pSetting, ILocalizer* localizer);
  ~CGUIControlLabelSetting() override = default;

  CGUIControl* GetControl() override { return reinterpret_cast<CGUIControl*>(m_pButton); }
  void Clear() override { m_pButton = NULL; }

private:
  CGUIButtonControl *m_pButton;
};
