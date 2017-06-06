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

#include "Peripheral.h"

#include <utility>

#include "guilib/LocalizeStrings.h"
#include "input/joysticks/IInputHandler.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "peripherals/Peripherals.h"
#include "settings/lib/Setting.h"
#include "peripherals/addons/AddonButtonMapping.h"
#include "peripherals/addons/AddonInputHandling.h"
#include "peripherals/bus/PeripheralBus.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "Util.h"
#include "filesystem/File.h"

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

struct SortBySettingsOrder
{
  bool operator()(const PeripheralDeviceSetting &left, const PeripheralDeviceSetting& right)
  {
    return left.m_order < right.m_order;
  }
};

CPeripheral::CPeripheral(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  m_manager(manager),
  m_type(scanResult.m_mappedType),
  m_busType(scanResult.m_busType),
  m_mappedBusType(scanResult.m_mappedBusType),
  m_strLocation(scanResult.m_strLocation),
  m_strDeviceName(scanResult.m_strDeviceName),
  m_iVendorId(scanResult.m_iVendorId),
  m_iProductId(scanResult.m_iProductId),
  m_strVersionInfo(g_localizeStrings.Get(13205)), // "unknown"
  m_bInitialised(false),
  m_bHidden(false),
  m_bError(false),
  m_bus(bus)
{
  PeripheralTypeTranslator::FormatHexString(scanResult.m_iVendorId, m_strVendorId);
  PeripheralTypeTranslator::FormatHexString(scanResult.m_iProductId, m_strProductId);
  if (scanResult.m_iSequence > 0)
  {
    m_strFileLocation = StringUtils::Format("peripherals://%s/%s_%d.dev",
                                            PeripheralTypeTranslator::BusTypeToString(scanResult.m_busType),
                                            scanResult.m_strLocation.c_str(),
                                            scanResult.m_iSequence);
  }
  else
  {
    m_strFileLocation = StringUtils::Format("peripherals://%s/%s.dev",
                                            PeripheralTypeTranslator::BusTypeToString(scanResult.m_busType),
                                            scanResult.m_strLocation.c_str());
  }
}

CPeripheral::~CPeripheral(void)
{
  PersistSettings(true);

  m_subDevices.clear();

  ClearSettings();
}

bool CPeripheral::operator ==(const CPeripheral &right) const
{
  return m_type == right.m_type &&
      m_strLocation == right.m_strLocation &&
      m_iVendorId == right.m_iVendorId &&
      m_iProductId == right.m_iProductId;
}

bool CPeripheral::operator !=(const CPeripheral &right) const
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

void CPeripheral::GetFeatures(std::vector<PeripheralFeature> &features) const
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
    m_strSettingsFile = StringUtils::Format("special://profile/peripheral_data/%s_%s.xml",
                                            PeripheralTypeTranslator::BusTypeToString(m_mappedBusType),
                                            CUtil::MakeLegalFileName(safeDeviceName, LEGAL_WIN32_COMPAT).c_str());
  }
  else
  {
    // Backwards compatibility - old settings files didn't include the device name
    m_strSettingsFile = StringUtils::Format("special://profile/peripheral_data/%s_%s_%s.xml",
                                            PeripheralTypeTranslator::BusTypeToString(m_mappedBusType),
                                            m_strVendorId.c_str(),
                                            m_strProductId.c_str());

    if (!XFILE::CFile::Exists(m_strSettingsFile))
      m_strSettingsFile = StringUtils::Format("special://profile/peripheral_data/%s_%s_%s_%s.xml",
                                              PeripheralTypeTranslator::BusTypeToString(m_mappedBusType),
                                              m_strVendorId.c_str(),
                                              m_strProductId.c_str(),
                                              CUtil::MakeLegalFileName(safeDeviceName, LEGAL_WIN32_COMPAT).c_str());
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
    CLog::Log(LOGDEBUG, "%s - initialised peripheral on '%s' with %d features and %d sub devices",
      __FUNCTION__, m_strLocation.c_str(), (int)m_features.size(), (int)m_subDevices.size());
    m_bInitialised = true;
  }

  return bReturn;
}

void CPeripheral::GetSubdevices(PeripheralVector &subDevices) const
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
  for (std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.begin(); it != m_settings.end(); ++it)
    tmpSettings.push_back(it->second);
  sort(tmpSettings.begin(), tmpSettings.end(), SortBySettingsOrder());

  std::vector<std::shared_ptr<CSetting>> settings;
  for (std::vector<PeripheralDeviceSetting>::const_iterator it = tmpSettings.begin(); it != tmpSettings.end(); ++it)
    settings.push_back(it->m_setting);
  return settings;
}

void CPeripheral::AddSetting(const std::string &strKey, SettingConstPtr setting, int order)
{
  if (!setting)
  {
    CLog::Log(LOGERROR, "%s - invalid setting", __FUNCTION__);
    return;
  }

  if (!HasSetting(strKey))
  {
    PeripheralDeviceSetting deviceSetting = { NULL, order };
    switch(setting->GetType())
    {
    case SettingTypeBool:
      {
        std::shared_ptr<const CSettingBool> mappedSetting = std::static_pointer_cast<const CSettingBool>(setting);
        std::shared_ptr<CSettingBool> boolSetting = std::make_shared<CSettingBool>(strKey, *mappedSetting);
        if (boolSetting)
        {
          boolSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = boolSetting;
        }
      }
      break;
    case SettingTypeInteger:
      {
        std::shared_ptr<const CSettingInt> mappedSetting = std::static_pointer_cast<const CSettingInt>(setting);
        std::shared_ptr<CSettingInt> intSetting = std::make_shared<CSettingInt>(strKey, *mappedSetting);
        if (intSetting)
        {
          intSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = intSetting;
        }
      }
      break;
    case SettingTypeNumber:
      {
        std::shared_ptr<const CSettingNumber> mappedSetting = std::static_pointer_cast<const CSettingNumber>(setting);
        std::shared_ptr<CSettingNumber> floatSetting = std::make_shared<CSettingNumber>(strKey, *mappedSetting);
        if (floatSetting)
        {
          floatSetting->SetVisible(mappedSetting->IsVisible());
          deviceSetting.m_setting = floatSetting;
        }
      }
      break;
    case SettingTypeString:
      {
        std::shared_ptr<const CSettingString> mappedSetting = std::static_pointer_cast<const CSettingString>(setting);
        std::shared_ptr<CSettingString> stringSetting = std::make_shared<CSettingString>(strKey, *mappedSetting);
        if (stringSetting)
        {
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

bool CPeripheral::HasSetting(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>:: const_iterator it = m_settings.find(strKey);
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

bool CPeripheral::GetSettingBool(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeBool)
  {
    std::shared_ptr<CSettingBool> boolSetting = std::static_pointer_cast<CSettingBool>((*it).second.m_setting);
    if (boolSetting)
      return boolSetting->GetValue();
  }

  return false;
}

int CPeripheral::GetSettingInt(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeInteger)
  {
    std::shared_ptr<CSettingInt> intSetting = std::static_pointer_cast<CSettingInt>((*it).second.m_setting);
    if (intSetting)
      return intSetting->GetValue();
  }

  return 0;
}

float CPeripheral::GetSettingFloat(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeNumber)
  {
    std::shared_ptr<CSettingNumber> floatSetting = std::static_pointer_cast<CSettingNumber>((*it).second.m_setting);
    if (floatSetting)
      return (float)floatSetting->GetValue();
  }

  return 0;
}

const std::string CPeripheral::GetSettingString(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeString)
  {
    std::shared_ptr<CSettingString> stringSetting = std::static_pointer_cast<CSettingString>((*it).second.m_setting);
    if (stringSetting)
      return stringSetting->GetValue();
  }

  return "";
}

bool CPeripheral::SetSetting(const std::string &strKey, bool bValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeBool)
  {
    std::shared_ptr<CSettingBool> boolSetting = std::static_pointer_cast<CSettingBool>((*it).second.m_setting);
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

bool CPeripheral::SetSetting(const std::string &strKey, int iValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeInteger)
  {
    std::shared_ptr<CSettingInt> intSetting = std::static_pointer_cast<CSettingInt>((*it).second.m_setting);
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

bool CPeripheral::SetSetting(const std::string &strKey, float fValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second.m_setting->GetType() == SettingTypeNumber)
  {
    std::shared_ptr<CSettingNumber> floatSetting = std::static_pointer_cast<CSettingNumber>((*it).second.m_setting);
    if (floatSetting)
    {
      bChanged = floatSetting->GetValue() != fValue;
      floatSetting->SetValue(fValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

void CPeripheral::SetSettingVisible(const std::string &strKey, bool bSetTo)
{
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    (*it).second.m_setting->SetVisible(bSetTo);
}

bool CPeripheral::IsSettingVisible(const std::string &strKey) const
{
  std::map<std::string, PeripheralDeviceSetting>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    return (*it).second.m_setting->IsVisible();
  return false;
}

bool CPeripheral::SetSetting(const std::string &strKey, const std::string &strValue)
{
  bool bChanged(false);
  std::map<std::string, PeripheralDeviceSetting>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
  {
    if ((*it).second.m_setting->GetType() == SettingTypeString)
    {
      std::shared_ptr<CSettingString> stringSetting = std::static_pointer_cast<CSettingString>((*it).second.m_setting);
      if (stringSetting)
      {
        bChanged = !StringUtils::EqualsNoCase(stringSetting->GetValue(), strValue);
        stringSetting->SetValue(strValue);
        if (bChanged && m_bInitialised)
          m_changedSettings.insert(strKey);
      }
    }
    else if ((*it).second.m_setting->GetType() == SettingTypeInteger)
      bChanged = SetSetting(strKey, (int) (strValue.empty() ? 0 : atoi(strValue.c_str())));
    else if ((*it).second.m_setting->GetType() == SettingTypeNumber)
      bChanged = SetSetting(strKey, (float) (strValue.empty() ? 0 : atof(strValue.c_str())));
    else if ((*it).second.m_setting->GetType() == SettingTypeBool)
      bChanged = SetSetting(strKey, strValue == "1");
  }
  return bChanged;
}

void CPeripheral::PersistSettings(bool bExiting /* = false */)
{
  CXBMCTinyXML doc;
  TiXmlElement node("settings");
  doc.InsertEndChild(node);
  for (std::map<std::string, PeripheralDeviceSetting>::const_iterator itr = m_settings.begin(); itr != m_settings.end(); ++itr)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", itr->first.c_str());
    std::string strValue;
    switch ((*itr).second.m_setting->GetType())
    {
    case SettingTypeString:
      {
        std::shared_ptr<CSettingString> stringSetting = std::static_pointer_cast<CSettingString>((*itr).second.m_setting);
        if (stringSetting)
          strValue = stringSetting->GetValue();
      }
      break;
    case SettingTypeInteger:
      {
        std::shared_ptr<CSettingInt> intSetting = std::static_pointer_cast<CSettingInt>((*itr).second.m_setting);
        if (intSetting)
          strValue = StringUtils::Format("%d", intSetting->GetValue());
      }
      break;
    case SettingTypeNumber:
      {
        std::shared_ptr<CSettingNumber> floatSetting = std::static_pointer_cast<CSettingNumber>((*itr).second.m_setting);
        if (floatSetting)
          strValue = StringUtils::Format("%.2f", floatSetting->GetValue());
      }
      break;
    case SettingTypeBool:
      {
        std::shared_ptr<CSettingBool> boolSetting = std::static_pointer_cast<CSettingBool>((*itr).second.m_setting);
        if (boolSetting)
          strValue = StringUtils::Format("%d", boolSetting->GetValue() ? 1:0);
      }
      break;
    default:
      break;
    }
    nodeSetting.SetAttribute("value", strValue.c_str());
    doc.RootElement()->InsertEndChild(nodeSetting);
  }

  doc.SaveFile(m_strSettingsFile);

  if (!bExiting)
  {
    for (std::set<std::string>::const_iterator it = m_changedSettings.begin(); it != m_changedSettings.end(); ++it)
      OnSettingChanged(*it);
  }
  m_changedSettings.clear();
}

void CPeripheral::LoadPersistedSettings(void)
{
  CXBMCTinyXML doc;
  if (doc.LoadFile(m_strSettingsFile))
  {
    const TiXmlElement *setting = doc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      std::string    strId = XMLUtils::GetAttribute(setting, "id");
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

void CPeripheral::RegisterJoystickInputHandler(IInputHandler* handler, bool bPromiscuous)
{
  auto it = m_inputHandlers.find(handler);
  if (it == m_inputHandlers.end())
  {
    CAddonInputHandling* addonInput = new CAddonInputHandling(m_manager, this, handler, GetDriverReceiver());
    RegisterJoystickDriverHandler(addonInput, bPromiscuous);
    m_inputHandlers[handler].reset(addonInput);
  }
}

void CPeripheral::UnregisterJoystickInputHandler(IInputHandler* handler)
{
  handler->ResetInputReceiver();

  auto it = m_inputHandlers.find(handler);
  if (it != m_inputHandlers.end())
  {
    UnregisterJoystickDriverHandler(it->second.get());
    m_inputHandlers.erase(it);
  }
}

void CPeripheral::RegisterJoystickButtonMapper(IButtonMapper* mapper)
{
  std::map<IButtonMapper*, IDriverHandler*>::iterator it = m_buttonMappers.find(mapper);
  if (it == m_buttonMappers.end())
  {
    IDriverHandler* addonMapping = new CAddonButtonMapping(m_manager, this, mapper);
    RegisterJoystickDriverHandler(addonMapping, false);
    m_buttonMappers[mapper] = addonMapping;
  }
}

void CPeripheral::UnregisterJoystickButtonMapper(IButtonMapper* mapper)
{
  std::map<IButtonMapper*, IDriverHandler*>::iterator it = m_buttonMappers.find(mapper);
  if (it != m_buttonMappers.end())
  {
    UnregisterJoystickDriverHandler(it->second);
    delete it->second;
    m_buttonMappers.erase(it);
  }
}

std::string CPeripheral::GetIcon() const
{
  std::string icon = "DefaultAddon.png";

  if (m_busType == PERIPHERAL_BUS_ADDON)
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

  return icon;
}

bool CPeripheral::operator ==(const PeripheralScanResult& right) const
{
  return StringUtils::EqualsNoCase(m_strLocation, right.m_strLocation);
}

bool CPeripheral::operator !=(const PeripheralScanResult& right) const
{
  return !(*this == right);
}
