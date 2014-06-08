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

#include <set>

#include "GUIControlSettings.h"
#include "FileItem.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/lib/Setting.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"

using namespace ADDON;

CGUIControlBaseSetting::CGUIControlBaseSetting(int id, CSetting *pSetting)
{
  m_id = id;
  m_pSetting = pSetting;
  m_delayed = false;
}

bool CGUIControlBaseSetting::IsEnabled() const
{
  return m_pSetting != NULL && m_pSetting->IsEnabled();
}

void CGUIControlBaseSetting::Update()
{
  CGUIControl *control = GetControl();
  if (control == NULL)
    return;

  control->SetEnabled(IsEnabled());
  if (m_pSetting)
    control->SetVisible(m_pSetting->IsVisible());
}

CGUIControlRadioButtonSetting::CGUIControlRadioButtonSetting(CGUIRadioButtonControl *pRadioButton, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  m_pRadioButton = pRadioButton;
  m_pRadioButton->SetID(id);
  Update();
}

CGUIControlRadioButtonSetting::~CGUIControlRadioButtonSetting()
{ }

bool CGUIControlRadioButtonSetting::OnClick()
{
  return ((CSettingBool *)m_pSetting)->SetValue(!((CSettingBool *)m_pSetting)->GetValue());
}

void CGUIControlRadioButtonSetting::Update()
{
  if (m_pRadioButton == NULL)
    return;

  CGUIControlBaseSetting::Update();

  m_pRadioButton->SetSelected(((CSettingBool *)m_pSetting)->GetValue());
}

CGUIControlSpinExSetting::CGUIControlSpinExSetting(CGUISpinControlEx *pSpin, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  m_pSpin = pSpin;
  m_pSpin->SetID(id);
  
  FillControl();
}

CGUIControlSpinExSetting::~CGUIControlSpinExSetting()
{ }

bool CGUIControlSpinExSetting::OnClick()
{
  if (m_pSpin == NULL)
    return false;

  switch (m_pSetting->GetType())
  {
    case SettingTypeInteger:
      return ((CSettingInt *)m_pSetting)->SetValue(m_pSpin->GetValue());
      break;

    case SettingTypeNumber:
      return ((CSettingNumber *)m_pSetting)->SetValue(m_pSpin->GetFloatValue());
    
    case SettingTypeString:
      return ((CSettingString *)m_pSetting)->SetValue(m_pSpin->GetStringValue());
    
    default:
      break;
  }
  
  return false;
}

void CGUIControlSpinExSetting::Update()
{
  if (m_pSpin == NULL)
    return;

  CGUIControlBaseSetting::Update();

  FillControl();

  // disable the spinner if it has less than two items
  if (!m_pSpin->IsDisabled() && (m_pSpin->GetMaximum() - m_pSpin->GetMinimum()) == 0)
    m_pSpin->SetEnabled(false);
}

void CGUIControlSpinExSetting::FillControl()
{
  m_pSpin->Clear();

  const std::string &controlFormat = m_pSetting->GetControl()->GetFormat();
  if (controlFormat == "number")
  {
    CSettingNumber *pSettingNumber = (CSettingNumber *)m_pSetting;
    m_pSpin->SetType(SPIN_CONTROL_TYPE_FLOAT);
    m_pSpin->SetFloatRange((float)pSettingNumber->GetMinimum(), (float)pSettingNumber->GetMaximum());
    m_pSpin->SetFloatInterval((float)pSettingNumber->GetStep());

    m_pSpin->SetFloatValue((float)pSettingNumber->GetValue());
  }
  else if (controlFormat == "integer")
  {
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
    FillIntegerSettingControl();
  }
  else if (controlFormat == "string")
  {
    m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);

    if (m_pSetting->GetType() == SettingTypeInteger)
      FillIntegerSettingControl();
    else if (m_pSetting->GetType() == SettingTypeString)
    {
      CSettingString *pSettingString = (CSettingString *)m_pSetting;
      if (pSettingString->GetOptionsType() == SettingOptionsTypeDynamic)
      {
        DynamicStringSettingOptions options = pSettingString->UpdateDynamicOptions();
        for (std::vector< std::pair<std::string, std::string> >::const_iterator option = options.begin(); option != options.end(); ++option)
          m_pSpin->AddLabel(option->first, option->second);

        m_pSpin->SetStringValue(pSettingString->GetValue());
      }
    }
  }
}

void CGUIControlSpinExSetting::FillIntegerSettingControl()
{
  CSettingInt *pSettingInt = (CSettingInt *)m_pSetting;
  switch (pSettingInt->GetOptionsType())
  {
    case SettingOptionsTypeStatic:
    {
      const StaticIntegerSettingOptions& options = pSettingInt->GetOptions();
      for (StaticIntegerSettingOptions::const_iterator it = options.begin(); it != options.end(); ++it)
        m_pSpin->AddLabel(g_localizeStrings.Get(it->first), it->second);

      break;
    }

    case SettingOptionsTypeDynamic:
    {
      DynamicIntegerSettingOptions options = pSettingInt->UpdateDynamicOptions();
      for (DynamicIntegerSettingOptions::const_iterator option = options.begin(); option != options.end(); ++option)
        m_pSpin->AddLabel(option->first, option->second);

      break;
    }

    case SettingOptionsTypeNone:
    default:
    {
      std::string strLabel;
      int i = pSettingInt->GetMinimum();
      const CSettingControlSpinner *control = static_cast<const CSettingControlSpinner*>(pSettingInt->GetControl());
      if (control->GetMinimumLabel() > -1)
      {
        strLabel = g_localizeStrings.Get(control->GetMinimumLabel());
        m_pSpin->AddLabel(strLabel, pSettingInt->GetMinimum());
        i += pSettingInt->GetStep();
      }

      for (; i <= pSettingInt->GetMaximum(); i += pSettingInt->GetStep())
      {
        if (control->GetFormatLabel() > -1)
          strLabel = StringUtils::Format(g_localizeStrings.Get(control->GetFormatLabel()).c_str(), i);
        else
          strLabel = StringUtils::Format(control->GetFormatString().c_str(), i);
        m_pSpin->AddLabel(strLabel, i);
      }

      break;
    }
  }

  m_pSpin->SetValue(pSettingInt->GetValue());
}

CGUIControlListSetting::CGUIControlListSetting(CGUIButtonControl *pButton, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  m_pButton = pButton;
  m_pButton->SetID(id);
  Update();
}

CGUIControlListSetting::~CGUIControlListSetting()
{ }

bool CGUIControlListSetting::OnClick()
{
  if (m_pButton == NULL)
    return false;

  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (dialog == NULL)
    return false;

  CFileItemList options;
  if (!GetItems(m_pSetting, options) || options.Size() <= 1)
    return false;

  const CSettingControlList *control = static_cast<const CSettingControlList*>(m_pSetting->GetControl());
  
  dialog->Reset();
  dialog->SetHeading(g_localizeStrings.Get(m_pSetting->GetLabel()));
  dialog->SetItems(&options);
  dialog->SetMultiSelection(control->CanMultiSelect());
  dialog->DoModal();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemList &items = dialog->GetSelectedItems();
  std::vector<CVariant> values;
  for (int index = 0; index < items.Size(); index++)
  {
    const CFileItemPtr item = items[index];
    if (item == NULL || !item->HasProperty("value"))
      return false;

    values.push_back(item->GetProperty("value"));
  }

  bool ret = false;
  switch (m_pSetting->GetType())
  {
    case SettingTypeInteger:
      if (values.size() > 1)
        return false;
      ret = ((CSettingInt *)m_pSetting)->SetValue((int)values.at(0).asInteger());
      break;

    case SettingTypeString:
      if (values.size() > 1)
        return false;
      ret = ((CSettingString *)m_pSetting)->SetValue(values.at(0).asString());
      break;

    case SettingTypeList:
      ret = CSettings::Get().SetList(m_pSetting->GetId(), values);
      break;
    
    default:
      break;
  }

  if (ret)
    Update();

  return ret;
}

void CGUIControlListSetting::Update()
{
  if (m_pButton == NULL)
    return;

  CGUIControlBaseSetting::Update();
  
  CFileItemList options;
  if (GetItems(m_pSetting, options))
  {
    std::vector<std::string> labels;
    for (int index = 0; index < options.Size(); index++)
    {
      const CFileItemPtr pItem = options.Get(index);
      if (pItem->IsSelected())
        labels.push_back(pItem->GetLabel());
    }

    m_pButton->SetLabel2(StringUtils::Join(labels, ", "));
  }

  // disable the control if it has less than two items
  if (!m_pButton->IsDisabled() && options.Size() <= 1)
    m_pButton->SetEnabled(false);
}

static CFileItemPtr GetItem(const std::string &label, const CVariant &value)
{
  CFileItemPtr pItem(new CFileItem(label));
  pItem->SetProperty("value", value);

  return pItem;
}

bool CGUIControlListSetting::GetItems(const CSetting *setting, CFileItemList &items)
{
  const CSettingControlList *control = static_cast<const CSettingControlList*>(setting->GetControl());
  const std::string &controlFormat = control->GetFormat();

  if (controlFormat == "integer")
    return GetIntegerItems(setting, items);
  else if (controlFormat == "string")
  {
    if (setting->GetType() == SettingTypeInteger ||
       (setting->GetType() == SettingTypeList && ((CSettingList *)setting)->GetElementType() == SettingTypeInteger))
      return GetIntegerItems(setting, items);
    else if (setting->GetType() == SettingTypeString ||
            (setting->GetType() == SettingTypeList && ((CSettingList *)setting)->GetElementType() == SettingTypeString))
      return GetStringItems(setting, items);
  }
  else
    return false;

  return true;
}

bool CGUIControlListSetting::GetIntegerItems(const CSetting *setting, CFileItemList &items)
{
  const CSettingInt *pSettingInt = NULL;
  std::set<int> values;
  if (setting->GetType() == SettingTypeInteger)
  {
    pSettingInt = static_cast<const CSettingInt*>(setting);
    values.insert(pSettingInt->GetValue());
  }
  else if (setting->GetType() == SettingTypeList)
  {
    const CSettingList *settingList = static_cast<const CSettingList*>(setting);
    if (settingList->GetElementType() != SettingTypeInteger)
      return false;

    pSettingInt = static_cast<const CSettingInt*>(settingList->GetDefinition());
    std::vector<CVariant> list = CSettings::Get().GetList(settingList->GetId());
    for (std::vector<CVariant>::const_iterator itValue = list.begin(); itValue != list.end(); ++itValue)
    {
      if (!itValue->isInteger())
        return false;
      values.insert(itValue->asInteger());
    }
  }
  else
    return false;

  switch (pSettingInt->GetOptionsType())
  {
    case SettingOptionsTypeStatic:
    {
      const StaticIntegerSettingOptions& options = pSettingInt->GetOptions();
      for (StaticIntegerSettingOptions::const_iterator it = options.begin(); it != options.end(); ++it)
      {
        CFileItemPtr pItem = GetItem(g_localizeStrings.Get(it->first), it->second);

        if (values.find(it->second) != values.end())
          pItem->Select(true);

        items.Add(pItem);
      }
      break;
    }

    case SettingOptionsTypeDynamic:
    {
      DynamicIntegerSettingOptions options = const_cast<CSettingInt*>(pSettingInt)->UpdateDynamicOptions();
      for (DynamicIntegerSettingOptions::const_iterator option = options.begin(); option != options.end(); ++option)
      {
        CFileItemPtr pItem = GetItem(option->first, option->second);

        if (values.find(option->second) != values.end())
          pItem->Select(true);

        items.Add(pItem);
      }
      break;
    }

    case SettingOptionsTypeNone:
    default:
      return false;
  }

  return true;
}

bool CGUIControlListSetting::GetStringItems(const CSetting *setting, CFileItemList &items)
{
  const CSettingString *pSettingString = NULL;
  std::set<std::string> values;
  if (setting->GetType() == SettingTypeString)
  {
    pSettingString = static_cast<const CSettingString*>(setting);
    values.insert(pSettingString->GetValue());
  }
  else if (setting->GetType() == SettingTypeList)
  {
    const CSettingList *settingList = static_cast<const CSettingList*>(setting);
    if (settingList->GetElementType() != SettingTypeString)
      return false;

    pSettingString = static_cast<const CSettingString*>(settingList->GetDefinition());
    std::vector<CVariant> list = CSettings::Get().GetList(settingList->GetId());
    for (std::vector<CVariant>::const_iterator itValue = list.begin(); itValue != list.end(); ++itValue)
    {
      if (!itValue->isString())
        return false;
      values.insert(itValue->asString());
    }
  }
  else
    return false;

  if (pSettingString->GetOptionsType() == SettingOptionsTypeDynamic)
  {
    DynamicStringSettingOptions options = const_cast<CSettingString*>(pSettingString)->UpdateDynamicOptions();
    for (DynamicStringSettingOptions::const_iterator option = options.begin(); option != options.end(); ++option)
    {
      CFileItemPtr pItem = GetItem(option->first, option->second);

      if (values.find(option->second) != values.end())
        pItem->Select(true);

      items.Add(pItem);
    }
  }
  else
    return false;

  return true;
}

CGUIControlButtonSetting::CGUIControlButtonSetting(CGUIButtonControl *pButton, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  m_pButton = pButton;
  m_pButton->SetID(id);
  Update();
}

CGUIControlButtonSetting::~CGUIControlButtonSetting()
{ }

bool CGUIControlButtonSetting::OnClick()
{
  if (m_pButton == NULL)
    return false;

  const std::string &controlFormat = m_pSetting->GetControl()->GetFormat();
  if (controlFormat == "addon")
  {
    // prompt for the addon
    CSettingAddon *setting = (CSettingAddon *)m_pSetting;
    CStdString addonID = setting->GetValue();
    if (CGUIWindowAddonBrowser::SelectAddonID(setting->GetAddonType(), addonID, setting->AllowEmpty()) != 1)
      return false;

    return setting->SetValue(addonID);
  }
  if (controlFormat == "path")
    return GetPath((CSettingPath *)m_pSetting);
  if (controlFormat == "action")
  {
    // simply call the OnSettingAction callback and whoever knows what to
    // do can do so (based on the setting's identification
    CSettingAction *pSettingAction = (CSettingAction *)m_pSetting;
    pSettingAction->OnSettingAction(pSettingAction);
    return true;
  }

  return false;
}

void CGUIControlButtonSetting::Update()
{
  if (m_pButton == NULL)
    return;

  CGUIControlBaseSetting::Update();

  if (m_pSetting->GetType() == SettingTypeString &&
      !static_cast<const CSettingControlButton*>(m_pSetting->GetControl())->HideValue())
  {
    std::string strText = ((CSettingString *)m_pSetting)->GetValue();
    const std::string &controlFormat = m_pSetting->GetControl()->GetFormat();
    if (controlFormat == "addon")
    {
      ADDON::AddonPtr addon;
      if (ADDON::CAddonMgr::Get().GetAddon(strText, addon))
        strText = addon->Name();
      if (strText.empty())
        strText = g_localizeStrings.Get(231); // None
    }
    else if (controlFormat == "path")
    {
      CStdString shortPath;
      if (CUtil::MakeShortenPath(strText, shortPath, 30))
        strText = shortPath;
    }

    m_pButton->SetLabel2(strText);
  }
}

bool CGUIControlButtonSetting::GetPath(CSettingPath *pathSetting)
{
  if (pathSetting == NULL)
    return false;

  CStdString path = pathSetting->GetValue();

  VECSOURCES shares;
  const std::vector<std::string>& sources = pathSetting->GetSources();
  for (std::vector<std::string>::const_iterator source = sources.begin(); source != sources.end(); ++source)
  {
    VECSOURCES *sources = CMediaSourceSettings::Get().GetSources(*source);
    if (sources != NULL)
      shares.insert(shares.end(), sources->begin(), sources->end());
  }

  g_mediaManager.GetNetworkLocations(shares);
  g_mediaManager.GetLocalDrives(shares);

  if (!CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(static_cast<const CSettingControlButton*>(pathSetting->GetControl())->GetHeading()), path, pathSetting->Writable()))
    return false;

  return pathSetting->SetValue(path);
}

CGUIControlEditSetting::CGUIControlEditSetting(CGUIEditControl *pEdit, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  const CSettingControlEdit* control = static_cast<const CSettingControlEdit*>(pSetting->GetControl());
  m_pEdit = pEdit;
  m_pEdit->SetID(id);
  int heading = m_pSetting->GetLabel();
  if (control->GetHeading() > 0)
    heading = control->GetHeading();
  if (heading < 0)
    heading = 0;

  CGUIEditControl::INPUT_TYPE inputType = CGUIEditControl::INPUT_TYPE_TEXT;
  const std::string &controlFormat = control->GetFormat();
  if (controlFormat == "string")
  {
    if (control->IsHidden())
      inputType = CGUIEditControl::INPUT_TYPE_PASSWORD;
  }
  else if (controlFormat == "integer")
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

  Update();

  // this will automatically trigger validation so it must be executed after
  // having set the value of the control based on the value of the setting
  m_pEdit->SetInputValidation(InputValidation, this);
}

CGUIControlEditSetting::~CGUIControlEditSetting()
{ }

bool CGUIControlEditSetting::OnClick()
{
  if (m_pEdit == NULL)
    return false;

  // update our string
  return m_pSetting->FromString(m_pEdit->GetLabel2());
}

void CGUIControlEditSetting::Update()
{
  if (m_pEdit == NULL)
    return;

  CGUIControlBaseSetting::Update();

  m_pEdit->SetLabel2(m_pSetting->ToString());
}

bool CGUIControlEditSetting::InputValidation(const std::string &input, void *data)
{
  if (data == NULL)
    return true;

  CGUIControlEditSetting *editControl = reinterpret_cast<CGUIControlEditSetting*>(data);
  if (editControl == NULL || editControl->GetSetting() == NULL)
    return true;

  return editControl->GetSetting()->CheckValidity(input);
}

CGUIControlSeparatorSetting::CGUIControlSeparatorSetting(CGUIImage *pImage, int id)
    : CGUIControlBaseSetting(id, NULL)
{
  m_pImage = pImage;
  m_pImage->SetID(id);
}

CGUIControlSeparatorSetting::~CGUIControlSeparatorSetting()
{ }
