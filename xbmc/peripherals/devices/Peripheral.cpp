/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Peripheral.h"

#include "Util.h"
#include "XBDateTime.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "guilib/LocalizeStrings.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/keyboard/generic/DefaultKeyboardHandling.h"
#include "input/mouse/generic/DefaultMouseHandling.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonButtonMapping.h"
#include "peripherals/addons/AddonInputHandling.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/bus/PeripheralBus.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "settings/SettingAddon.h"
#include "settings/lib/Setting.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

struct SortBySettingsOrder
{
  bool operator()(const PeripheralDeviceSetting& left, const PeripheralDeviceSetting& right)
  {
    return left.m_order < right.m_order;
  }
};

CPeripheral::CPeripheral(CPeripherals& manager,
                         const PeripheralScanResult& scanResult,
                         CPeripheralBus* bus)
  : m_manager(manager),
    m_type(scanResult.m_mappedType),
    m_busType(scanResult.m_busType),
    m_mappedBusType(scanResult.m_mappedBusType),
    m_strLocation(scanResult.m_strLocation),
    m_strDeviceName(scanResult.m_strDeviceName),
    m_iVendorId(scanResult.m_iVendorId),
    m_iProductId(scanResult.m_iProductId),
    m_strVersionInfo(g_localizeStrings.Get(13205)), // "unknown"
    m_bus(bus)
{
  PeripheralTypeTranslator::FormatHexString(scanResult.m_iVendorId, m_strVendorId);
  PeripheralTypeTranslator::FormatHexString(scanResult.m_iProductId, m_strProductId);
  if (scanResult.m_iSequence > 0)
  {
    m_strFileLocation =
        StringUtils::Format("peripherals://{}/{}_{}.dev",
                            PeripheralTypeTranslator::BusTypeToString(scanResult.m_busType),
                            scanResult.m_strLocation, scanResult.m_iSequence);
  }
  else
  {
    m_strFileLocation = StringUtils::Format(
        "peripherals://{}/{}.dev", PeripheralTypeTranslator::BusTypeToString(scanResult.m_busType),
        scanResult.m_strLocation);
  }
}

CPeripheral::~CPeripheral(void)
{
  PersistSettings(true);

  m_subDevices.clear();

  ClearSettings();
}

bool CPeripheral::operator==(const CPeripheral& right) const
{
  return m_type == right.m_type && m_strLocation == right.m_strLocation &&
         m_iVendorId == right.m_iVendorId && m_iProductId == right.m_iProductId;
}

bool CPeripheral::operator!=(const CPeripheral& right) const
{
  return !(*this == right);
}

bool CPeripheral::HasFeature(const PeripheralFeature feature) const
{
  bool bReturn(false);

  for (unsigned int iFeaturePtr = 0; iFeaturePtr < m_features.size(); iFeaturePtr++)
  {
    if (m_features.at(iFeaturePtr) == feature)
    {
      bReturn = true;
      break;
    }
  }

  if (!bReturn)
  {
    for (unsigned int iSubdevicePtr = 0; iSubdevicePtr < m_subDevices.size(); iSubdevicePtr++)
    {
      if (m_subDevices.at(iSubdevicePtr)->HasFeature(feature))
      {
        bReturn = true;
        break;
      }
    }
  }

  return bReturn;
}

void CPeripheral::GetFeatures(std::vector<PeripheralFeature>& features) const
{
  for (unsigned int iFeaturePtr = 0; iFeaturePtr < m_features.size(); iFeaturePtr++)
    features.push_back(m_features.at(iFeaturePtr));

  for (unsigned int iSubdevicePtr = 0; iSubdevicePtr < m_subDevices.size(); iSubdevicePtr++)
    m_subDevices.at(iSubdevicePtr)->GetFeatures(features);
}

bool CPeripheral::Initialise(void)
{
  bool bReturn(false);

  if (m_bError)
    return bReturn;

  bReturn = true;
  if (m_bInitialised)
    return bReturn;

  m_manager.GetSettingsFromMapping(*this);

  std::string safeDeviceName = m_strDeviceName;
  StringUtils::Replace(safeDeviceName, ' ', '_');

  if (m_iVendorId == 0x0000 && m_iProductId == 0x0000)
  {
    m_strSettingsFile = StringUtils::Format(
        "special://profile/peripheral_data/{}_{}.xml",
        PeripheralTypeTranslator::BusTypeToString(m_mappedBusType),
        CUtil::MakeLegalFileName(std::move(safeDeviceName), LEGAL_WIN32_COMPAT));
  }
  else
  {
    // Backwards compatibility - old settings files didn't include the device name
    m_strSettingsFile = StringUtils::Format(
        "special://profile/peripheral_data/{}_{}_{}.xml",
        PeripheralTypeTranslator::BusTypeToString(m_mappedBusType), m_strVendorId, m_strProductId);

    if (!CFileUtils::Exists(m_strSettingsFile))
      m_strSettingsFile = StringUtils::Format(
          "special://profile/peripheral_data/{}_{}_{}_{}.xml",
          PeripheralTypeTranslator::BusTypeToString(m_mappedBusType), m_strVendorId, m_strProductId,
          CUtil::MakeLegalFileName(std::move(safeDeviceName), LEGAL_WIN32_COMPAT));
  }

  LoadPersistedSettings();

  for (unsigned int iFeaturePtr = 0; iFeaturePtr < m_features.size(); iFeaturePtr++)
  {
    PeripheralFeature feature = m_features.at(iFeaturePtr);
    bReturn &= InitialiseFeature(feature);
  }

  for (unsigned int iSubdevicePtr = 0; iSubdevicePtr < m_subDevices.size(); iSubdevicePtr++)
    bReturn &= m_subDevices.at(iSubdevicePtr)->Initialise();

  if (bReturn)
  {
    CLog::Log(LOGDEBUG, "{} - initialised peripheral on '{}' with {} features and {} sub devices",
              __FUNCTION__, m_strLocation, (int)m_features.size(), (int)m_subDevices.size());
    m_bInitialised = true;
  }

  return bReturn;
}

void CPeripheral::GetSubdevices(PeripheralVector& subDevices) const
{
  subDevices = m_subDevices;
}

bool CPeripheral::IsMultiFunctional(void) const
{
  return m_subDevices.size() > 0;
}

std::vector<std::shared_ptr<CSetting>> CPeripheral::GetSettings(void) const
{
  std::vector<PeripheralDeviceSetting> tmpSettings;
  tmpSettings.reserve(m_settings.size());
  for (const auto& it : m_settings)
    tmpSettings.push_back(it.second);
  sort(tmpSettings.begin(), tmpSettings.end(), SortBySettingsOrder());

  std::vector<std::shared_ptr<CSetting>> settings;
  settings.reserve(tmpSettings.size());
  for (const auto& it : tmpSettings)
    settings.push_back(it.m_setting);
  return settings;
}

void CPeripheral::AddSetting(const std::string& strKey, const SettingConstPtr& setting, int order)
{
  if (!setting)
  {
    CLog::Log(LOGERROR, "{} - invalid setting", __FUNCTION__);
    return;
  }

  if (!HasSetting(strKey))
  {
    PeripheralDeviceSetting deviceSetting = {NULL, order};
    switch (setting->GetType())
    {
      case SettingType::Boolean:
      {
        std::shared_ptr<const CSettingBool> mappedSetting =
            std::static_pointer_cast<const CSettingBool>(setting);
        std::shared_ptr<CSettingBool> boolSetting =
            std::make_shared<CSettingBool>(strKey, *mappedSetting);
        if (boolSetting)
        {
          boolSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = boolSetting;
        }
      }
      break;
      case SettingType::Integer:
      {
        std::shared_ptr<const CSettingInt> mappedSetting =
            std::static_pointer_cast<const CSettingInt>(setting);
        std::shared_ptr<CSettingInt> intSetting =
            std::make_shared<CSettingInt>(strKey, *mappedSetting);
        if (intSetting)
        {
          intSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = intSetting;
        }
      }
      break;
      case SettingType::Number:
      {
        std::shared_ptr<const CSettingNumber> mappedSetting =
            std::static_pointer_cast<const CSettingNumber>(setting);
        std::shared_ptr<CSettingNumber> floatSetting =
            std::make_shared<CSettingNumber>(strKey, *mappedSetting);
        if (floatSetting)
        {
          floatSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = floatSetting;
        }
      }
      break;
      case SettingType::String:
      {
        if (std::dynamic_pointer_cast<const CSettingAddon>(setting))
        {
          std::shared_ptr<const CSettingAddon> mappedSetting =
              std::static_pointer_cast<const CSettingAddon>(setting);
          std::shared_ptr<CSettingAddon> addonSetting =
              std::make_shared<CSettingAddon>(strKey, *mappedSetting);
          addonSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = addonSetting;
        }
        else
        {
          std::shared_ptr<const CSettingString> mappedSetting =
              std::static_pointer_cast<const CSettingString>(setting);
          std::shared_ptr<CSettingString> stringSetting =
              std::make_shared<CSettingString>(strKey, *mappedSetting);
          stringSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = stringSetting;
        }
      }
      break;
      default:
        //! @todo add more types if needed
        break;
    }

    if (deviceSetting.m_setting != NULL)
      m_settings.insert(make_pair(strKey, deviceSetting));
  }
}

bool CPeripheral::HasSetting(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  return it != m_settings.end();
}

bool CPeripheral::HasSettings(void) const
{
  return !m_settings.empty();
}

bool CPeripheral::HasConfigurableSettings(void) const
{
  bool bReturn(false);
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.begin();
  while (it != m_settings.end() && !bReturn)
  {
    if ((*it).second.m_setting->IsVisible())
    {
      bReturn = true;
      break;
    }

    ++it;
  }

  return bReturn;
}

bool CPeripheral::GetSettingBool(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Boolean)
  {
    std::shared_ptr<CSettingBool> boolSetting =
        std::static_pointer_cast<CSettingBool>((*it).second.m_setting);
    if (boolSetting)
      return boolSetting->GetValue();
  }

  return false;
}

int CPeripheral::GetSettingInt(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Integer)
  {
    std::shared_ptr<CSettingInt> intSetting =
        std::static_pointer_cast<CSettingInt>((*it).second.m_setting);
    if (intSetting)
      return intSetting->GetValue();
  }

  return 0;
}

float CPeripheral::GetSettingFloat(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Number)
  {
    std::shared_ptr<CSettingNumber> floatSetting =
        std::static_pointer_cast<CSettingNumber>((*it).second.m_setting);
    if (floatSetting)
      return (float)floatSetting->GetValue();
  }

  return 0;
}

const std::string CPeripheral::GetSettingString(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::String)
  {
    std::shared_ptr<CSettingString> stringSetting =
        std::static_pointer_cast<CSettingString>((*it).second.m_setting);
    if (stringSetting)
      return stringSetting->GetValue();
  }

  return "";
}

bool CPeripheral::SetSetting(const std::string& strKey, bool bValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Boolean)
  {
    std::shared_ptr<CSettingBool> boolSetting =
        std::static_pointer_cast<CSettingBool>((*it).second.m_setting);
    if (boolSetting)
    {
      bChanged = boolSetting->GetValue() != bValue;
      boolSetting->SetValue(bValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

bool CPeripheral::SetSetting(const std::string& strKey, int iValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Integer)
  {
    std::shared_ptr<CSettingInt> intSetting =
        std::static_pointer_cast<CSettingInt>((*it).second.m_setting);
    if (intSetting)
    {
      bChanged = intSetting->GetValue() != iValue;
      intSetting->SetValue(iValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

bool CPeripheral::SetSetting(const std::string& strKey, float fValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingType::Number)
  {
    std::shared_ptr<CSettingNumber> floatSetting =
        std::static_pointer_cast<CSettingNumber>((*it).second.m_setting);
    if (floatSetting)
    {
      bChanged = floatSetting->GetValue() != static_cast<double>(fValue);
      floatSetting->SetValue(static_cast<double>(fValue));
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

void CPeripheral::SetSettingVisible(const std::string& strKey, bool bSetTo)
{
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    (*it).second.m_setting->SetVisible(bSetTo);
}

bool CPeripheral::IsSettingVisible(const std::string& strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    return (*it).second.m_setting->IsVisible();
  return false;
}

bool CPeripheral::SetSetting(const std::string& strKey, const std::string& strValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
  {
    if ((*it).second.m_setting->GetType() == SettingType::String)
    {
      std::shared_ptr<CSettingString> stringSetting =
          std::static_pointer_cast<CSettingString>((*it).second.m_setting);
      if (stringSetting)
      {
        bChanged = !StringUtils::EqualsNoCase(stringSetting->GetValue(), strValue);
        stringSetting->SetValue(strValue);
        if (bChanged && m_bInitialised)
          m_changedSettings.insert(strKey);
      }
    }
    else if ((*it).second.m_setting->GetType() == SettingType::Integer)
      bChanged = SetSetting(strKey, strValue.empty() ? 0 : atoi(strValue.c_str()));
    else if ((*it).second.m_setting->GetType() == SettingType::Number)
      bChanged = SetSetting(strKey, (float)(strValue.empty() ? 0 : atof(strValue.c_str())));
    else if ((*it).second.m_setting->GetType() == SettingType::Boolean)
      bChanged = SetSetting(strKey, strValue == "1");
  }
  return bChanged;
}

void CPeripheral::PersistSettings(bool bExiting /* = false */)
{
  CXBMCTinyXML2 doc;
  auto* node = doc.NewElement("settings");
  if (node == nullptr)
    return;

  doc.InsertEndChild(node);
  for (const auto& itr : m_settings)
  {
    auto* nodeSetting = doc.NewElement("setting");
    if (nodeSetting == nullptr)
      continue;

    nodeSetting->SetAttribute("id", itr.first.c_str());
    std::string strValue;
    switch (itr.second.m_setting->GetType())
    {
      case SettingType::String:
      {
        std::shared_ptr<CSettingString> stringSetting =
            std::static_pointer_cast<CSettingString>(itr.second.m_setting);
        if (stringSetting)
          strValue = stringSetting->GetValue();
      }
      break;
      case SettingType::Integer:
      {
        std::shared_ptr<CSettingInt> intSetting =
            std::static_pointer_cast<CSettingInt>(itr.second.m_setting);
        if (intSetting)
          strValue = std::to_string(intSetting->GetValue());
      }
      break;
      case SettingType::Number:
      {
        std::shared_ptr<CSettingNumber> floatSetting =
            std::static_pointer_cast<CSettingNumber>(itr.second.m_setting);
        if (floatSetting)
          strValue = StringUtils::Format("{:.2f}", floatSetting->GetValue());
      }
      break;
      case SettingType::Boolean:
      {
        std::shared_ptr<CSettingBool> boolSetting =
            std::static_pointer_cast<CSettingBool>(itr.second.m_setting);
        if (boolSetting)
          strValue = std::to_string(boolSetting->GetValue() ? 1 : 0);
      }
      break;
      default:
        break;
    }
    nodeSetting->SetAttribute("value", strValue.c_str());
    doc.RootElement()->InsertEndChild(nodeSetting);
  }

  doc.SaveFile(m_strSettingsFile);

  if (!bExiting)
  {
    for (const auto& it : m_changedSettings)
      OnSettingChanged(it);
  }
  m_changedSettings.clear();
}

void CPeripheral::LoadPersistedSettings(void)
{
  CXBMCTinyXML2 doc;
  if (doc.LoadFile(m_strSettingsFile))
  {
    const auto* setting = doc.RootElement()->FirstChildElement("setting");
    while (setting != nullptr)
    {
      std::string strId = XMLUtils::GetAttribute(setting, "id");
      std::string strValue = XMLUtils::GetAttribute(setting, "value");
      SetSetting(strId, strValue);

      setting = setting->NextSiblingElement("setting");
    }
  }
}

void CPeripheral::ResetDefaultSettings(void)
{
  ClearSettings();
  m_manager.GetSettingsFromMapping(*this);

  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.begin();
  while (it != m_settings.end())
  {
    m_changedSettings.insert((*it).first);
    ++it;
  }

  PersistSettings();
}

void CPeripheral::ClearSettings(void)
{
  m_settings.clear();
}

void CPeripheral::RegisterInputHandler(IInputHandler* handler, bool bPromiscuous)
{
  auto it = m_inputHandlers.find(handler);
  if (it == m_inputHandlers.end())
  {
    PeripheralAddonPtr addon = m_manager.GetAddonWithButtonMap(this);
    if (addon)
    {
      std::unique_ptr<CAddonInputHandling> addonInput = std::make_unique<CAddonInputHandling>(
          this, std::move(addon), handler, GetDriverReceiver());
      if (addonInput->Load())
      {
        RegisterJoystickDriverHandler(addonInput.get(), bPromiscuous);
        m_inputHandlers[handler] = std::move(addonInput);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", m_strLocation);
    }
  }
}

void CPeripheral::UnregisterInputHandler(IInputHandler* handler)
{
  handler->ResetInputReceiver();

  auto it = m_inputHandlers.find(handler);
  if (it != m_inputHandlers.end())
  {
    UnregisterJoystickDriverHandler(it->second.get());
    m_inputHandlers.erase(it);
  }
}

void CPeripheral::RegisterKeyboardHandler(KEYBOARD::IKeyboardInputHandler* handler,
                                          bool bPromiscuous,
                                          bool forceDefaultMap)
{
  auto it = m_keyboardHandlers.find(handler);
  if (it == m_keyboardHandlers.end())
  {
    std::unique_ptr<KODI::KEYBOARD::IKeyboardDriverHandler> keyboardDriverHandler;

    if (!forceDefaultMap)
    {
      PeripheralAddonPtr addon = m_manager.GetAddonWithButtonMap(this);
      if (addon)
      {
        std::unique_ptr<CAddonInputHandling> addonInput =
            std::make_unique<CAddonInputHandling>(this, std::move(addon), handler);
        if (addonInput->Load())
          keyboardDriverHandler = std::move(addonInput);
      }
      else
      {
        CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", m_strLocation);
      }
    }

    if (!keyboardDriverHandler)
    {
      std::unique_ptr<KODI::KEYBOARD::CDefaultKeyboardHandling> defaultInput =
          std::make_unique<KODI::KEYBOARD::CDefaultKeyboardHandling>(this, handler);
      if (defaultInput->Load())
        keyboardDriverHandler = std::move(defaultInput);
    }

    if (keyboardDriverHandler)
    {
      RegisterKeyboardDriverHandler(keyboardDriverHandler.get(), bPromiscuous);
      m_keyboardHandlers[handler] = std::move(keyboardDriverHandler);
    }
  }
}

void CPeripheral::UnregisterKeyboardHandler(KEYBOARD::IKeyboardInputHandler* handler)
{
  auto it = m_keyboardHandlers.find(handler);
  if (it != m_keyboardHandlers.end())
  {
    UnregisterKeyboardDriverHandler(it->second.get());
    m_keyboardHandlers.erase(it);
  }
}

void CPeripheral::RegisterMouseHandler(MOUSE::IMouseInputHandler* handler,
                                       bool bPromiscuous,
                                       bool forceDefaultMap)
{
  auto it = m_mouseHandlers.find(handler);
  if (it == m_mouseHandlers.end())
  {
    std::unique_ptr<KODI::MOUSE::IMouseDriverHandler> mouseDriverHandler;

    if (!forceDefaultMap)
    {
      PeripheralAddonPtr addon = m_manager.GetAddonWithButtonMap(this);
      if (addon)
      {
        std::unique_ptr<CAddonInputHandling> addonInput =
            std::make_unique<CAddonInputHandling>(this, std::move(addon), handler);
        if (addonInput->Load())
          mouseDriverHandler = std::move(addonInput);
      }
      else
      {
        CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", m_strLocation);
      }
    }

    if (!mouseDriverHandler)
    {
      std::unique_ptr<KODI::MOUSE::CDefaultMouseHandling> defaultInput =
          std::make_unique<KODI::MOUSE::CDefaultMouseHandling>(this, handler);
      if (defaultInput->Load())
        mouseDriverHandler = std::move(defaultInput);
    }

    if (mouseDriverHandler)
    {
      RegisterMouseDriverHandler(mouseDriverHandler.get(), bPromiscuous);
      m_mouseHandlers[handler] = std::move(mouseDriverHandler);
    }
  }
}

void CPeripheral::UnregisterMouseHandler(MOUSE::IMouseInputHandler* handler)
{
  auto it = m_mouseHandlers.find(handler);
  if (it != m_mouseHandlers.end())
  {
    UnregisterMouseDriverHandler(it->second.get());
    m_mouseHandlers.erase(it);
  }
}

void CPeripheral::RegisterJoystickButtonMapper(IButtonMapper* mapper)
{
  auto it = m_buttonMappers.find(mapper);
  if (it == m_buttonMappers.end())
  {
    std::unique_ptr<CAddonButtonMapping> addonMapping(
        new CAddonButtonMapping(m_manager, this, mapper));

    RegisterJoystickDriverHandler(addonMapping.get(), false);
    RegisterKeyboardDriverHandler(addonMapping.get(), false);
    RegisterMouseDriverHandler(addonMapping.get(), false);

    m_buttonMappers[mapper] = std::move(addonMapping);
  }
}

void CPeripheral::UnregisterJoystickButtonMapper(IButtonMapper* mapper)
{
  auto it = m_buttonMappers.find(mapper);
  if (it != m_buttonMappers.end())
  {
    UnregisterMouseDriverHandler(it->second.get());
    UnregisterKeyboardDriverHandler(it->second.get());
    UnregisterJoystickDriverHandler(it->second.get());

    m_buttonMappers.erase(it);
  }
}

std::string CPeripheral::GetIcon() const
{
  std::string icon;

  // Try controller profile
  const GAME::ControllerPtr controller = ControllerProfile();
  if (controller)
    icon = controller->Layout().ImagePath();

  // Try add-on
  if (icon.empty() && m_busType == PERIPHERAL_BUS_ADDON)
  {
    CPeripheralBusAddon* bus = static_cast<CPeripheralBusAddon*>(m_bus);

    PeripheralAddonPtr addon;
    unsigned int index;
    if (bus->SplitLocation(m_strLocation, addon, index))
    {
      std::string addonIcon = addon->Icon();
      if (!addonIcon.empty())
        icon = std::move(addonIcon);
    }
  }

  // Fallback
  if (icon.empty())
    icon = "DefaultAddon.png";

  return icon;
}

bool CPeripheral::operator==(const PeripheralScanResult& right) const
{
  return StringUtils::EqualsNoCase(m_strLocation, right.m_strLocation);
}

bool CPeripheral::operator!=(const PeripheralScanResult& right) const
{
  return !(*this == right);
}

CDateTime CPeripheral::LastActive() const
{
  return CDateTime();
}
