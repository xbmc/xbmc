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

#include "Peripherals.h"
#include "bus/PeripheralBus.h"
#include "devices/PeripheralBluetooth.h"
#include "devices/PeripheralDisk.h"
#include "devices/PeripheralHID.h"
#include "devices/PeripheralNIC.h"
#include "devices/PeripheralNyxboard.h"
#include "devices/PeripheralTuner.h"
#include "devices/PeripheralCecAdapter.h"
#include "devices/PeripheralImon.h"
#include "bus/PeripheralBusUSB.h"
#include "dialogs/GUIDialogPeripheralManager.h"

#if defined(HAVE_LIBCEC)
#include "bus/virtual/PeripheralBusCEC.h"
#endif

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "utils/XBMCTinyXML.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "GUIUserMessages.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "input/Key.h"
#include "settings/lib/Setting.h"

using namespace PERIPHERALS;
using namespace XFILE;
using namespace std;

CPeripherals::CPeripherals(void)
{
  Clear();
}

CPeripherals::~CPeripherals(void)
{
  Clear();
}

CPeripherals &CPeripherals::Get(void)
{
  static CPeripherals peripheralsInstance;
  return peripheralsInstance;
}

void CPeripherals::Initialise(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bIsStarted)
  {
    m_bIsStarted = true;

    CDirectory::Create("special://profile/peripheral_data");

    /* load mappings from peripherals.xml */
    LoadMappings();

#if defined(HAVE_PERIPHERAL_BUS_USB)
    m_busses.push_back(new CPeripheralBusUSB(this));
#endif
#if defined(HAVE_LIBCEC)
    m_busses.push_back(new CPeripheralBusCEC(this));
#endif

    /* initialise all known busses */
    for (int iBusPtr = (int)m_busses.size() - 1; iBusPtr >= 0; iBusPtr--)
    {
      if (!m_busses.at(iBusPtr)->Initialise())
      {
        CLog::Log(LOGERROR, "%s - failed to initialise bus %s", __FUNCTION__, PeripheralTypeTranslator::BusTypeToString(m_busses.at(iBusPtr)->Type()));
        delete m_busses.at(iBusPtr);
        m_busses.erase(m_busses.begin() + iBusPtr);
      }
    }

    m_bInitialised = true;
  }
}

void CPeripherals::Clear(void)
{
  CSingleLock lock(m_critSection);
  /* delete busses and devices */
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
    delete m_busses.at(iBusPtr);
  m_busses.clear();

  /* delete mappings */
  for (unsigned int iMappingPtr = 0; iMappingPtr < m_mappings.size(); iMappingPtr++)
  {
    map<std::string, PeripheralDeviceSetting> settings = m_mappings.at(iMappingPtr).m_settings;
    for (map<std::string, PeripheralDeviceSetting>::iterator itr = settings.begin(); itr != settings.end(); ++itr)
      delete itr->second.m_setting;
    m_mappings.at(iMappingPtr).m_settings.clear();
  }
  m_mappings.clear();

  /* reset class state */
  m_bIsStarted   = false;
  m_bInitialised = false;
#if !defined(HAVE_LIBCEC)
  m_bMissingLibCecWarningDisplayed = false;
#endif
}

void CPeripherals::TriggerDeviceScan(const PeripheralBusType type /* = PERIPHERAL_BUS_UNKNOWN */)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (type == PERIPHERAL_BUS_UNKNOWN || m_busses.at(iBusPtr)->Type() == type)
    {
      m_busses.at(iBusPtr)->TriggerDeviceScan();
      if (type != PERIPHERAL_BUS_UNKNOWN)
        break;
    }
  }
}

CPeripheralBus *CPeripherals::GetBusByType(const PeripheralBusType type) const
{
  CSingleLock lock(m_critSection);
  CPeripheralBus *bus(NULL);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (m_busses.at(iBusPtr)->Type() == type)
    {
      bus = m_busses.at(iBusPtr);
      break;
    }
  }

  return bus;
}

CPeripheral *CPeripherals::GetPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  CSingleLock lock(m_critSection);
  CPeripheral *peripheral(NULL);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    /* check whether the bus matches if a bus type other than unknown was passed */
    if (busType != PERIPHERAL_BUS_UNKNOWN && m_busses.at(iBusPtr)->Type() != busType)
      continue;

    /* return the first device that matches */
    if ((peripheral = m_busses.at(iBusPtr)->GetPeripheral(strLocation)) != NULL)
      break;
  }

  return peripheral;
}

bool CPeripherals::HasPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  return (GetPeripheralAtLocation(strLocation, busType) != NULL);
}

CPeripheralBus *CPeripherals::GetBusWithDevice(const std::string &strLocation) const
{
  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    /* return the first bus that matches */
    if (m_busses.at(iBusPtr)->HasPeripheral(strLocation))
      return m_busses.at(iBusPtr);
  }

  return NULL;
}

int CPeripherals::GetPeripheralsWithFeature(vector<CPeripheral *> &results, const PeripheralFeature feature, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  CSingleLock lock(m_critSection);
  int iReturn(0);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    /* check whether the bus matches if a bus type other than unknown was passed */
    if (busType != PERIPHERAL_BUS_UNKNOWN && m_busses.at(iBusPtr)->Type() != busType)
      continue;

    iReturn += m_busses.at(iBusPtr)->GetPeripheralsWithFeature(results, feature);
  }

  return iReturn;
}

size_t CPeripherals::GetNumberOfPeripherals() const
{
  size_t iReturn(0);
  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    iReturn += m_busses.at(iBusPtr)->GetNumberOfPeripherals();
  }

  return iReturn;
}

bool CPeripherals::HasPeripheralWithFeature(const PeripheralFeature feature, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  vector<CPeripheral *> dummy;
  return (GetPeripheralsWithFeature(dummy, feature, busType) > 0);
}

CPeripheral *CPeripherals::CreatePeripheral(CPeripheralBus &bus, const PeripheralScanResult& result)
{
  CPeripheral *peripheral = NULL;
  PeripheralScanResult mappedResult = result;
  if (mappedResult.m_busType == PERIPHERAL_BUS_UNKNOWN)
    mappedResult.m_busType = bus.Type();

  /* check whether there's something mapped in peripherals.xml */
  if (!GetMappingForDevice(bus, mappedResult))
  {
    /* don't create instances for devices that aren't mapped in peripherals.xml */
    return NULL;
  }

  switch(mappedResult.m_mappedType)
  {
  case PERIPHERAL_HID:
    peripheral = new CPeripheralHID(mappedResult);
    break;

  case PERIPHERAL_NIC:
    peripheral = new CPeripheralNIC(mappedResult);
    break;

  case PERIPHERAL_DISK:
    peripheral = new CPeripheralDisk(mappedResult);
    break;

  case PERIPHERAL_NYXBOARD:
    peripheral = new CPeripheralNyxboard(mappedResult);
    break;

  case PERIPHERAL_TUNER:
    peripheral = new CPeripheralTuner(mappedResult);
    break;

  case PERIPHERAL_BLUETOOTH:
    peripheral = new CPeripheralBluetooth(mappedResult);
    break;

  case PERIPHERAL_CEC:
#if defined(HAVE_LIBCEC)
    if (bus.Type() == PERIPHERAL_BUS_CEC)
      peripheral = new CPeripheralCecAdapter(mappedResult);
#else
    if (!m_bMissingLibCecWarningDisplayed)
    {
      m_bMissingLibCecWarningDisplayed = true;
      CLog::Log(LOGWARNING, "%s - libCEC support has not been compiled in, so the CEC adapter cannot be used.", __FUNCTION__);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(36000), g_localizeStrings.Get(36017));
    }
#endif
    break;

  case PERIPHERAL_IMON:
    peripheral = new CPeripheralImon(mappedResult);
    break;

  default:
    break;
  }

  if (peripheral)
  {
    /* try to initialise the new peripheral
     * Initialise() will make sure that each device is only initialised once */
    if (peripheral->Initialise())
    {
      bus.Register(peripheral);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - failed to initialise peripheral on '%s'", __FUNCTION__, mappedResult.m_strLocation.c_str());
      delete peripheral;
      peripheral = NULL;
    }
  }

  return peripheral;
}

void CPeripherals::OnDeviceAdded(const CPeripheralBus &bus, const CPeripheral &peripheral)
{
  CGUIDialogPeripheralManager *dialog = (CGUIDialogPeripheralManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_MANAGER);
  if (dialog && dialog->IsActive())
    dialog->Update();

  // refresh settings (peripherals manager could be enabled now)
  CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
  g_windowManager.SendThreadMessage(msg, WINDOW_SETTINGS_SYSTEM);

  SetChanged();

  // don't show a notification for devices detected during the initial scan
  if (bus.IsInitialised())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35005), peripheral.DeviceName());
}

void CPeripherals::OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral)
{
  CGUIDialogPeripheralManager *dialog = (CGUIDialogPeripheralManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_MANAGER);
  if (dialog && dialog->IsActive())
    dialog->Update();

  // refresh settings (peripherals manager could be disabled now)
  CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
  g_windowManager.SendThreadMessage(msg, WINDOW_SETTINGS_SYSTEM);

  SetChanged();

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35006), peripheral.DeviceName());
}

bool CPeripherals::GetMappingForDevice(const CPeripheralBus &bus, PeripheralScanResult& result) const
{
  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (unsigned int iMappingPtr = 0; iMappingPtr < m_mappings.size(); iMappingPtr++)
  {
    PeripheralDeviceMapping mapping = m_mappings.at(iMappingPtr);

    bool bProductMatch = false;
    if (mapping.m_PeripheralID.size() == 0)
    {
      bProductMatch = true;
    }
    else
    {
      for (unsigned int i = 0; i < mapping.m_PeripheralID.size(); i++)
        if (mapping.m_PeripheralID[i].m_iVendorId == result.m_iVendorId && mapping.m_PeripheralID[i].m_iProductId == result.m_iProductId)
          bProductMatch = true;
    }

    bool bBusMatch = (mapping.m_busType == PERIPHERAL_BUS_UNKNOWN || mapping.m_busType == bus.Type());
    bool bClassMatch = (mapping.m_class == PERIPHERAL_UNKNOWN || mapping.m_class == result.m_type);

    if (bProductMatch && bBusMatch && bClassMatch)
    {
      std::string strVendorId, strProductId;
      PeripheralTypeTranslator::FormatHexString(result.m_iVendorId, strVendorId);
      PeripheralTypeTranslator::FormatHexString(result.m_iProductId, strProductId);
      CLog::Log(LOGDEBUG, "%s - device (%s:%s) mapped to %s (type = %s)", __FUNCTION__, strVendorId.c_str(), strProductId.c_str(), mapping.m_strDeviceName.c_str(), PeripheralTypeTranslator::TypeToString(mapping.m_mappedTo));
      result.m_mappedType    = m_mappings[iMappingPtr].m_mappedTo;
      result.m_strDeviceName = m_mappings[iMappingPtr].m_strDeviceName;
      return true;
    }
  }

  return false;
}

void CPeripherals::GetSettingsFromMapping(CPeripheral &peripheral) const
{
  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (unsigned int iMappingPtr = 0; iMappingPtr < m_mappings.size(); iMappingPtr++)
  {
    const PeripheralDeviceMapping *mapping = &m_mappings.at(iMappingPtr);

    bool bProductMatch = false;
    if (mapping->m_PeripheralID.size() == 0)
    {
      bProductMatch = true;
    }
    else
    {
      for (unsigned int i = 0; i < mapping->m_PeripheralID.size(); i++)
        if (mapping->m_PeripheralID[i].m_iVendorId == peripheral.VendorId() && mapping->m_PeripheralID[i].m_iProductId == peripheral.ProductId())
          bProductMatch = true;
    }

    bool bBusMatch = (mapping->m_busType == PERIPHERAL_BUS_UNKNOWN || mapping->m_busType == peripheral.GetBusType());
    bool bClassMatch = (mapping->m_class == PERIPHERAL_UNKNOWN || mapping->m_class == peripheral.Type());

    if (bBusMatch && bProductMatch && bClassMatch)
    {
      for (map<std::string, PeripheralDeviceSetting>::const_iterator itr = mapping->m_settings.begin(); itr != mapping->m_settings.end(); ++itr)
        peripheral.AddSetting((*itr).first, (*itr).second.m_setting, (*itr).second.m_order);
    }
  }
}

#define SS(x) ((x) ? x : "")
bool CPeripherals::LoadMappings(void)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile("special://xbmc/system/peripherals.xml"))
  {
    CLog::Log(LOGWARNING, "%s - peripherals.xml does not exist", __FUNCTION__);
    return true;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "peripherals") != 0)
  {
    CLog::Log(LOGERROR, "%s - peripherals.xml does not contain <peripherals>", __FUNCTION__);
    return false;
  }

  for (TiXmlElement *currentNode = pRootElement->FirstChildElement("peripheral"); currentNode; currentNode = currentNode->NextSiblingElement("peripheral"))
  {
    PeripheralID id;
    PeripheralDeviceMapping mapping;

    mapping.m_strDeviceName = XMLUtils::GetAttribute(currentNode, "name");

    // If there is no vendor_product attribute ignore this entry
    if (currentNode->Attribute("vendor_product"))
    {
      // The vendor_product attribute is a list of comma separated vendor:product pairs
      vector<string> vpArray = StringUtils::Split(currentNode->Attribute("vendor_product"), ",");
      for (vector<string>::const_iterator i = vpArray.begin(); i != vpArray.end(); ++i)
      {
        vector<string> idArray = StringUtils::Split(*i, ":");
        if (idArray.size() != 2)
        {
          CLog::Log(LOGERROR, "%s - ignoring node \"%s\" with invalid vendor_product attribute", __FUNCTION__, mapping.m_strDeviceName.c_str());
          continue;
        }

        id.m_iVendorId = PeripheralTypeTranslator::HexStringToInt(idArray[0].c_str());
        id.m_iProductId = PeripheralTypeTranslator::HexStringToInt(idArray[1].c_str());
        mapping.m_PeripheralID.push_back(id);
      }
    }

    mapping.m_busType       = PeripheralTypeTranslator::GetBusTypeFromString(XMLUtils::GetAttribute(currentNode, "bus"));
    mapping.m_class         = PeripheralTypeTranslator::GetTypeFromString(XMLUtils::GetAttribute(currentNode, "class"));
    mapping.m_mappedTo      = PeripheralTypeTranslator::GetTypeFromString(XMLUtils::GetAttribute(currentNode, "mapTo"));
    GetSettingsFromMappingsFile(currentNode, mapping.m_settings);

    m_mappings.push_back(mapping);
    CLog::Log(LOGDEBUG, "%s - loaded node \"%s\"", __FUNCTION__, mapping.m_strDeviceName.c_str());
  }

  return true;
}

void CPeripherals::GetSettingsFromMappingsFile(TiXmlElement *xmlNode, map<std::string, PeripheralDeviceSetting> &settings)
{
  TiXmlElement *currentNode = xmlNode->FirstChildElement("setting");
  int iMaxOrder = 0;

  while (currentNode)
  {
    CSetting *setting = NULL;
    std::string strKey = XMLUtils::GetAttribute(currentNode, "key");
    if (strKey.empty())
      continue;

    std::string strSettingsType = XMLUtils::GetAttribute(currentNode, "type");
    int iLabelId = currentNode->Attribute("label") ? atoi(currentNode->Attribute("label")) : -1;
    const std::string config = XMLUtils::GetAttribute(currentNode, "configurable");
    bool bConfigurable = (config.empty() || (config != "no" && config != "false" && config != "0"));
    if (strSettingsType == "bool")
    {
      const std::string value = XMLUtils::GetAttribute(currentNode, "value");
      bool bValue = (value != "no" && value != "false" && value != "0");
      setting = new CSettingBool(strKey, iLabelId, bValue);
    }
    else if (strSettingsType == "int")
    {
      int iValue = currentNode->Attribute("value") ? atoi(currentNode->Attribute("value")) : 0;
      int iMin   = currentNode->Attribute("min") ? atoi(currentNode->Attribute("min")) : 0;
      int iStep  = currentNode->Attribute("step") ? atoi(currentNode->Attribute("step")) : 1;
      int iMax   = currentNode->Attribute("max") ? atoi(currentNode->Attribute("max")) : 255;
      setting = new CSettingInt(strKey, iLabelId, iValue, iMin, iStep, iMax);
    }
    else if (strSettingsType == "float")
    {
      float fValue = currentNode->Attribute("value") ? (float) atof(currentNode->Attribute("value")) : 0;
      float fMin   = currentNode->Attribute("min") ? (float) atof(currentNode->Attribute("min")) : 0;
      float fStep  = currentNode->Attribute("step") ? (float) atof(currentNode->Attribute("step")) : 0;
      float fMax   = currentNode->Attribute("max") ? (float) atof(currentNode->Attribute("max")) : 0;
      setting = new CSettingNumber(strKey, iLabelId, fValue, fMin, fStep, fMax);
    }
    else if (StringUtils::EqualsNoCase(strSettingsType, "enum"))
    {
      std::string strEnums = XMLUtils::GetAttribute(currentNode, "lvalues");
      if (!strEnums.empty())
      {
        vector< pair<int,int> > enums;
        vector<std::string> valuesVec;
        StringUtils::Tokenize(strEnums, valuesVec, "|");
        for (unsigned int i = 0; i < valuesVec.size(); i++)
          enums.push_back(make_pair(atoi(valuesVec[i].c_str()), atoi(valuesVec[i].c_str())));
        int iValue = currentNode->Attribute("value") ? atoi(currentNode->Attribute("value")) : 0;
        setting = new CSettingInt(strKey, iLabelId, iValue, enums);
      }
    }
    else
    {
      std::string strValue = XMLUtils::GetAttribute(currentNode, "value");
      setting = new CSettingString(strKey, iLabelId, strValue);
    }

    if (setting)
    {
      //TODO add more types if needed

      /* set the visibility */
      setting->SetVisible(bConfigurable);

      /* set the order */
      int iOrder = 0;
      currentNode->Attribute("order", &iOrder);
      /* if the order attribute is invalid or 0, then the setting will be added at the end */
      if (iOrder < 0)
        iOrder = 0;
      if (iOrder > iMaxOrder)
       iMaxOrder = iOrder;

      /* and add this new setting */
      PeripheralDeviceSetting deviceSetting = { setting, iOrder };
      settings[strKey] = deviceSetting;
    }

    currentNode = currentNode->NextSiblingElement("setting");
  }

  /* add the settings without an order attribute or an invalid order attribute set at the end */
  for (map<std::string, PeripheralDeviceSetting>::iterator it = settings.begin(); it != settings.end(); ++it)
  {
    if (it->second.m_order == 0)
      it->second.m_order = ++iMaxOrder;
  }
}

void CPeripherals::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (StringUtils::EqualsNoCase(strBus, "all") ||
        StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(m_busses.at(iBusPtr)->Type())))
      m_busses.at(iBusPtr)->GetDirectory(strPath, items);
  }
}

CPeripheral *CPeripherals::GetByPath(const std::string &strPath) const
{
  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return NULL;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(m_busses.at(iBusPtr)->Type())))
      return m_busses.at(iBusPtr)->GetByPath(strPath);
  }

  return NULL;
}

bool CPeripherals::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_MUTE)
  {
    return ToggleMute();
  }

  if (SupportsCEC() && action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN))
  {
    vector<CPeripheral *> peripherals;
    if (GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
    {
      for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < peripherals.size(); iPeripheralPtr++)
      {
        CPeripheralCecAdapter *cecDevice = (CPeripheralCecAdapter *) peripherals.at(iPeripheralPtr);
        if (cecDevice && cecDevice->HasAudioControl())
        {
          if (action.GetID() == ACTION_VOLUME_UP)
            cecDevice->VolumeUp();
          else
            cecDevice->VolumeDown();
          return true;
        }
      }
    }
  }

  return false;
}

bool CPeripherals::IsMuted(void)
{
  vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < peripherals.size(); iPeripheralPtr++)
    {
      CPeripheralCecAdapter *cecDevice = (CPeripheralCecAdapter *) peripherals.at(iPeripheralPtr);
      if (cecDevice && cecDevice->IsMuted())
        return true;
    }
  }

  return false;
}

bool CPeripherals::ToggleMute(void)
{
  vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < peripherals.size(); iPeripheralPtr++)
    {
      CPeripheralCecAdapter *cecDevice = (CPeripheralCecAdapter *) peripherals.at(iPeripheralPtr);
      if (cecDevice && cecDevice->HasAudioControl())
      {
        cecDevice->ToggleMute();
        return true;
      }
    }
  }

  return false;
}

bool CPeripherals::ToggleDeviceState(CecStateChange mode /*= STATE_SWITCH_TOGGLE */, unsigned int iPeripheral /*= 0 */)
{
  bool ret(false);
  vector<CPeripheral *> peripherals;

  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (unsigned int iPeripheralPtr = iPeripheral; iPeripheralPtr < peripherals.size(); iPeripheralPtr++)
    {
      CPeripheralCecAdapter *cecDevice = (CPeripheralCecAdapter *) peripherals.at(iPeripheralPtr);
      if (cecDevice)
        ret = cecDevice->ToggleDeviceState(mode);
      if (iPeripheral)
        break;
    }
  }

  return ret;
}

bool CPeripherals::GetNextKeypress(float frameTime, CKey &key)
{
  vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < peripherals.size(); iPeripheralPtr++)
    {
      CPeripheralCecAdapter *cecDevice = (CPeripheralCecAdapter *) peripherals.at(iPeripheralPtr);
      if (cecDevice && cecDevice->GetButton())
      {
        CKey newKey(cecDevice->GetButton(), cecDevice->GetHoldTime());
        cecDevice->ResetButton();
        key = newKey;
        return true;
      }
    }
  }

  return false;
}

void CPeripherals::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "locale.language")
  {
    // user set language, no longer use the TV's language
    vector<CPeripheral *> cecDevices;
    if (g_peripherals.GetPeripheralsWithFeature(cecDevices, FEATURE_CEC) > 0)
    {
      for (vector<CPeripheral *>::iterator it = cecDevices.begin(); it != cecDevices.end(); ++it)
        (*it)->SetSetting("use_tv_menu_language", false);
    }
  }
}

void CPeripherals::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "input.peripherals")
  {
    CGUIDialogPeripheralManager *dialog = (CGUIDialogPeripheralManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_MANAGER);
    if (dialog != NULL)
      dialog->DoModal();
  }
}
