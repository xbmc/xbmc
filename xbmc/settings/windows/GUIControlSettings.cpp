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

#include "GUIControlSettings.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/LocalizeStrings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Setting.h"
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
  if (control != NULL)
    control->SetEnabled(IsEnabled());
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

  switch (m_pSetting->GetControl().GetFormat())
  {
    case SettingControlFormatNumber:
    {
      CSettingNumber *pSettingNumber = (CSettingNumber *)m_pSetting;
      m_pSpin->SetType(SPIN_CONTROL_TYPE_FLOAT);
      m_pSpin->SetFloatRange((float)pSettingNumber->GetMinimum(), (float)pSettingNumber->GetMaximum());
      m_pSpin->SetFloatInterval((float)pSettingNumber->GetStep());

      m_pSpin->SetFloatValue((float)pSettingNumber->GetValue());
      break;
    }

    case SettingControlFormatInteger:
    {
      m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
      FillIntegerSettingControl();
      break;
    }

    case SettingControlFormatString:
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
          for (std::vector< std::pair<std::string, std::string> >::const_iterator option = options.begin(); option != options.end(); option++)
            m_pSpin->AddLabel(option->first, option->second);

          m_pSpin->SetStringValue(pSettingString->GetValue());
        }
      }
      break;
    }

    default:
      break;
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
      for (StaticIntegerSettingOptions::const_iterator it = options.begin(); it != options.end(); it++)
        m_pSpin->AddLabel(g_localizeStrings.Get(it->first), it->second);

      break;
    }

    case SettingOptionsTypeDynamic:
    {
      DynamicIntegerSettingOptions options = pSettingInt->UpdateDynamicOptions();
      for (DynamicIntegerSettingOptions::const_iterator option = options.begin(); option != options.end(); option++)
        m_pSpin->AddLabel(option->first, option->second);

      break;
    }

    case SettingOptionsTypeNone:
    default:
    {
      std::string strLabel;
      int i = pSettingInt->GetMinimum();
      if (pSettingInt->GetMinimumLabel() > -1)
      {
        strLabel = g_localizeStrings.Get(pSettingInt->GetMinimumLabel());
        m_pSpin->AddLabel(strLabel, pSettingInt->GetMinimum());
        i += pSettingInt->GetStep();
      }
      for (; i <= pSettingInt->GetMaximum(); i += pSettingInt->GetStep())
      {
        if (pSettingInt->GetFormat() > -1)
          strLabel = StringUtils::Format(g_localizeStrings.Get(pSettingInt->GetFormat()).c_str(), i);
        else
          strLabel = StringUtils::Format(pSettingInt->GetFormatString().c_str(), i);
        m_pSpin->AddLabel(strLabel, i);
      }

      break;
    }
  }

  m_pSpin->SetValue(pSettingInt->GetValue());
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

  bool success = false;
  switch (m_pSetting->GetControl().GetFormat())
  {
    case SettingControlFormatAddon:
    {
      // prompt for the addon
      CSettingAddon *setting = (CSettingAddon *)m_pSetting;
      CStdString addonID = setting->GetValue();
      if (!CGUIWindowAddonBrowser::SelectAddonID(setting->GetAddonType(), addonID, setting->AllowEmpty()) == 1)
        return false;

      success = setting->SetValue(addonID);
      break;
    }

    case SettingControlFormatPath:
    {
      success = GetPath((CSettingPath *)m_pSetting);
      break;
    }

    case SettingControlFormatAction:
    {
      // simply call the OnSettingAction callback and whoever knows what to
      // do can do so (based on the setting's identification
      CSettingAction *pSettingAction = (CSettingAction *)m_pSetting;
      pSettingAction->OnSettingAction(pSettingAction);
      success = true;
      break;
    }

    default:
      return false;
  }

  return success;
}

void CGUIControlButtonSetting::Update()
{
  if (m_pButton == NULL)
    return;

  CGUIControlBaseSetting::Update();

  if (m_pSetting->GetType() == SettingTypeString)
  {
    std::string strText = ((CSettingString *)m_pSetting)->GetValue();
    switch (m_pSetting->GetControl().GetFormat())
    {
      case SettingControlFormatAddon:
      {
        ADDON::AddonPtr addon;
        if (ADDON::CAddonMgr::Get().GetAddon(strText, addon))
          strText = addon->Name();
        if (strText.empty())
          strText = g_localizeStrings.Get(231); // None
        break;
      }

      case SettingControlFormatPath:
      {
        CStdString shortPath;
        if (CUtil::MakeShortenPath(strText, shortPath, 30))
          strText = shortPath;
        break;
      }

      default:
        return;
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
  for (std::vector<std::string>::const_iterator source = sources.begin(); source != sources.end(); source++)
  {
    VECSOURCES *sources = CMediaSourceSettings::Get().GetSources(*source);
    if (sources != NULL)
      shares.insert(shares.end(), sources->begin(), sources->end());
  }

  g_mediaManager.GetNetworkLocations(shares);
  g_mediaManager.GetLocalDrives(shares);

  if (!CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pathSetting->GetHeading()), path, pathSetting->Writable()))
    return false;

  return pathSetting->SetValue(path);
}

CGUIControlEditSetting::CGUIControlEditSetting(CGUIEditControl *pEdit, int id, CSetting *pSetting)
  : CGUIControlBaseSetting(id, pSetting)
{
  m_pEdit = pEdit;
  m_pEdit->SetID(id);
  int heading = m_pSetting->GetLabel();
  if (m_pSetting->GetType() == SettingTypeString)
  {
    CSettingString *pSettingString = (CSettingString *)m_pSetting;
    if (pSettingString->GetHeading() > 0)
      heading = pSettingString->GetHeading();
  }
  if (heading < 0)
    heading = 0;

  CGUIEditControl::INPUT_TYPE inputType = CGUIEditControl::INPUT_TYPE_TEXT;
  const CSettingControl& control = pSetting->GetControl();
  switch (control.GetFormat())
  {
    case SettingControlFormatString:
      if (control.GetAttributes() & SettingControlAttributeHidden)
        inputType = CGUIEditControl::INPUT_TYPE_PASSWORD;
      break;
      
    case SettingControlFormatInteger:
      if (control.GetAttributes() & SettingControlAttributeVerifyNew)
        inputType = CGUIEditControl::INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW;
      else
        inputType = CGUIEditControl::INPUT_TYPE_NUMBER;
      break;
      
    case SettingControlFormatIP:
      inputType = CGUIEditControl::INPUT_TYPE_IPADDRESS;
      break;
      
    case SettingControlFormatMD5:
      inputType = CGUIEditControl::INPUT_TYPE_PASSWORD_MD5;
      break;

    default:
      break;
  }
  m_pEdit->SetInputType(inputType, heading);

  Update();
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

CGUIControlSeparatorSetting::CGUIControlSeparatorSetting(CGUIImage *pImage, int id)
    : CGUIControlBaseSetting(id, NULL)
{
  m_pImage = pImage;
  m_pImage->SetID(id);
}

CGUIControlSeparatorSetting::~CGUIControlSeparatorSetting()
{ }
