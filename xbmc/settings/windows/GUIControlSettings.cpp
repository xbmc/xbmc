/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlSettings.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "addons/settings/SettingUrlEncodedString.h"
#include "dialogs/GUIDialogColorPicker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSlider.h"
#include "guilib/GUIColorButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <set>
#include <utility>

using namespace ADDON;

static std::string Localize(std::uint32_t code,
                            ILocalizer* localizer,
                            const std::string& addonId = "")
{
  if (localizer == nullptr)
    return "";

  if (!addonId.empty())
  {
    std::string label = g_localizeStrings.GetAddonString(addonId, code);
    if (!label.empty())
      return label;
  }

  return localizer->Localize(code);
}

template<typename TValueType>
static CFileItemPtr GetFileItem(const std::string& label,
                                const std::string& label2,
                                const TValueType& value,
                                const std::vector<std::pair<std::string, CVariant>>& properties,
                                const std::set<TValueType>& selectedValues)
{
  CFileItemPtr item(new CFileItem(label));
  item->SetProperty("value", value);
  item->SetLabel2(label2);

  for (const auto& prop : properties)
    item->SetProperty(prop.first, prop.second);

  if (selectedValues.find(value) != selectedValues.end())
    item->Select(true);

  return item;
}

template<class SettingOption>
static bool CompareSettingOptionAseconding(const SettingOption& lhs, const SettingOption& rhs)
{
  return StringUtils::CompareNoCase(lhs.label, rhs.label) < 0;
}

template<class SettingOption>
static bool CompareSettingOptionDeseconding(const SettingOption& lhs, const SettingOption& rhs)
{
  return StringUtils::CompareNoCase(lhs.label, rhs.label) > 0;
}

static bool GetIntegerOptions(const SettingConstPtr& setting,
                              IntegerSettingOptions& options,
                              std::set<int>& selectedOptions,
                              ILocalizer* localizer,
                              bool updateOptions)
{
  std::shared_ptr<const CSettingInt> pSettingInt = NULL;
  if (setting->GetType() == SettingType::Integer)
    pSettingInt = std::static_pointer_cast<const CSettingInt>(setting);
  else if (setting->GetType() == SettingType::List)
  {
    std::shared_ptr<const CSettingList> settingList =
        std::static_pointer_cast<const CSettingList>(setting);
    if (settingList->GetElementType() != SettingType::Integer)
      return false;

    pSettingInt = std::static_pointer_cast<const CSettingInt>(settingList->GetDefinition());
  }

  switch (pSettingInt->GetOptionsType())
  {
    case SettingOptionsType::StaticTranslatable:
    {
      const TranslatableIntegerSettingOptions& settingOptions =
          pSettingInt->GetTranslatableOptions();
      for (const auto& option : settingOptions)
        options.emplace_back(Localize(option.label, localizer, option.addonId), option.value);
      break;
    }

    case SettingOptionsType::Static:
    {
      const IntegerSettingOptions& settingOptions = pSettingInt->GetOptions();
      options.insert(options.end(), settingOptions.begin(), settingOptions.end());
      break;
    }

    case SettingOptionsType::Dynamic:
    {
      IntegerSettingOptions settingOptions;
      if (updateOptions)
        settingOptions = std::const_pointer_cast<CSettingInt>(pSettingInt)->UpdateDynamicOptions();
      else
        settingOptions = pSettingInt->GetDynamicOptions();
      options.insert(options.end(), settingOptions.begin(), settingOptions.end());
      break;
    }

    case SettingOptionsType::Unknown:
    default:
    {
      std::shared_ptr<const CSettingControlFormattedRange> control =
          std::static_pointer_cast<const CSettingControlFormattedRange>(pSettingInt->GetControl());
      for (int i = pSettingInt->GetMinimum(); i <= pSettingInt->GetMaximum();
           i += pSettingInt->GetStep())
      {
        std::string strLabel;
        if (i == pSettingInt->GetMinimum() && control->GetMinimumLabel() > -1)
          strLabel = Localize(control->GetMinimumLabel(), localizer);
        else if (control->GetFormatLabel() > -1)
          strLabel = StringUtils::Format(Localize(control->GetFormatLabel(), localizer), i);
        else
          strLabel = StringUtils::Format(control->GetFormatString(), i);

        options.emplace_back(strLabel, i);
      }

      break;
    }
  }

  switch (pSettingInt->GetOptionsSort())
  {
    case SettingOptionsSort::Ascending:
      std::sort(options.begin(), options.end(),
                CompareSettingOptionAseconding<IntegerSettingOption>);
      break;

    case SettingOptionsSort::Descending:
      std::sort(options.begin(), options.end(),
                CompareSettingOptionDeseconding<IntegerSettingOption>);
      break;

    case SettingOptionsSort::NoSorting:
    default:
      break;
  }

  // this must be done after potentially calling CSettingInt::UpdateDynamicOptions() because it can
  // change the value of the setting
  if (setting->GetType() == SettingType::Integer)
    selectedOptions.insert(pSettingInt->GetValue());
  else if (setting->GetType() == SettingType::List)
  {
    std::vector<CVariant> list =
        CSettingUtils::GetList(std::static_pointer_cast<const CSettingList>(setting));
    for (const auto& itValue : list)
      selectedOptions.insert((int)itValue.asInteger());
  }
  else
    return false;

  return true;
}

static bool GetStringOptions(const SettingConstPtr& setting,
                             StringSettingOptions& options,
                             std::set<std::string>& selectedOptions,
                             ILocalizer* localizer,
                             bool updateOptions)
{
  std::shared_ptr<const CSettingString> pSettingString = NULL;
  if (setting->GetType() == SettingType::String)
    pSettingString = std::static_pointer_cast<const CSettingString>(setting);
  else if (setting->GetType() == SettingType::List)
  {
    std::shared_ptr<const CSettingList> settingList =
        std::static_pointer_cast<const CSettingList>(setting);
    if (settingList->GetElementType() != SettingType::String)
      return false;

    pSettingString = std::static_pointer_cast<const CSettingString>(settingList->GetDefinition());
  }

  switch (pSettingString->GetOptionsType())
  {
    case SettingOptionsType::StaticTranslatable:
    {
      const TranslatableStringSettingOptions& settingOptions =
          pSettingString->GetTranslatableOptions();
      for (const auto& option : settingOptions)
        options.emplace_back(Localize(option.first, localizer), option.second);
      break;
    }

    case SettingOptionsType::Static:
    {
      const StringSettingOptions& settingOptions = pSettingString->GetOptions();
      options.insert(options.end(), settingOptions.begin(), settingOptions.end());
      break;
    }

    case SettingOptionsType::Dynamic:
    {
      StringSettingOptions settingOptions;
      if (updateOptions)
        settingOptions =
            std::const_pointer_cast<CSettingString>(pSettingString)->UpdateDynamicOptions();
      else
        settingOptions = pSettingString->GetDynamicOptions();
      options.insert(options.end(), settingOptions.begin(), settingOptions.end());
      break;
    }

    case SettingOptionsType::Unknown:
    default:
      return false;
  }

  switch (pSettingString->GetOptionsSort())
  {
    case SettingOptionsSort::Ascending:
      std::sort(options.begin(), options.end(),
                CompareSettingOptionAseconding<StringSettingOption>);
      break;

    case SettingOptionsSort::Descending:
      std::sort(options.begin(), options.end(),
                CompareSettingOptionDeseconding<StringSettingOption>);
      break;

    case SettingOptionsSort::NoSorting:
    default:
      break;
  }

  // this must be done after potentially calling CSettingString::UpdateDynamicOptions() because it
  // can change the value of the setting
  if (setting->GetType() == SettingType::String)
    selectedOptions.insert(pSettingString->GetValue());
  else if (setting->GetType() == SettingType::List)
  {
    std::vector<CVariant> list =
        CSettingUtils::GetList(std::static_pointer_cast<const CSettingList>(setting));
    for (const auto& itValue : list)
      selectedOptions.insert(itValue.asString());
  }
  else
    return false;

  return true;
}

CGUIControlBaseSetting::CGUIControlBaseSetting(int id,
                                               std::shared_ptr<CSetting> pSetting,
                                               ILocalizer* localizer)
  : m_id(id), m_pSetting(std::move(pSetting)), m_localizer(localizer)
{
}

bool CGUIControlBaseSetting::IsEnabled() const
{
  return m_pSetting != NULL && m_pSetting->IsEnabled();
}

void CGUIControlBaseSetting::UpdateFromControl()
{
  Update(true, true);
}

void CGUIControlBaseSetting::UpdateFromSetting(bool updateDisplayOnly /* = false */)
{
  Update(false, updateDisplayOnly);
}

std::string CGUIControlBaseSetting::Localize(std::uint32_t code) const
{
  return ::Localize(code, m_localizer);
}

void CGUIControlBaseSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || updateDisplayOnly)
    return;

  CGUIControl* control = GetControl();
  if (control == NULL)
    return;

  control->SetEnabled(IsEnabled());
  if (m_pSetting)
    control->SetVisible(m_pSetting->IsVisible());
  SetValid(true);
}

CGUIControlRadioButtonSetting::CGUIControlRadioButtonSetting(CGUIRadioButtonControl* pRadioButton,
                                                             int id,
                                                             std::shared_ptr<CSetting> pSetting,
                                                             ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pRadioButton = pRadioButton;
  if (m_pRadioButton == NULL)
    return;

  m_pRadioButton->SetID(id);
}

CGUIControlRadioButtonSetting::~CGUIControlRadioButtonSetting() = default;

bool CGUIControlRadioButtonSetting::OnClick()
{
  SetValid(std::static_pointer_cast<CSettingBool>(m_pSetting)
               ->SetValue(!std::static_pointer_cast<CSettingBool>(m_pSetting)->GetValue()));
  return IsValid();
}

void CGUIControlRadioButtonSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || m_pRadioButton == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  m_pRadioButton->SetSelected(std::static_pointer_cast<CSettingBool>(m_pSetting)->GetValue());
}

CGUIControlColorButtonSetting::CGUIControlColorButtonSetting(
    CGUIColorButtonControl* pColorControl,
    int id,
    const std::shared_ptr<CSetting>& pSetting,
    ILocalizer* localizer)
  : CGUIControlBaseSetting(id, pSetting, localizer)
{
  m_pColorButton = pColorControl;
  if (!m_pColorButton)
    return;

  m_pColorButton->SetID(id);
}

CGUIControlColorButtonSetting::~CGUIControlColorButtonSetting() = default;

bool CGUIControlColorButtonSetting::OnClick()
{
  if (!m_pColorButton)
    return false;

  std::shared_ptr<CSettingString> settingHexColor =
      std::static_pointer_cast<CSettingString>(m_pSetting);

  CGUIDialogColorPicker* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogColorPicker>(
          WINDOW_DIALOG_COLOR_PICKER);
  if (!dialog)
    return false;

  dialog->Reset();
  dialog->SetHeading(CVariant{Localize(m_pSetting->GetLabel())});
  dialog->LoadColors();
  std::string hexColor;
  if (settingHexColor)
    hexColor = settingHexColor.get()->GetValue();
  dialog->SetSelectedColor(hexColor);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  SetValid(
      std::static_pointer_cast<CSettingString>(m_pSetting)->SetValue(dialog->GetSelectedColor()));
  return IsValid();
}

void CGUIControlColorButtonSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || !m_pColorButton)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);
  // Set the color to apply to the preview color box
  m_pColorButton->SetImageBoxColor(
      std::static_pointer_cast<CSettingString>(m_pSetting)->GetValue());
}

CGUIControlSpinExSetting::CGUIControlSpinExSetting(CGUISpinControlEx* pSpin,
                                                   int id,
                                                   std::shared_ptr<CSetting> pSetting,
                                                   ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pSpin = pSpin;
  if (m_pSpin == NULL)
    return;

  m_pSpin->SetID(id);

  const std::string& controlFormat = m_pSetting->GetControl()->GetFormat();
  if (controlFormat == "number")
  {
    std::shared_ptr<CSettingNumber> pSettingNumber =
        std::static_pointer_cast<CSettingNumber>(m_pSetting);
    m_pSpin->SetType(SPIN_CONTROL_TYPE_FLOAT);
    m_pSpin->SetFloatRange(static_cast<float>(pSettingNumber->GetMinimum()),
                           static_cast<float>(pSettingNumber->GetMaximum()));
    m_pSpin->SetFloatInterval(static_cast<float>(pSettingNumber->GetStep()));
  }
  else if (controlFormat == "integer")
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
  else if (controlFormat == "string")
  {
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);

    if (m_pSetting->GetType() == SettingType::Integer)
      FillIntegerSettingControl(false);
    else if (m_pSetting->GetType() == SettingType::Number)
    {
      std::shared_ptr<CSettingNumber> pSettingNumber =
          std::static_pointer_cast<CSettingNumber>(m_pSetting);
      std::shared_ptr<const CSettingControlFormattedRange> control =
          std::static_pointer_cast<const CSettingControlFormattedRange>(m_pSetting->GetControl());
      int index = 0;
      for (double value = pSettingNumber->GetMinimum(); value <= pSettingNumber->GetMaximum();
           value += pSettingNumber->GetStep(), index++)
      {
        std::string strLabel;
        if (value == pSettingNumber->GetMinimum() && control->GetMinimumLabel() > -1)
          strLabel = Localize(control->GetMinimumLabel());
        else if (control->GetFormatLabel() > -1)
          strLabel = StringUtils::Format(Localize(control->GetFormatLabel()), value);
        else
          strLabel = StringUtils::Format(control->GetFormatString(), value);

        m_pSpin->AddLabel(strLabel, index);
      }
    }
  }
}

CGUIControlSpinExSetting::~CGUIControlSpinExSetting() = default;

bool CGUIControlSpinExSetting::OnClick()
{
  if (m_pSpin == NULL)
    return false;

  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
      SetValid(std::static_pointer_cast<CSettingInt>(m_pSetting)->SetValue(m_pSpin->GetValue()));
      break;

    case SettingType::Number:
    {
      auto pSettingNumber = std::static_pointer_cast<CSettingNumber>(m_pSetting);
      const auto& controlFormat = m_pSetting->GetControl()->GetFormat();
      if (controlFormat == "number")
        SetValid(pSettingNumber->SetValue(static_cast<double>(m_pSpin->GetFloatValue())));
      else
        SetValid(pSettingNumber->SetValue(pSettingNumber->GetMinimum() +
                                          pSettingNumber->GetStep() * m_pSpin->GetValue()));

      break;
    }

    case SettingType::String:
      SetValid(std::static_pointer_cast<CSettingString>(m_pSetting)
                   ->SetValue(m_pSpin->GetStringValue()));
      break;

    default:
      return false;
  }

  return IsValid();
}

void CGUIControlSpinExSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || m_pSpin == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  FillControl(!updateDisplayOnly);

  if (!updateDisplayOnly)
  {
    // disable the spinner if it has less than two items
    if (!m_pSpin->IsDisabled() && (m_pSpin->GetMaximum() - m_pSpin->GetMinimum()) == 0)
      m_pSpin->SetEnabled(false);
  }
}

void CGUIControlSpinExSetting::FillControl(bool updateValues)
{
  if (m_pSpin == NULL)
    return;

  if (updateValues)
    m_pSpin->Clear();

  const std::string& controlFormat = m_pSetting->GetControl()->GetFormat();
  if (controlFormat == "number")
  {
    std::shared_ptr<CSettingNumber> pSettingNumber =
        std::static_pointer_cast<CSettingNumber>(m_pSetting);
    m_pSpin->SetFloatValue((float)pSettingNumber->GetValue());
  }
  else if (controlFormat == "integer")
    FillIntegerSettingControl(updateValues);
  else if (controlFormat == "string")
  {
    if (m_pSetting->GetType() == SettingType::Integer)
      FillIntegerSettingControl(updateValues);
    else if (m_pSetting->GetType() == SettingType::Number)
      FillFloatSettingControl();
    else if (m_pSetting->GetType() == SettingType::String)
      FillStringSettingControl(updateValues);
  }
}

void CGUIControlSpinExSetting::FillIntegerSettingControl(bool updateValues)
{
  IntegerSettingOptions options;
  std::set<int> selectedValues;
  // get the integer options
  if (!GetIntegerOptions(m_pSetting, options, selectedValues, m_localizer, updateValues) ||
      selectedValues.size() != 1)
    return;

  if (updateValues)
  {
    // add them to the spinner
    for (const auto& option : options)
      m_pSpin->AddLabel(option.label, option.value);
  }

  // and set the current value
  m_pSpin->SetValue(*selectedValues.begin());
}

void CGUIControlSpinExSetting::FillFloatSettingControl()
{
  std::shared_ptr<CSettingNumber> pSettingNumber =
      std::static_pointer_cast<CSettingNumber>(m_pSetting);
  std::shared_ptr<const CSettingControlFormattedRange> control =
      std::static_pointer_cast<const CSettingControlFormattedRange>(m_pSetting->GetControl());
  int index = 0;
  int currentIndex = 0;
  for (double value = pSettingNumber->GetMinimum(); value <= pSettingNumber->GetMaximum();
       value += pSettingNumber->GetStep(), index++)
  {
    if (value == pSettingNumber->GetValue())
    {
      currentIndex = index;
      break;
    }
  }

  m_pSpin->SetValue(currentIndex);
}

void CGUIControlSpinExSetting::FillStringSettingControl(bool updateValues)
{
  StringSettingOptions options;
  std::set<std::string> selectedValues;
  // get the string options
  if (!GetStringOptions(m_pSetting, options, selectedValues, m_localizer, updateValues) ||
      selectedValues.size() != 1)
    return;

  if (updateValues)
  {
    // add them to the spinner
    for (const auto& option : options)
      m_pSpin->AddLabel(option.label, option.value);
  }

  // and set the current value
  m_pSpin->SetStringValue(*selectedValues.begin());
}

CGUIControlListSetting::CGUIControlListSetting(CGUIButtonControl* pButton,
                                               int id,
                                               std::shared_ptr<CSetting> pSetting,
                                               ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pButton = pButton;
  if (m_pButton == NULL)
    return;

  m_pButton->SetID(id);
}

CGUIControlListSetting::~CGUIControlListSetting() = default;

bool CGUIControlListSetting::OnClick()
{
  if (m_pButton == NULL)
    return false;

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (dialog == NULL)
    return false;

  CFileItemList options;
  std::shared_ptr<const CSettingControlList> control =
      std::static_pointer_cast<const CSettingControlList>(m_pSetting->GetControl());
  bool optionsValid = GetItems(m_pSetting, options, false);

  bool bValueAdded = false;
  bool bAllowNewOption = false;
  if (m_pSetting->GetType() == SettingType::List)
  {
    std::shared_ptr<const CSettingList> settingList =
      std::static_pointer_cast<const CSettingList>(m_pSetting);
    if (settingList->GetElementType() == SettingType::String)
    {
      bAllowNewOption = std::static_pointer_cast<const CSettingString>(settingList->GetDefinition())
        ->AllowNewOption();
    }
  }
  if (!bAllowNewOption)
  {
    // Do not show dialog if
    // there are no items to be chosen
    if (!optionsValid || options.Size() <= 0)
      return false;

    dialog->Reset();
    dialog->SetHeading(CVariant{Localize(m_pSetting->GetLabel())});
    dialog->SetItems(options);
    dialog->SetMultiSelection(control->CanMultiSelect());
    dialog->SetUseDetails(control->UseDetails());
    dialog->Open();

    if (!dialog->IsConfirmed())
      return false;
  }
  else
  {
    // Possible to add items, as well as select from any options given
    // Add any current values that are not options as items in list
    std::vector<CVariant> list =
      CSettingUtils::GetList(std::static_pointer_cast<const CSettingList>(m_pSetting));
    for (const auto& value : list)
    {
      bool found = std::any_of(options.begin(), options.end(), [&](const auto& p) {
        return p->GetProperty("value").asString() == value.asString();
      });
      if (!found)
      {
        CFileItemPtr item(new CFileItem(value.asString()));
        item->SetProperty("value", value.asString());
        item->Select(true);
        options.Add(item);
      }
    }

    bool bRepeat = true;
    while (bRepeat)
    {
      std::string strAddButton = Localize(control->GetAddButtonLabel());
      if (strAddButton.empty())
        strAddButton = Localize(15019); // "ADD"
      dialog->Reset(); // Clears AddButtonPressed
      dialog->SetHeading(CVariant{ Localize(m_pSetting->GetLabel()) });
      dialog->SetItems(options);
      dialog->SetMultiSelection(control->CanMultiSelect());
      dialog->EnableButton2(bAllowNewOption, strAddButton);

      dialog->Open();

      if (!dialog->IsConfirmed())
        return false;

      if (dialog->IsButton2Pressed())
      {
        // Get new list value
        std::string strLabel;
        bool bValidType = false;
        while (!bValidType && CGUIKeyboardFactory::ShowAndGetInput(
          strLabel, CVariant{ Localize(control->GetAddButtonLabel()) }, false))
        {
          // Validate new value is unique and truncate at any comma
          StringUtils::Trim(strLabel);
          strLabel = strLabel.substr(0, strLabel.find(','));
          if (!strLabel.empty())
          {
            bValidType = !std::any_of(options.begin(), options.end(), [&](const auto& p) {
              return p->GetProperty("value").asString() == strLabel;
            });
          }
          if (bValidType)
          { // Add new value to the list of options
            CFileItemPtr pItem(new CFileItem(strLabel));
            pItem->Select(true);
            pItem->SetProperty("value", strLabel);
            options.Add(pItem);
            bValueAdded = true;
          }
        }
      }
      bRepeat = dialog->IsButton2Pressed();
    }
  }

  std::vector<CVariant> values;
  for (int i : dialog->GetSelectedItems())
  {
    const CFileItemPtr item = options.Get(i);
    if (item == NULL || !item->HasProperty("value"))
      return false;

    values.push_back(item->GetProperty("value"));
  }

  bool ret = false;
  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
      if (values.size() > 1)
        return false;
      ret = std::static_pointer_cast<CSettingInt>(m_pSetting)
                ->SetValue((int)values.at(0).asInteger());
      break;

    case SettingType::String:
      if (values.size() > 1)
        return false;
      ret = std::static_pointer_cast<CSettingString>(m_pSetting)->SetValue(values.at(0).asString());
      break;

    case SettingType::List:
      ret = CSettingUtils::SetList(std::static_pointer_cast<CSettingList>(m_pSetting), values);
      break;

    default:
      return false;
  }

  if (ret)
    UpdateFromSetting(!bValueAdded);
  else
    SetValid(false);

  return IsValid();
}

void CGUIControlListSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || m_pButton == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  CFileItemList options;
  std::shared_ptr<const CSettingControlList> control =
      std::static_pointer_cast<const CSettingControlList>(m_pSetting->GetControl());
  bool optionsValid = GetItems(m_pSetting, options, !updateDisplayOnly);

  bool bAllowNewOption = false;
  if (m_pSetting->GetType() == SettingType::List)
  {
    std::shared_ptr<const CSettingList> settingList =
        std::static_pointer_cast<const CSettingList>(m_pSetting);
    if (settingList->GetElementType() == SettingType::String)
    {
      bAllowNewOption = std::static_pointer_cast<const CSettingString>(settingList->GetDefinition())
                            ->AllowNewOption();
    }
  }

  std::string label2;
  if (optionsValid && !control->HideValue())
  {
    SettingControlListValueFormatter formatter = control->GetFormatter();
    if (formatter)
      label2 = formatter(m_pSetting);

    if (label2.empty() && bAllowNewOption)
    {
      const std::shared_ptr<const CSettingList> settingList =
          std::static_pointer_cast<const CSettingList>(m_pSetting);
      label2 = settingList->ToString();
    }

    if (label2.empty())
    {
      std::vector<std::string> labels;
      for (int index = 0; index < options.Size(); index++)
      {
        const CFileItemPtr pItem = options.Get(index);
        if (pItem->IsSelected())
          labels.push_back(pItem->GetLabel());
      }

      label2 = StringUtils::Join(labels, ", ");
    }
  }

  m_pButton->SetLabel2(label2);

  if (!updateDisplayOnly)
  {
    // Disable the control if no items can be added and
    // there are no items to be chosen
    if (!m_pButton->IsDisabled() && !bAllowNewOption && (options.Size() <= 0))
      m_pButton->SetEnabled(false);
  }
}

bool CGUIControlListSetting::GetItems(const SettingConstPtr& setting,
                                      CFileItemList& items,
                                      bool updateItems) const
{
  std::shared_ptr<const CSettingControlList> control =
      std::static_pointer_cast<const CSettingControlList>(setting->GetControl());
  const std::string& controlFormat = control->GetFormat();

  if (controlFormat == "integer")
    return GetIntegerItems(setting, items, updateItems);
  else if (controlFormat == "string")
  {
    if (setting->GetType() == SettingType::Integer ||
        (setting->GetType() == SettingType::List &&
         std::static_pointer_cast<const CSettingList>(setting)->GetElementType() ==
             SettingType::Integer))
      return GetIntegerItems(setting, items, updateItems);
    else if (setting->GetType() == SettingType::String ||
             (setting->GetType() == SettingType::List &&
              std::static_pointer_cast<const CSettingList>(setting)->GetElementType() ==
                  SettingType::String))
      return GetStringItems(setting, items, updateItems);
  }
  else
    return false;

  return true;
}

bool CGUIControlListSetting::GetIntegerItems(const SettingConstPtr& setting,
                                             CFileItemList& items,
                                             bool updateItems) const
{
  IntegerSettingOptions options;
  std::set<int> selectedValues;
  // get the integer options
  if (!GetIntegerOptions(setting, options, selectedValues, m_localizer, updateItems))
    return false;

  // turn them into CFileItems and add them to the item list
  for (const auto& option : options)
    items.Add(
        GetFileItem(option.label, option.label2, option.value, option.properties, selectedValues));

  return true;
}

bool CGUIControlListSetting::GetStringItems(const SettingConstPtr& setting,
                                            CFileItemList& items,
                                            bool updateItems) const
{
  StringSettingOptions options;
  std::set<std::string> selectedValues;
  // get the string options
  if (!GetStringOptions(setting, options, selectedValues, m_localizer, updateItems))
    return false;

  // turn them into CFileItems and add them to the item list
  for (const auto& option : options)
    items.Add(
        GetFileItem(option.label, option.label2, option.value, option.properties, selectedValues));

  return true;
}

CGUIControlButtonSetting::CGUIControlButtonSetting(CGUIButtonControl* pButton,
                                                   int id,
                                                   std::shared_ptr<CSetting> pSetting,
                                                   ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pButton = pButton;
  if (m_pButton == NULL)
    return;

  m_pButton->SetID(id);
}

CGUIControlButtonSetting::~CGUIControlButtonSetting() = default;

bool CGUIControlButtonSetting::OnClick()
{
  if (m_pButton == NULL)
    return false;

  std::shared_ptr<const ISettingControl> control = m_pSetting->GetControl();
  const std::string& controlType = control->GetType();
  const std::string& controlFormat = control->GetFormat();
  if (controlType == "button")
  {
    std::shared_ptr<const CSettingControlButton> buttonControl =
        std::static_pointer_cast<const CSettingControlButton>(control);
    if (controlFormat == "addon")
    {
      // prompt for the addon
      std::shared_ptr<CSettingAddon> setting;
      std::vector<std::string> addonIDs;
      if (m_pSetting->GetType() == SettingType::List)
      {
        std::shared_ptr<CSettingList> settingList =
            std::static_pointer_cast<CSettingList>(m_pSetting);
        setting = std::static_pointer_cast<CSettingAddon>(settingList->GetDefinition());
        for (const SettingPtr& addon : settingList->GetValue())
          addonIDs.push_back(std::static_pointer_cast<CSettingAddon>(addon)->GetValue());
      }
      else
      {
        setting = std::static_pointer_cast<CSettingAddon>(m_pSetting);
        addonIDs.push_back(setting->GetValue());
      }

      if (CGUIWindowAddonBrowser::SelectAddonID(
              setting->GetAddonType(), addonIDs, setting->AllowEmpty(),
              buttonControl->ShowAddonDetails(), m_pSetting->GetType() == SettingType::List,
              buttonControl->ShowInstalledAddons(), buttonControl->ShowInstallableAddons(),
              buttonControl->ShowMoreAddons()) != 1)
        return false;

      if (m_pSetting->GetType() == SettingType::List)
        std::static_pointer_cast<CSettingList>(m_pSetting)->FromString(addonIDs);
      else
        SetValid(setting->SetValue(addonIDs[0]));
    }
    else if (controlFormat == "path" || controlFormat == "file" || controlFormat == "image")
      SetValid(GetPath(std::static_pointer_cast<CSettingPath>(m_pSetting), m_localizer));
    else if (controlFormat == "date")
    {
      std::shared_ptr<CSettingDate> settingDate =
          std::static_pointer_cast<CSettingDate>(m_pSetting);

      KODI::TIME::SystemTime systemdate;
      settingDate->GetDate().GetAsSystemTime(systemdate);
      if (CGUIDialogNumeric::ShowAndGetDate(systemdate, Localize(buttonControl->GetHeading())))
        SetValid(settingDate->SetDate(CDateTime(systemdate)));
    }
    else if (controlFormat == "time")
    {
      std::shared_ptr<CSettingTime> settingTime =
          std::static_pointer_cast<CSettingTime>(m_pSetting);

      KODI::TIME::SystemTime systemtime;
      settingTime->GetTime().GetAsSystemTime(systemtime);

      if (CGUIDialogNumeric::ShowAndGetTime(systemtime, Localize(buttonControl->GetHeading())))
        SetValid(settingTime->SetTime(CDateTime(systemtime)));
    }
    else if (controlFormat == "action")
    {
      // simply call the OnSettingAction callback and whoever knows what to
      // do can do so (based on the setting's identification)
      m_pSetting->OnSettingAction(m_pSetting);
      SetValid(true);
    }
  }
  else if (controlType == "slider")
  {
    float value, min, step, max;
    if (m_pSetting->GetType() == SettingType::Integer)
    {
      std::shared_ptr<CSettingInt> settingInt = std::static_pointer_cast<CSettingInt>(m_pSetting);
      value = (float)settingInt->GetValue();
      min = (float)settingInt->GetMinimum();
      step = (float)settingInt->GetStep();
      max = (float)settingInt->GetMaximum();
    }
    else if (m_pSetting->GetType() == SettingType::Number)
    {
      std::shared_ptr<CSettingNumber> settingNumber =
          std::static_pointer_cast<CSettingNumber>(m_pSetting);
      value = (float)settingNumber->GetValue();
      min = (float)settingNumber->GetMinimum();
      step = (float)settingNumber->GetStep();
      max = (float)settingNumber->GetMaximum();
    }
    else
      return false;

    std::shared_ptr<const CSettingControlSlider> sliderControl =
        std::static_pointer_cast<const CSettingControlSlider>(control);
    CGUIDialogSlider::ShowAndGetInput(Localize(sliderControl->GetHeading()), value, min, step, max,
                                      this, NULL);
    SetValid(true);
  }

  // update the displayed value
  UpdateFromSetting(true);

  return IsValid();
}

void CGUIControlButtonSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || m_pButton == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  std::string strText;
  std::shared_ptr<const ISettingControl> control = m_pSetting->GetControl();
  const std::string& controlType = control->GetType();
  const std::string& controlFormat = control->GetFormat();

  if (controlType == "button")
  {
    if (!std::static_pointer_cast<const CSettingControlButton>(control)->HideValue())
    {
      auto setting = m_pSetting;
      if (m_pSetting->GetType() == SettingType::List)
        setting = std::static_pointer_cast<CSettingList>(m_pSetting)->GetDefinition();

      switch (setting->GetType())
      {
        case SettingType::String:
        {
          if (controlFormat == "addon")
          {
            std::vector<std::string> addonIDs;
            if (m_pSetting->GetType() == SettingType::List)
            {
              for (const auto& addonSetting :
                   std::static_pointer_cast<CSettingList>(m_pSetting)->GetValue())
                addonIDs.push_back(
                    std::static_pointer_cast<CSettingAddon>(addonSetting)->GetValue());
            }
            else
              addonIDs.push_back(std::static_pointer_cast<CSettingString>(setting)->GetValue());

            std::vector<std::string> addonNames;
            for (const auto& addonID : addonIDs)
            {
              ADDON::AddonPtr addon;
              if (CServiceBroker::GetAddonMgr().GetAddon(addonID, addon,
                                                         ADDON::OnlyEnabled::CHOICE_YES))
                addonNames.push_back(addon->Name());
            }

            if (addonNames.empty())
              strText = g_localizeStrings.Get(231); // None
            else
              strText = StringUtils::Join(addonNames, ", ");
          }
          else
          {
            std::string strValue = std::static_pointer_cast<CSettingString>(setting)->GetValue();
            if (controlFormat == "path" || controlFormat == "file" || controlFormat == "image")
            {
              std::string shortPath;
              if (CUtil::MakeShortenPath(strValue, shortPath, 30))
                strText = shortPath;
            }
            else if (controlFormat == "infolabel")
            {
              strText = strValue;
              if (strText.empty())
                strText = g_localizeStrings.Get(231); // None
            }
            else
              strText = strValue;
          }

          break;
        }

        case SettingType::Action:
        {
          // CSettingAction.
          // Note: This can be removed once all settings use a proper control & format combination.
          // CSettingAction is strictly speaking not designed to have a label2, it does not even have a value.
          strText = m_pButton->GetLabel2();

          break;
        }

        default:
          break;
      }
    }
  }
  else if (controlType == "slider")
  {
    switch (m_pSetting->GetType())
    {
      case SettingType::Integer:
      {
        std::shared_ptr<const CSettingInt> settingInt =
            std::static_pointer_cast<CSettingInt>(m_pSetting);
        strText = CGUIControlSliderSetting::GetText(m_pSetting, settingInt->GetValue(),
                                                    settingInt->GetMinimum(), settingInt->GetStep(),
                                                    settingInt->GetMaximum(), m_localizer);
        break;
      }

      case SettingType::Number:
      {
        std::shared_ptr<const CSettingNumber> settingNumber =
            std::static_pointer_cast<CSettingNumber>(m_pSetting);
        strText = CGUIControlSliderSetting::GetText(
            m_pSetting, settingNumber->GetValue(), settingNumber->GetMinimum(),
            settingNumber->GetStep(), settingNumber->GetMaximum(), m_localizer);
        break;
      }

      default:
        break;
    }
  }

  m_pButton->SetLabel2(strText);
}

bool CGUIControlButtonSetting::GetPath(const std::shared_ptr<CSettingPath>& pathSetting,
                                       ILocalizer* localizer)
{
  if (pathSetting == NULL)
    return false;

  std::string path = pathSetting->GetValue();

  VECSOURCES shares;
  bool localSharesOnly = false;
  const std::vector<std::string>& sources = pathSetting->GetSources();
  for (const auto& source : sources)
  {
    if (StringUtils::EqualsNoCase(source, "local"))
      localSharesOnly = true;
    else
    {
      VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources(source);
      if (sources != NULL)
        shares.insert(shares.end(), sources->begin(), sources->end());
    }
  }

  CServiceBroker::GetMediaManager().GetLocalDrives(shares);
  if (!localSharesOnly)
    CServiceBroker::GetMediaManager().GetNetworkLocations(shares);

  bool result = false;
  std::shared_ptr<const CSettingControlButton> control =
      std::static_pointer_cast<const CSettingControlButton>(pathSetting->GetControl());
  const auto heading = ::Localize(control->GetHeading(), localizer);
  if (control->GetFormat() == "file")
    result = CGUIDialogFileBrowser::ShowAndGetFile(
        shares, pathSetting->GetMasking(CServiceBroker::GetFileExtensionProvider()), heading, path,
        control->UseImageThumbs(), control->UseFileDirectories());
  else if (control->GetFormat() == "image")
  {
    /* Check setting contains own masking, to filter required image type.
     * e.g. png only needed
     * <constraints>
     *   <masking>*.png</masking>
     * </constraints>
     * <control type="button" format="image">
     *   ...
     */
    std::string ext = pathSetting->GetMasking(CServiceBroker::GetFileExtensionProvider());
    if (ext.empty())
      ext = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
    result = CGUIDialogFileBrowser::ShowAndGetFile(shares, ext, heading, path, true);
  }
  else
    result =
        CGUIDialogFileBrowser::ShowAndGetDirectory(shares, heading, path, pathSetting->Writable());

  if (!result)
    return false;

  return pathSetting->SetValue(path);
}

void CGUIControlButtonSetting::OnSliderChange(void* data, CGUISliderControl* slider)
{
  if (slider == NULL)
    return;

  std::string strText;
  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
    {
      std::shared_ptr<CSettingInt> settingInt = std::static_pointer_cast<CSettingInt>(m_pSetting);
      if (settingInt->SetValue(slider->GetIntValue()))
        strText = CGUIControlSliderSetting::GetText(m_pSetting, settingInt->GetValue(),
                                                    settingInt->GetMinimum(), settingInt->GetStep(),
                                                    settingInt->GetMaximum(), m_localizer);
      break;
    }

    case SettingType::Number:
    {
      std::shared_ptr<CSettingNumber> settingNumber =
          std::static_pointer_cast<CSettingNumber>(m_pSetting);
      if (settingNumber->SetValue(static_cast<double>(slider->GetFloatValue())))
        strText = CGUIControlSliderSetting::GetText(
            m_pSetting, settingNumber->GetValue(), settingNumber->GetMinimum(),
            settingNumber->GetStep(), settingNumber->GetMaximum(), m_localizer);
      break;
    }

    default:
      break;
  }

  if (!strText.empty())
    slider->SetTextValue(strText);
}

CGUIControlEditSetting::CGUIControlEditSetting(CGUIEditControl* pEdit,
                                               int id,
                                               const std::shared_ptr<CSetting>& pSetting,
                                               ILocalizer* localizer)
  : CGUIControlBaseSetting(id, pSetting, localizer)
{
  std::shared_ptr<const CSettingControlEdit> control =
      std::static_pointer_cast<const CSettingControlEdit>(pSetting->GetControl());
  m_pEdit = pEdit;
  if (m_pEdit == NULL)
    return;

  m_pEdit->SetID(id);
  int heading = m_pSetting->GetLabel();
  if (control->GetHeading() > 0)
    heading = control->GetHeading();
  if (heading < 0)
    heading = 0;

  CGUIEditControl::INPUT_TYPE inputType = CGUIEditControl::INPUT_TYPE_TEXT;
  const std::string& controlFormat = control->GetFormat();
  if (controlFormat == "string")
  {
    if (control->IsHidden())
      inputType = CGUIEditControl::INPUT_TYPE_PASSWORD;
  }
  else if (controlFormat == "integer" || controlFormat == "number")
  {
    if (control->VerifyNewValue())
      inputType = CGUIEditControl::INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW;
    else
      inputType = CGUIEditControl::INPUT_TYPE_NUMBER;
  }
  else if (controlFormat == "ip")
    inputType = CGUIEditControl::INPUT_TYPE_IPADDRESS;
  else if (controlFormat == "md5")
    inputType = CGUIEditControl::INPUT_TYPE_PASSWORD_MD5;

  m_pEdit->SetInputType(inputType, heading);

  // this will automatically trigger validation so it must be executed after
  // having set the value of the control based on the value of the setting
  m_pEdit->SetInputValidation(InputValidation, this);
}

CGUIControlEditSetting::~CGUIControlEditSetting() = default;

bool CGUIControlEditSetting::OnClick()
{
  if (m_pEdit == NULL)
    return false;

  // update our string
  if (m_pSetting->GetControl()->GetFormat() == "urlencoded")
  {
    std::shared_ptr<CSettingUrlEncodedString> urlEncodedSetting =
        std::static_pointer_cast<CSettingUrlEncodedString>(m_pSetting);
    SetValid(urlEncodedSetting->SetDecodedValue(m_pEdit->GetLabel2()));
  }
  else
    SetValid(m_pSetting->FromString(m_pEdit->GetLabel2()));

  return IsValid();
}

void CGUIControlEditSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (fromControl || m_pEdit == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  std::shared_ptr<const CSettingControlEdit> control =
      std::static_pointer_cast<const CSettingControlEdit>(m_pSetting->GetControl());

  if (control->GetFormat() == "urlencoded")
  {
    std::shared_ptr<CSettingUrlEncodedString> urlEncodedSetting =
        std::static_pointer_cast<CSettingUrlEncodedString>(m_pSetting);
    m_pEdit->SetLabel2(urlEncodedSetting->GetDecodedValue());
  }
  else
    m_pEdit->SetLabel2(m_pSetting->ToString());
}

bool CGUIControlEditSetting::InputValidation(const std::string& input, void* data)
{
  if (data == NULL)
    return true;

  CGUIControlEditSetting* editControl = reinterpret_cast<CGUIControlEditSetting*>(data);
  if (editControl->GetSetting() == NULL)
    return true;

  editControl->SetValid(editControl->GetSetting()->CheckValidity(input));
  return editControl->IsValid();
}

CGUIControlSliderSetting::CGUIControlSliderSetting(CGUISettingsSliderControl* pSlider,
                                                   int id,
                                                   std::shared_ptr<CSetting> pSetting,
                                                   ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pSlider = pSlider;
  if (m_pSlider == NULL)
    return;

  m_pSlider->SetID(id);

  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
    {
      std::shared_ptr<CSettingInt> settingInt = std::static_pointer_cast<CSettingInt>(m_pSetting);
      if (m_pSetting->GetControl()->GetFormat() == "percentage")
        m_pSlider->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
      else
      {
        m_pSlider->SetType(SLIDER_CONTROL_TYPE_INT);
        m_pSlider->SetRange(settingInt->GetMinimum(), settingInt->GetMaximum());
      }
      m_pSlider->SetIntInterval(settingInt->GetStep());
      break;
    }

    case SettingType::Number:
    {
      std::shared_ptr<CSettingNumber> settingNumber =
          std::static_pointer_cast<CSettingNumber>(m_pSetting);
      m_pSlider->SetType(SLIDER_CONTROL_TYPE_FLOAT);
      m_pSlider->SetFloatRange(static_cast<float>(settingNumber->GetMinimum()),
                               static_cast<float>(settingNumber->GetMaximum()));
      m_pSlider->SetFloatInterval(static_cast<float>(settingNumber->GetStep()));
      break;
    }

    default:
      break;
  }
}

CGUIControlSliderSetting::~CGUIControlSliderSetting() = default;

bool CGUIControlSliderSetting::OnClick()
{
  if (m_pSlider == NULL)
    return false;

  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
      SetValid(
          std::static_pointer_cast<CSettingInt>(m_pSetting)->SetValue(m_pSlider->GetIntValue()));
      break;

    case SettingType::Number:
      SetValid(std::static_pointer_cast<CSettingNumber>(m_pSetting)
                   ->SetValue(static_cast<double>(m_pSlider->GetFloatValue())));
      break;

    default:
      return false;
  }

  return IsValid();
}

void CGUIControlSliderSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (m_pSlider == NULL)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  std::string strText;
  switch (m_pSetting->GetType())
  {
    case SettingType::Integer:
    {
      std::shared_ptr<const CSettingInt> settingInt =
          std::static_pointer_cast<CSettingInt>(m_pSetting);
      int value;
      if (fromControl)
        value = m_pSlider->GetIntValue();
      else
      {
        value = std::static_pointer_cast<CSettingInt>(m_pSetting)->GetValue();
        m_pSlider->SetIntValue(value);
      }

      strText = CGUIControlSliderSetting::GetText(m_pSetting, value, settingInt->GetMinimum(),
                                                  settingInt->GetStep(), settingInt->GetMaximum(),
                                                  m_localizer);
      break;
    }

    case SettingType::Number:
    {
      std::shared_ptr<const CSettingNumber> settingNumber =
          std::static_pointer_cast<CSettingNumber>(m_pSetting);
      double value;
      if (fromControl)
        value = static_cast<double>(m_pSlider->GetFloatValue());
      else
      {
        value = std::static_pointer_cast<CSettingNumber>(m_pSetting)->GetValue();
        m_pSlider->SetFloatValue((float)value);
      }

      strText = CGUIControlSliderSetting::GetText(m_pSetting, value, settingNumber->GetMinimum(),
                                                  settingNumber->GetStep(),
                                                  settingNumber->GetMaximum(), m_localizer);
      break;
    }

    default:
      break;
  }

  if (!strText.empty())
    m_pSlider->SetTextValue(strText);
}

std::string CGUIControlSliderSetting::GetText(const std::shared_ptr<CSetting>& setting,
                                              const CVariant& value,
                                              const CVariant& minimum,
                                              const CVariant& step,
                                              const CVariant& maximum,
                                              ILocalizer* localizer)
{
  if (setting == NULL || !(value.isInteger() || value.isDouble()))
    return "";

  const auto control = std::static_pointer_cast<const CSettingControlSlider>(setting->GetControl());
  if (control == NULL)
    return "";

  SettingControlSliderFormatter formatter = control->GetFormatter();
  if (formatter != NULL)
    return formatter(control, value, minimum, step, maximum);

  std::string formatString = control->GetFormatString();
  if (control->GetFormatLabel() > -1)
    formatString = ::Localize(control->GetFormatLabel(), localizer);

  std::string formattedString;
  if (FormatText(formatString, value, setting->GetId(), formattedString))
    return formattedString;

  // fall back to default formatting
  formatString = control->GetDefaultFormatString();
  if (FormatText(formatString, value, setting->GetId(), formattedString))
    return formattedString;

  return "";
}

bool CGUIControlSliderSetting::FormatText(const std::string& formatString,
                                          const CVariant& value,
                                          const std::string& settingId,
                                          std::string& formattedText)
{
  try
  {
    if (value.isDouble())
      formattedText = StringUtils::Format(formatString, value.asDouble());
    else
      formattedText = StringUtils::Format(formatString, static_cast<int>(value.asInteger()));
  }
  catch (const std::runtime_error& err)
  {
    CLog::Log(LOGERROR, "Invalid formatting with string \"{}\" for setting \"{}\": {}",
              formatString, settingId, err.what());
    return false;
  }

  return true;
}

CGUIControlRangeSetting::CGUIControlRangeSetting(CGUISettingsSliderControl* pSlider,
                                                 int id,
                                                 std::shared_ptr<CSetting> pSetting,
                                                 ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pSlider = pSlider;
  if (m_pSlider == NULL)
    return;

  m_pSlider->SetID(id);
  m_pSlider->SetRangeSelection(true);

  if (m_pSetting->GetType() == SettingType::List)
  {
    std::shared_ptr<CSettingList> settingList = std::static_pointer_cast<CSettingList>(m_pSetting);
    SettingConstPtr listDefintion = settingList->GetDefinition();
    switch (listDefintion->GetType())
    {
      case SettingType::Integer:
      {
        std::shared_ptr<const CSettingInt> listDefintionInt =
            std::static_pointer_cast<const CSettingInt>(listDefintion);
        if (m_pSetting->GetControl()->GetFormat() == "percentage")
          m_pSlider->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
        else
        {
          m_pSlider->SetType(SLIDER_CONTROL_TYPE_INT);
          m_pSlider->SetRange(listDefintionInt->GetMinimum(), listDefintionInt->GetMaximum());
        }
        m_pSlider->SetIntInterval(listDefintionInt->GetStep());
        break;
      }

      case SettingType::Number:
      {
        std::shared_ptr<const CSettingNumber> listDefinitionNumber =
            std::static_pointer_cast<const CSettingNumber>(listDefintion);
        m_pSlider->SetType(SLIDER_CONTROL_TYPE_FLOAT);
        m_pSlider->SetFloatRange(static_cast<float>(listDefinitionNumber->GetMinimum()),
                                 static_cast<float>(listDefinitionNumber->GetMaximum()));
        m_pSlider->SetFloatInterval(static_cast<float>(listDefinitionNumber->GetStep()));
        break;
      }

      default:
        break;
    }
  }
}

CGUIControlRangeSetting::~CGUIControlRangeSetting() = default;

bool CGUIControlRangeSetting::OnClick()
{
  if (m_pSlider == NULL || m_pSetting->GetType() != SettingType::List)
    return false;

  std::shared_ptr<CSettingList> settingList = std::static_pointer_cast<CSettingList>(m_pSetting);
  const SettingList& settingListValues = settingList->GetValue();
  if (settingListValues.size() != 2)
    return false;

  std::vector<CVariant> values;
  SettingConstPtr listDefintion = settingList->GetDefinition();
  switch (listDefintion->GetType())
  {
    case SettingType::Integer:
      values.emplace_back(m_pSlider->GetIntValue(CGUISliderControl::RangeSelectorLower));
      values.emplace_back(m_pSlider->GetIntValue(CGUISliderControl::RangeSelectorUpper));
      break;

    case SettingType::Number:
      values.emplace_back(m_pSlider->GetFloatValue(CGUISliderControl::RangeSelectorLower));
      values.emplace_back(m_pSlider->GetFloatValue(CGUISliderControl::RangeSelectorUpper));
      break;

    default:
      return false;
  }

  if (values.size() != 2)
    return false;

  SetValid(CSettingUtils::SetList(settingList, values));
  return IsValid();
}

void CGUIControlRangeSetting::Update(bool fromControl, bool updateDisplayOnly)
{
  if (m_pSlider == NULL || m_pSetting->GetType() != SettingType::List)
    return;

  CGUIControlBaseSetting::Update(fromControl, updateDisplayOnly);

  std::shared_ptr<CSettingList> settingList = std::static_pointer_cast<CSettingList>(m_pSetting);
  const SettingList& settingListValues = settingList->GetValue();
  if (settingListValues.size() != 2)
    return;

  SettingConstPtr listDefintion = settingList->GetDefinition();
  std::shared_ptr<const CSettingControlRange> controlRange =
      std::static_pointer_cast<const CSettingControlRange>(m_pSetting->GetControl());
  const std::string& controlFormat = controlRange->GetFormat();

  std::string strText;
  std::string strTextLower, strTextUpper;
  std::string formatString =
      Localize(controlRange->GetFormatLabel() > -1 ? controlRange->GetFormatLabel() : 21469);
  std::string valueFormat = controlRange->GetValueFormat();
  if (controlRange->GetValueFormatLabel() > -1)
    valueFormat = Localize(controlRange->GetValueFormatLabel());

  switch (listDefintion->GetType())
  {
    case SettingType::Integer:
    {
      int valueLower, valueUpper;
      if (fromControl)
      {
        valueLower = m_pSlider->GetIntValue(CGUISliderControl::RangeSelectorLower);
        valueUpper = m_pSlider->GetIntValue(CGUISliderControl::RangeSelectorUpper);
      }
      else
      {
        valueLower = std::static_pointer_cast<CSettingInt>(settingListValues[0])->GetValue();
        valueUpper = std::static_pointer_cast<CSettingInt>(settingListValues[1])->GetValue();
        m_pSlider->SetIntValue(valueLower, CGUISliderControl::RangeSelectorLower);
        m_pSlider->SetIntValue(valueUpper, CGUISliderControl::RangeSelectorUpper);
      }

      if (controlFormat == "date" || controlFormat == "time")
      {
        CDateTime dateLower((time_t)valueLower);
        CDateTime dateUpper((time_t)valueUpper);

        if (controlFormat == "date")
        {
          if (valueFormat.empty())
          {
            strTextLower = dateLower.GetAsLocalizedDate();
            strTextUpper = dateUpper.GetAsLocalizedDate();
          }
          else
          {
            strTextLower = dateLower.GetAsLocalizedDate(valueFormat);
            strTextUpper = dateUpper.GetAsLocalizedDate(valueFormat);
          }
        }
        else
        {
          if (valueFormat.empty())
            valueFormat = "mm:ss";

          strTextLower = dateLower.GetAsLocalizedTime(valueFormat);
          strTextUpper = dateUpper.GetAsLocalizedTime(valueFormat);
        }
      }
      else
      {
        strTextLower = StringUtils::Format(valueFormat, valueLower);
        strTextUpper = StringUtils::Format(valueFormat, valueUpper);
      }

      if (valueLower != valueUpper)
        strText = StringUtils::Format(formatString, strTextLower, strTextUpper);
      else
        strText = strTextLower;
      break;
    }

    case SettingType::Number:
    {
      double valueLower, valueUpper;
      if (fromControl)
      {
        valueLower =
            static_cast<double>(m_pSlider->GetFloatValue(CGUISliderControl::RangeSelectorLower));
        valueUpper =
            static_cast<double>(m_pSlider->GetFloatValue(CGUISliderControl::RangeSelectorUpper));
      }
      else
      {
        valueLower = std::static_pointer_cast<CSettingNumber>(settingListValues[0])->GetValue();
        valueUpper = std::static_pointer_cast<CSettingNumber>(settingListValues[1])->GetValue();
        m_pSlider->SetFloatValue((float)valueLower, CGUISliderControl::RangeSelectorLower);
        m_pSlider->SetFloatValue((float)valueUpper, CGUISliderControl::RangeSelectorUpper);
      }

      strTextLower = StringUtils::Format(valueFormat, valueLower);
      if (valueLower != valueUpper)
      {
        strTextUpper = StringUtils::Format(valueFormat, valueUpper);
        strText = StringUtils::Format(formatString, strTextLower, strTextUpper);
      }
      else
        strText = strTextLower;
      break;
    }

    default:
      strText.clear();
      break;
  }

  if (!strText.empty())
    m_pSlider->SetTextValue(strText);
}

CGUIControlSeparatorSetting::CGUIControlSeparatorSetting(CGUIImage* pImage,
                                                         int id,
                                                         ILocalizer* localizer)
  : CGUIControlBaseSetting(id, NULL, localizer)
{
  m_pImage = pImage;
  if (m_pImage == NULL)
    return;

  m_pImage->SetID(id);
}

CGUIControlSeparatorSetting::~CGUIControlSeparatorSetting() = default;

CGUIControlGroupTitleSetting::CGUIControlGroupTitleSetting(CGUILabelControl* pLabel,
                                                           int id,
                                                           ILocalizer* localizer)
  : CGUIControlBaseSetting(id, NULL, localizer)
{
  m_pLabel = pLabel;
  if (m_pLabel == NULL)
    return;

  m_pLabel->SetID(id);
}

CGUIControlGroupTitleSetting::~CGUIControlGroupTitleSetting() = default;

CGUIControlLabelSetting::CGUIControlLabelSetting(CGUIButtonControl* pButton,
                                                 int id,
                                                 std::shared_ptr<CSetting> pSetting,
                                                 ILocalizer* localizer)
  : CGUIControlBaseSetting(id, std::move(pSetting), localizer)
{
  m_pButton = pButton;
  if (m_pButton == NULL)
    return;

  m_pButton->SetID(id);
  UpdateFromSetting();
}
