/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Peripherals.h"

#include <utility>

#include "addons/PeripheralAddon.h"
#include "addons/AddonButtonMap.h"
#include "bus/PeripheralBus.h"
#include "bus/PeripheralBusUSB.h"
#if defined(TARGET_ANDROID)
#include "bus/android/PeripheralBusAndroid.h"
#endif
#include "bus/virtual/PeripheralBusAddon.h"
#include "devices/PeripheralBluetooth.h"
#include "devices/PeripheralCecAdapter.h"
#include "devices/PeripheralDisk.h"
#include "devices/PeripheralHID.h"
#include "devices/PeripheralImon.h"
#include "devices/PeripheralJoystick.h"
#include "devices/PeripheralNIC.h"
#include "devices/PeripheralNyxboard.h"
#include "devices/PeripheralTuner.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogPeripheralSettings.h"
#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "GUIUserMessages.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#if defined(HAVE_LIBCEC)
#include "bus/virtual/PeripheralBusCEC.h"
#endif


using namespace JOYSTICK;
using namespace PERIPHERALS;
using namespace XFILE;

CPeripherals::CPeripherals() :
  m_eventScanner(this)
{
  Clear();
}

CPeripherals::~CPeripherals()
{
  Clear();
}

CPeripherals &CPeripherals::GetInstance()
{
  static CPeripherals peripheralsInstance;
  return peripheralsInstance;
}

void CPeripherals::Initialise()
{
  CSingleLock lock(m_critSection);
  if (m_bIsStarted)
    return;

  m_bIsStarted = true;

  CDirectory::Create("special://profile/peripheral_data");

  /* load mappings from peripherals.xml */
  LoadMappings();

  std::vector<PeripheralBusPtr> busses;

#if defined(HAVE_PERIPHERAL_BUS_USB)
  busses.push_back(std::make_shared<CPeripheralBusUSB>(this));
#endif
#if defined(HAVE_LIBCEC)
  busses.push_back(std::make_shared<CPeripheralBusCEC>(this));
#endif
  busses.push_back(std::make_shared<CPeripheralBusAddon>(this));
#if defined(TARGET_ANDROID)
  busses.push_back(std::make_shared<CPeripheralBusAndroid>(this));
#endif

  /* initialise all known busses and run an initial scan for devices */
  for (auto& bus : busses)
    bus->Initialise();

  {
    CSingleLock bussesLock(m_critSectionBusses);
    m_busses = std::move(busses);
  }

  m_eventScanner.Start();

  m_bInitialised = true;
  KODI::MESSAGING::CApplicationMessenger::GetInstance().RegisterReceiver(this);
}

void CPeripherals::Clear()
{
  m_eventScanner.Stop();

  // avoid deadlocks by copying all busses into a temporary variable and destroying them from there
  std::vector<PeripheralBusPtr> busses;
  {
    CSingleLock bussesLock(m_critSectionBusses);
    /* delete busses and devices */
    busses = m_busses;
    m_busses.clear();
  }
  busses.clear();

  {
    CSingleLock mappingsLock(m_critSectionMappings);
    /* delete mappings */
    for (auto& mapping : m_mappings)
    {
      std::map<std::string, PeripheralDeviceSetting> settings = mapping.m_settings;
      for (const auto& setting : mapping.m_settings)
        delete setting.second.m_setting;
      mapping.m_settings.clear();
    }
    m_mappings.clear();
  }

  CSingleLock lock(m_critSection);
  /* reset class state */
  m_bIsStarted   = false;
  m_bInitialised = false;
#if !defined(HAVE_LIBCEC)
  m_bMissingLibCecWarningDisplayed = false;
#endif
}

void CPeripherals::TriggerDeviceScan(const PeripheralBusType type /* = PERIPHERAL_BUS_UNKNOWN */)
{
  std::vector<PeripheralBusPtr> busses;
  {
    CSingleLock lock(m_critSectionBusses);
    busses = m_busses;
  }

  for (auto& bus : busses)
  {
    bool bScan = false;

    if (type == PERIPHERAL_BUS_UNKNOWN)
      bScan = true;
    else if (bus->Type() == PERIPHERAL_BUS_ADDON)
      bScan = true;
    else if (bus->Type() == type)
      bScan = true;

    if (bScan)
      bus->TriggerDeviceScan();
  }
}

PeripheralBusPtr CPeripherals::GetBusByType(const PeripheralBusType type) const
{
  CSingleLock lock(m_critSectionBusses);

  const auto& bus = std::find_if(m_busses.cbegin(), m_busses.cend(),
    [type](const PeripheralBusPtr& bus) {
      return bus->Type() == type;
    });
  if (bus != m_busses.cend())
    return *bus;

  return nullptr;
}

CPeripheral *CPeripherals::GetPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  CSingleLock lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    /* check whether the bus matches if a bus type other than unknown was passed */
    if (busType != PERIPHERAL_BUS_UNKNOWN && bus->Type() != busType)
      continue;

    /* return the first device that matches */
    CPeripheral* peripheral = bus->GetPeripheral(strLocation);
    if (peripheral != nullptr)
      return peripheral;
  }

  return nullptr;
}

bool CPeripherals::HasPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  return (GetPeripheralAtLocation(strLocation, busType) != nullptr);
}

PeripheralBusPtr CPeripherals::GetBusWithDevice(const std::string &strLocation) const
{
  CSingleLock lock(m_critSectionBusses);

  const auto& bus = std::find_if(m_busses.cbegin(), m_busses.cend(),
    [&strLocation](const PeripheralBusPtr& bus) {
    return bus->HasPeripheral(strLocation);
  });
  if (bus != m_busses.cend())
    return *bus;

  return nullptr;
}

int CPeripherals::GetPeripheralsWithFeature(std::vector<CPeripheral *> &results, const PeripheralFeature feature, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  CSingleLock lock(m_critSectionBusses);
  int iReturn(0);
  for (const auto& bus : m_busses)
  {
    /* check whether the bus matches if a bus type other than unknown was passed */
    if (busType != PERIPHERAL_BUS_UNKNOWN && bus->Type() != busType)
      continue;

    iReturn += bus->GetPeripheralsWithFeature(results, feature);
  }

  return iReturn;
}

size_t CPeripherals::GetNumberOfPeripherals() const
{
  size_t iReturn(0);
  CSingleLock lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
    iReturn += bus->GetNumberOfPeripherals();

  return iReturn;
}

bool CPeripherals::HasPeripheralWithFeature(const PeripheralFeature feature, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  std::vector<CPeripheral *> dummy;
  return (GetPeripheralsWithFeature(dummy, feature, busType) > 0);
}

CPeripheral *CPeripherals::CreatePeripheral(CPeripheralBus &bus, const PeripheralScanResult& result)
{
  CPeripheral *peripheral = nullptr;
  PeripheralScanResult mappedResult = result;
  if (mappedResult.m_busType == PERIPHERAL_BUS_UNKNOWN)
    mappedResult.m_busType = bus.Type();

  /* check whether there's something mapped in peripherals.xml */
  GetMappingForDevice(bus, mappedResult);

  switch(mappedResult.m_mappedType)
  {
  case PERIPHERAL_HID:
    peripheral = new CPeripheralHID(mappedResult, &bus);
    break;

  case PERIPHERAL_NIC:
    peripheral = new CPeripheralNIC(mappedResult, &bus);
    break;

  case PERIPHERAL_DISK:
    peripheral = new CPeripheralDisk(mappedResult, &bus);
    break;

  case PERIPHERAL_NYXBOARD:
    peripheral = new CPeripheralNyxboard(mappedResult, &bus);
    break;

  case PERIPHERAL_TUNER:
    peripheral = new CPeripheralTuner(mappedResult, &bus);
    break;

  case PERIPHERAL_BLUETOOTH:
    peripheral = new CPeripheralBluetooth(mappedResult, &bus);
    break;

  case PERIPHERAL_CEC:
#if defined(HAVE_LIBCEC)
    if (bus.Type() == PERIPHERAL_BUS_CEC)
      peripheral = new CPeripheralCecAdapter(mappedResult, &bus);
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
    peripheral = new CPeripheralImon(mappedResult, &bus);
    break;

  case PERIPHERAL_JOYSTICK:
    peripheral = new CPeripheralJoystick(mappedResult, &bus);
    break;

  default:
    break;
  }

  if (peripheral)
  {
    /* try to initialise the new peripheral
     * Initialise() will make sure that each device is only initialised once */
    if (peripheral->Initialise())
      bus.Register(peripheral);
    else
    {
      CLog::Log(LOGDEBUG, "%s - failed to initialise peripheral on '%s'", __FUNCTION__, mappedResult.m_strLocation.c_str());
      delete peripheral;
      peripheral = nullptr;
    }
  }

  return peripheral;
}

void CPeripherals::OnDeviceAdded(const CPeripheralBus &bus, const CPeripheral &peripheral)
{
  OnDeviceChanged();

  // don't show a notification for devices detected during the initial scan
  if (bus.IsInitialised())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35005), peripheral.DeviceName());
}

void CPeripherals::OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral)
{
  OnDeviceChanged();

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35006), peripheral.DeviceName());
}

void CPeripherals::OnDeviceChanged()
{
  // refresh settings (peripherals manager could be enabled/disabled now)
  CGUIMessage msgSettings(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
  g_windowManager.SendThreadMessage(msgSettings, WINDOW_SETTINGS_SYSTEM);

  SetChanged();
}

bool CPeripherals::GetMappingForDevice(const CPeripheralBus &bus, PeripheralScanResult& result) const
{
  CSingleLock lock(m_critSectionMappings);

  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (const auto& mapping : m_mappings)
  {
    bool bProductMatch = false;
    if (mapping.m_PeripheralID.empty())
      bProductMatch = true;
    else
    {
      for (const auto& peripheralID : mapping.m_PeripheralID)
        if (peripheralID.m_iVendorId == result.m_iVendorId && peripheralID.m_iProductId == result.m_iProductId)
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
      result.m_mappedType    = mapping.m_mappedTo;
      if (!mapping.m_strDeviceName.empty())
        result.m_strDeviceName = mapping.m_strDeviceName;
      return true;
    }
  }

  return false;
}

void CPeripherals::GetSettingsFromMapping(CPeripheral &peripheral) const
{
  CSingleLock lock(m_critSectionMappings);

  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (const auto& mapping : m_mappings)
  {
    bool bProductMatch = false;
    if (mapping.m_PeripheralID.empty())
      bProductMatch = true;
    else
    {
      for (const auto& peripheralID : mapping.m_PeripheralID)
        if (peripheralID.m_iVendorId == peripheral.VendorId() && peripheralID.m_iProductId == peripheral.ProductId())
          bProductMatch = true;
    }

    bool bBusMatch = (mapping.m_busType == PERIPHERAL_BUS_UNKNOWN || mapping.m_busType == peripheral.GetBusType());
    bool bClassMatch = (mapping.m_class == PERIPHERAL_UNKNOWN || mapping.m_class == peripheral.Type());

    if (bBusMatch && bProductMatch && bClassMatch)
    {
      for (auto itr = mapping.m_settings.begin(); itr != mapping.m_settings.end(); ++itr)
        peripheral.AddSetting((*itr).first, (*itr).second.m_setting, (*itr).second.m_order);
    }
  }
}

#define SS(x) ((x) ? x : "")
bool CPeripherals::LoadMappings()
{
  CSingleLock lock(m_critSectionMappings);

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
      std::vector<std::string> vpArray = StringUtils::Split(currentNode->Attribute("vendor_product"), ",");
      for (const auto& i : vpArray)
      {
        std::vector<std::string> idArray = StringUtils::Split(i, ":");
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

void CPeripherals::GetSettingsFromMappingsFile(TiXmlElement *xmlNode, std::map<std::string, PeripheralDeviceSetting> &settings)
{
  TiXmlElement *currentNode = xmlNode->FirstChildElement("setting");
  int iMaxOrder = 0;

  while (currentNode)
  {
    CSetting *setting = nullptr;
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
        std::vector< std::pair<int,int> > enums;
        std::vector<std::string> valuesVec;
        StringUtils::Tokenize(strEnums, valuesVec, "|");
        for (unsigned int i = 0; i < valuesVec.size(); i++)
          enums.push_back(std::make_pair(atoi(valuesVec[i].c_str()), atoi(valuesVec[i].c_str())));
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
      //! @todo add more types if needed

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
  for (auto& it : settings)
  {
    if (it.second.m_order == 0)
      it.second.m_order = ++iMaxOrder;
  }
}

void CPeripherals::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  CSingleLock lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    if (StringUtils::EqualsNoCase(strBus, "all") ||
        StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(bus->Type())))
      bus->GetDirectory(strPath, items);
  }
}

CPeripheral *CPeripherals::GetByPath(const std::string &strPath) const
{
  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return nullptr;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  CSingleLock lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    if (StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(bus->Type())))
      return bus->GetByPath(strPath);
  }

  return nullptr;
}

bool CPeripherals::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_MUTE)
  {
    return ToggleMute();
  }

  if (SupportsCEC() && action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN))
  {
    std::vector<CPeripheral *> peripherals;
    if (GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
    {
      for (auto& peripheral : peripherals)
      {
        CPeripheralCecAdapter *cecDevice = reinterpret_cast<CPeripheralCecAdapter*>(peripheral);
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

bool CPeripherals::IsMuted()
{
  std::vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (const auto& peripheral : peripherals)
    {
      CPeripheralCecAdapter *cecDevice = reinterpret_cast<CPeripheralCecAdapter*>(peripheral);
      if (cecDevice && cecDevice->IsMuted())
        return true;
    }
  }

  return false;
}

bool CPeripherals::ToggleMute()
{
  std::vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      CPeripheralCecAdapter *cecDevice = reinterpret_cast<CPeripheralCecAdapter*>(peripheral);
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
  std::vector<CPeripheral *> peripherals;

  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      CPeripheralCecAdapter *cecDevice = reinterpret_cast<CPeripheralCecAdapter*>(peripheral);
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
  std::vector<CPeripheral *> peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      CPeripheralCecAdapter *cecDevice = reinterpret_cast<CPeripheralCecAdapter*>(peripheral);
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

void CPeripherals::OnUserNotification()
{
  std::vector<CPeripheral*> peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_RUMBLE);

  for (CPeripheral* peripheral : peripherals)
    peripheral->OnUserNotification();
}

bool CPeripherals::TestFeature(PeripheralFeature feature)
{
  std::vector<CPeripheral*> peripherals;
  GetPeripheralsWithFeature(peripherals, feature);

  if (!peripherals.empty())
  {
    for (CPeripheral* peripheral : peripherals)
      peripheral->TestFeature(feature);
    return true;
  }
  return false;
}

void CPeripherals::ProcessEvents(void)
{
  std::vector<PeripheralBusPtr> busses;
  {
    CSingleLock lock(m_critSectionBusses);
    busses = m_busses;
  }

  for (PeripheralBusPtr& bus : busses)
    bus->ProcessEvents();
}

PeripheralAddonPtr CPeripherals::GetAddonWithButtonMap(const CPeripheral* device)
{
  PeripheralBusAddonPtr addonBus = std::static_pointer_cast<CPeripheralBusAddon>(GetBusByType(PERIPHERAL_BUS_ADDON));

  PeripheralAddonPtr addon;

  PeripheralAddonPtr addonWithButtonMap;
  if (addonBus && addonBus->GetAddonWithButtonMap(device, addonWithButtonMap))
    addon = std::move(addonWithButtonMap);

  return addon;
}

void CPeripherals::ResetButtonMaps(const std::string& controllerId)
{
  PeripheralBusAddonPtr addonBus = std::static_pointer_cast<CPeripheralBusAddon>(GetBusByType(PERIPHERAL_BUS_ADDON));

  std::vector<CPeripheral*> peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);

  for (auto& peripheral : peripherals)
  {
    PeripheralAddonPtr addon;
    if (addonBus->GetAddonWithButtonMap(peripheral, addon))
    {
      CAddonButtonMap buttonMap(peripheral, addon, controllerId);
      buttonMap.Reset();
    }
  }
}

void CPeripherals::RegisterJoystickButtonMapper(IButtonMapper* mapper)
{
  std::vector<CPeripheral*> peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);

  for (auto& peripheral : peripherals)
    peripheral->RegisterJoystickButtonMapper(mapper);
}

void CPeripherals::UnregisterJoystickButtonMapper(IButtonMapper* mapper)
{
  std::vector<CPeripheral*> peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);

  for (auto& peripheral : peripherals)
    peripheral->UnregisterJoystickButtonMapper(mapper);
}

void CPeripherals::OnSettingChanged(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    // user set language, no longer use the TV's language
    std::vector<CPeripheral *> cecDevices;
    if (GetPeripheralsWithFeature(cecDevices, FEATURE_CEC) > 0)
    {
      for (auto& cecDevice : cecDevices)
        cecDevice->SetSetting("use_tv_menu_language", false);
    }
  }
}

void CPeripherals::OnSettingAction(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_INPUT_PERIPHERALS)
  {
    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    CFileItemList items;
    GetDirectory("peripherals://all/", items);

    int iPos = -1;
    do
    {
      pDialog->Reset();
      pDialog->SetHeading(CVariant{35000});
      pDialog->SetUseDetails(true);
      pDialog->SetItems(items);
      pDialog->SetSelected(iPos);
      pDialog->Open();

      iPos = pDialog->IsConfirmed() ? pDialog->GetSelectedItem() : -1;

      if (iPos >= 0)
      {
        CFileItemPtr pItem = items.Get(iPos);

        // show an error if the peripheral doesn't have any settings
        CPeripheral *peripheral = GetByPath(pItem->GetPath());
        if (peripheral == nullptr || peripheral->GetSettings().empty())
        {
          CGUIDialogOK::ShowAndGetInput(CVariant{35000}, CVariant{35004});
          continue;
        }

        CGUIDialogPeripheralSettings *pSettingsDialog = (CGUIDialogPeripheralSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_SETTINGS);
        if (pItem && pSettingsDialog)
        {
          // pass peripheral item properties to settings dialog so skin authors
          // can use it to show more detailed information about the device
          pSettingsDialog->SetProperty("vendor", pItem->GetProperty("vendor"));
          pSettingsDialog->SetProperty("product", pItem->GetProperty("product"));
          pSettingsDialog->SetProperty("bus", pItem->GetProperty("bus"));
          pSettingsDialog->SetProperty("location", pItem->GetProperty("location"));
          pSettingsDialog->SetProperty("class", pItem->GetProperty("class"));
          pSettingsDialog->SetProperty("version", pItem->GetProperty("version"));

          // open settings dialog
          pSettingsDialog->SetFileItem(pItem.get());
          pSettingsDialog->Open();
        }
      }
    } while (pDialog->IsConfirmed());
  }
  else if (settingId == CSettings::SETTING_INPUT_CONTROLLERCONFIG)
    g_windowManager.ActivateWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
  else if (settingId == CSettings::SETTING_INPUT_TESTRUMBLE)
    TestFeature(FEATURE_RUMBLE);
}

void CPeripherals::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_CECTOGGLESTATE:
    *static_cast<bool*>(pMsg->lpVoid) = ToggleDeviceState(STATE_SWITCH_TOGGLE);
    break;

  case TMSG_CECACTIVATESOURCE:
    ToggleDeviceState(STATE_ACTIVATE_SOURCE);
    break;

  case TMSG_CECSTANDBY:
    ToggleDeviceState(STATE_STANDBY);
    break;
  }
}

int CPeripherals::GetMessageMask()
{
  return TMSG_MASK_PERIPHERALS;
}
