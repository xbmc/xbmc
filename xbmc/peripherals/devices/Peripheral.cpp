/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "peripherals/Peripherals.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "settings/GUISettings.h"
#include "utils/XBMCTinyXML.h"
#include "utils/URIUtils.h"
#include "guilib/LocalizeStrings.h"

using namespace PERIPHERALS;
using namespace std;

struct SortBySettingsOrder
{
  bool operator()(const CSetting *left, const CSetting *right)
  {
    return left->GetOrder() < right->GetOrder();
  }
};

CPeripheral::CPeripheral(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  m_type(type),
  m_busType(busType),
  m_strLocation(strLocation),
  m_strDeviceName(strDeviceName),
  m_strFileLocation(StringUtils::EmptyString),
  m_iVendorId(iVendorId),
  m_iProductId(iProductId),
  m_strVersionInfo(g_localizeStrings.Get(13205)), // "unknown"
  m_bInitialised(false),
  m_bHidden(false),
  m_bError(false)
{
  PeripheralTypeTranslator::FormatHexString(iVendorId, m_strVendorId);
  PeripheralTypeTranslator::FormatHexString(iProductId, m_strProductId);
  m_strFileLocation.Format("peripherals://%s/%s.dev", PeripheralTypeTranslator::BusTypeToString(busType), strLocation.c_str());
}

CPeripheral::CPeripheral(void) :
  m_type(PERIPHERAL_UNKNOWN),
  m_busType(PERIPHERAL_BUS_UNKNOWN),
  m_strLocation(StringUtils::EmptyString),
  m_strDeviceName(StringUtils::EmptyString),
  m_strFileLocation(StringUtils::EmptyString),
  m_iVendorId(0),
  m_strVendorId("0000"),
  m_iProductId(0),
  m_strProductId("0000"),
  m_strVersionInfo(g_localizeStrings.Get(13205)), // "unknown"
  m_bInitialised(false),
  m_bHidden(false),
  m_bError(false)
{
}

CPeripheral::~CPeripheral(void)
{
  PersistSettings(true);

  for (unsigned int iSubdevicePtr = 0; iSubdevicePtr < m_subDevices.size(); iSubdevicePtr++)
    delete m_subDevices.at(iSubdevicePtr);
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

  g_peripherals.GetSettingsFromMapping(*this);
  m_strSettingsFile.Format("special://profile/peripheral_data/%s_%s_%s.xml", PeripheralTypeTranslator::BusTypeToString(m_busType), m_strVendorId.c_str(), m_strProductId.c_str());
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

void CPeripheral::GetSubdevices(vector<CPeripheral *> &subDevices) const
{
  for (unsigned int iSubdevicePtr = 0; iSubdevicePtr < m_subDevices.size(); iSubdevicePtr++)
    subDevices.push_back(m_subDevices.at(iSubdevicePtr));
}

bool CPeripheral::IsMultiFunctional(void) const
{
  return m_subDevices.size() > 0;
}

vector<CSetting *> CPeripheral::GetSettings(void) const
{
  vector<CSetting *> settings;
  for (map<CStdString, CSetting *>::const_iterator it = m_settings.begin(); it != m_settings.end(); it++)
    settings.push_back(it->second);
  sort(settings.begin(), settings.end(), SortBySettingsOrder());
  return settings;
}

void CPeripheral::AddSetting(const CStdString &strKey, const CSetting *setting)
{
  if (!setting)
  {
    CLog::Log(LOGERROR, "%s - invalid setting", __FUNCTION__);
    return;
  }

  if (!HasSetting(strKey))
  {
    switch(setting->GetType())
    {
    case SETTINGS_TYPE_BOOL:
      {
        const CSettingBool *mappedSetting = (const CSettingBool *) setting;
        CSettingBool *boolSetting = new CSettingBool(mappedSetting->GetOrder(), strKey.c_str(), mappedSetting->GetLabel(), mappedSetting->GetData(), mappedSetting->GetControlType());
        if (boolSetting)
        {
          boolSetting->SetVisible(mappedSetting->IsVisible());
          m_settings.insert(make_pair(strKey, boolSetting));
        }
      }
      break;
    case SETTINGS_TYPE_INT:
      {
        const CSettingInt *mappedSetting = (const CSettingInt *) setting;
        CSettingInt *intSetting(NULL);
        if (mappedSetting->GetControlType() == SPIN_CONTROL_INT)
        {
          intSetting = new CSettingInt(mappedSetting->GetOrder(), strKey.c_str(), mappedSetting->GetLabel(), mappedSetting->GetData(), mappedSetting->m_iMin, mappedSetting->m_iStep, mappedSetting->m_iMax, mappedSetting->GetControlType(), mappedSetting->m_strFormat);
        }
        else if (mappedSetting->GetControlType() == SPIN_CONTROL_TEXT)
        {
          intSetting = new CSettingInt(mappedSetting->GetOrder(), strKey.c_str(), mappedSetting->GetLabel(), mappedSetting->GetData(), mappedSetting->m_entries, mappedSetting->GetControlType());
        }
        if (intSetting)
        {
          intSetting->SetVisible(mappedSetting->IsVisible());
          m_settings.insert(make_pair(strKey, intSetting));
        }
      }
      break;
    case SETTINGS_TYPE_FLOAT:
      {
        const CSettingFloat *mappedSetting = (const CSettingFloat *) setting;
        CSettingFloat *floatSetting = new CSettingFloat(mappedSetting->GetOrder(), strKey.c_str(), mappedSetting->GetLabel(), mappedSetting->GetData(), mappedSetting->m_fMin, mappedSetting->m_fStep, mappedSetting->m_fMax, mappedSetting->GetControlType());
        if (floatSetting)
        {
          floatSetting->SetVisible(mappedSetting->IsVisible());
          m_settings.insert(make_pair(strKey, floatSetting));
        }
      }
      break;
    case SETTINGS_TYPE_STRING:
      {
        const CSettingString *mappedSetting = (const CSettingString *) setting;
        CSettingString *stringSetting = new CSettingString(mappedSetting->GetOrder(), strKey.c_str(), mappedSetting->GetLabel(), mappedSetting->GetData().c_str(), mappedSetting->GetControlType(), mappedSetting->m_bAllowEmpty, mappedSetting->m_iHeadingString);
        if (stringSetting)
        {
          stringSetting->SetVisible(mappedSetting->IsVisible());
          m_settings.insert(make_pair(strKey, stringSetting));
        }
      }
      break;
    default:
      //TODO add more types if needed
      break;
    }
  }
}

bool CPeripheral::HasSetting(const CStdString &strKey) const
{
  map<CStdString, CSetting *>:: const_iterator it = m_settings.find(strKey);
  return it != m_settings.end();
}

bool CPeripheral::HasSettings(void) const
{
  return m_settings.size() > 0;
}

bool CPeripheral::HasConfigurableSettings(void) const
{
  bool bReturn(false);
  map<CStdString, CSetting *>::const_iterator it = m_settings.begin();
  while (it != m_settings.end() && !bReturn)
  {
    if ((*it).second->IsVisible())
    {
      bReturn = true;
      break;
    }

    ++it;
  }

  return bReturn;
}

bool CPeripheral::GetSettingBool(const CStdString &strKey) const
{
  map<CStdString, CSetting *>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_BOOL)
  {
    CSettingBool *boolSetting = (CSettingBool *) (*it).second;
    if (boolSetting)
      return boolSetting->GetData();
  }

  return false;
}

int CPeripheral::GetSettingInt(const CStdString &strKey) const
{
  map<CStdString, CSetting *>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_INT)
  {
    CSettingInt *intSetting = (CSettingInt *) (*it).second;
    if (intSetting)
      return intSetting->GetData();
  }

  return 0;
}

float CPeripheral::GetSettingFloat(const CStdString &strKey) const
{
  map<CStdString, CSetting *>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_FLOAT)
  {
    CSettingFloat *floatSetting = (CSettingFloat *) (*it).second;
    if (floatSetting)
      return floatSetting->GetData();
  }

  return 0;
}

const CStdString CPeripheral::GetSettingString(const CStdString &strKey) const
{
  map<CStdString, CSetting *>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_STRING)
  {
    CSettingString *stringSetting = (CSettingString *) (*it).second;
    if (stringSetting)
      return stringSetting->GetData();
  }

  return StringUtils::EmptyString;
}

bool CPeripheral::SetSetting(const CStdString &strKey, bool bValue)
{
  bool bChanged(false);
  map<CStdString, CSetting *>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_BOOL)
  {
    CSettingBool *boolSetting = (CSettingBool *) (*it).second;
    if (boolSetting)
    {
      bChanged = boolSetting->GetData() != bValue;
      boolSetting->SetData(bValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

bool CPeripheral::SetSetting(const CStdString &strKey, int iValue)
{
  bool bChanged(false);
  map<CStdString, CSetting *>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_INT)
  {
    CSettingInt *intSetting = (CSettingInt *) (*it).second;
    if (intSetting)
    {
      bChanged = intSetting->GetData() != iValue;
      intSetting->SetData(iValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

bool CPeripheral::SetSetting(const CStdString &strKey, float fValue)
{
  bool bChanged(false);
  map<CStdString, CSetting *>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end() && (*it).second->GetType() == SETTINGS_TYPE_FLOAT)
  {
    CSettingFloat *floatSetting = (CSettingFloat *) (*it).second;
    if (floatSetting)
    {
      bChanged = floatSetting->GetData() != fValue;
      floatSetting->SetData(fValue);
      if (bChanged && m_bInitialised)
        m_changedSettings.insert(strKey);
    }
  }
  return bChanged;
}

void CPeripheral::SetSettingVisible(const CStdString &strKey, bool bSetTo)
{
  map<CStdString, CSetting *>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    (*it).second->SetVisible(bSetTo);
}

bool CPeripheral::IsSettingVisible(const CStdString &strKey) const
{
  map<CStdString, CSetting *>::const_iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
    return (*it).second->IsVisible();
  return false;
}

bool CPeripheral::SetSetting(const CStdString &strKey, const CStdString &strValue)
{
  bool bChanged(false);
  map<CStdString, CSetting *>::iterator it = m_settings.find(strKey);
  if (it != m_settings.end())
  {
    if ((*it).second->GetType() == SETTINGS_TYPE_STRING)
    {
      CSettingString *stringSetting = (CSettingString *) (*it).second;
      if (stringSetting)
      {
        bChanged = !stringSetting->GetData().Equals(strValue);
        stringSetting->SetData(strValue);
        if (bChanged && m_bInitialised)
          m_changedSettings.insert(strKey);
      }
    }
    else if ((*it).second->GetType() == SETTINGS_TYPE_INT)
      bChanged = SetSetting(strKey, (int) (strValue.IsEmpty() ? 0 : atoi(strValue.c_str())));
    else if ((*it).second->GetType() == SETTINGS_TYPE_FLOAT)
      bChanged = SetSetting(strKey, (float) (strValue.IsEmpty() ? 0 : atof(strValue.c_str())));
    else if ((*it).second->GetType() == SETTINGS_TYPE_BOOL)
      bChanged = SetSetting(strKey, strValue.Equals("1"));
  }
  return bChanged;
}

void CPeripheral::PersistSettings(bool bExiting /* = false */)
{
  CXBMCTinyXML doc;
  TiXmlElement node("settings");
  doc.InsertEndChild(node);
  for (map<CStdString, CSetting *>::const_iterator itr = m_settings.begin(); itr != m_settings.end(); itr++)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", itr->first.c_str());
    CStdString strValue;
    switch ((*itr).second->GetType())
    {
    case SETTINGS_TYPE_STRING:
      {
        CSettingString *stringSetting = (CSettingString *) (*itr).second;
        if (stringSetting)
          strValue = stringSetting->GetData();
      }
      break;
    case SETTINGS_TYPE_INT:
      {
        CSettingInt *intSetting = (CSettingInt *) (*itr).second;
        if (intSetting)
          strValue.Format("%d", intSetting->GetData());
      }
      break;
    case SETTINGS_TYPE_FLOAT:
      {
        CSettingFloat *floatSetting = (CSettingFloat *) (*itr).second;
        if (floatSetting)
          strValue.Format("%.2f", floatSetting->GetData());
      }
      break;
    case SETTINGS_TYPE_BOOL:
      {
        CSettingBool *boolSetting = (CSettingBool *) (*itr).second;
        if (boolSetting)
          strValue.Format("%d", boolSetting->GetData() ? 1:0);
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
    for (set<CStdString>::const_iterator it = m_changedSettings.begin(); it != m_changedSettings.end(); it++)
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
      CStdString strId(setting->Attribute("id"));
      CStdString strValue(setting->Attribute("value"));
      SetSetting(strId, strValue);

      setting = setting->NextSiblingElement("setting");
    }
  }
}

void CPeripheral::ResetDefaultSettings(void)
{
  ClearSettings();
  g_peripherals.GetSettingsFromMapping(*this);

  map<CStdString, CSetting *>::iterator it = m_settings.begin();
  while (it != m_settings.end())
  {
    m_changedSettings.insert((*it).first);
    ++it;
  }

  PersistSettings();
}

void CPeripheral::ClearSettings(void)
{
  map<CStdString, CSetting *>::iterator it = m_settings.begin();
  while (it != m_settings.end())
  {
    delete (*it).second;
    ++it;
  }
  m_settings.clear();
}
