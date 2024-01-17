/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Peripherals.h"

#include "CompileInfo.h"
#include "addons/AddonButtonMap.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "bus/PeripheralBus.h"
#include "bus/PeripheralBusUSB.h"

#include <mutex>
#include <utility>
#if defined(TARGET_ANDROID)
#include "platform/android/peripherals/PeripheralBusAndroid.h"
#elif defined(TARGET_DARWIN)
#include "platform/darwin/peripherals/PeripheralBusGCController.h"
#endif
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "bus/virtual/PeripheralBusAddon.h"
#include "bus/virtual/PeripheralBusApplication.h"
#include "devices/PeripheralBluetooth.h"
#include "devices/PeripheralCecAdapter.h"
#include "devices/PeripheralDisk.h"
#include "devices/PeripheralHID.h"
#include "devices/PeripheralImon.h"
#include "devices/PeripheralJoystick.h"
#include "devices/PeripheralKeyboard.h"
#include "devices/PeripheralMouse.h"
#include "devices/PeripheralNIC.h"
#include "devices/PeripheralNyxboard.h"
#include "devices/PeripheralTuner.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "input/keyboard/Key.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "peripherals/dialogs/GUIDialogPeripherals.h"
#include "peripherals/events/EventScanner.h"
#include "settings/SettingAddon.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#if defined(HAVE_LIBCEC)
#include "bus/virtual/PeripheralBusCEC.h"
#else
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#endif

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;
using namespace XFILE;

CPeripherals::CPeripherals(CInputManager& inputManager,
                           GAME::CControllerManager& controllerProfiles)
  : m_inputManager(inputManager),
    m_controllerProfiles(controllerProfiles),
    m_eventScanner(new CEventScanner(*this))
{
  // Register settings
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_INPUT_PERIPHERALS);
  settingSet.insert(CSettings::SETTING_INPUT_PERIPHERALLIBRARIES);
  settingSet.insert(CSettings::SETTING_INPUT_CONTROLLERCONFIG);
  settingSet.insert(CSettings::SETTING_INPUT_TESTRUMBLE);
  settingSet.insert(CSettings::SETTING_LOCALE_LANGUAGE);
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, settingSet);
}

CPeripherals::~CPeripherals()
{
  // Unregister settings
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);

  Clear();
}

void CPeripherals::Initialise()
{
  Clear();

  CDirectory::Create("special://profile/peripheral_data");

  /* load mappings from peripherals.xml */
  LoadMappings();

  std::vector<PeripheralBusPtr> busses;

#if defined(HAVE_PERIPHERAL_BUS_USB)
  busses.push_back(std::make_shared<CPeripheralBusUSB>(*this));
#endif
#if defined(HAVE_LIBCEC)
  busses.push_back(std::make_shared<CPeripheralBusCEC>(*this));
#endif
  busses.push_back(std::make_shared<CPeripheralBusAddon>(*this));
#if defined(TARGET_ANDROID)
  busses.push_back(std::make_shared<CPeripheralBusAndroid>(*this));
#elif defined(TARGET_DARWIN)
  busses.push_back(std::make_shared<CPeripheralBusGCController>(*this));
#endif
  busses.push_back(std::make_shared<CPeripheralBusApplication>(*this));

  {
    std::unique_lock<CCriticalSection> bussesLock(m_critSectionBusses);
    m_busses = busses;
  }

  /* initialise all known busses and run an initial scan for devices */
  for (auto& bus : busses)
    bus->Initialise();

  m_eventScanner->Start();

  CServiceBroker::GetAppMessenger()->RegisterReceiver(this);
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

void CPeripherals::Clear()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  m_eventScanner->Stop();

  // avoid deadlocks by copying all busses into a temporary variable and destroying them from there
  std::vector<PeripheralBusPtr> busses;
  {
    std::unique_lock<CCriticalSection> bussesLock(m_critSectionBusses);
    /* delete busses and devices */
    busses = m_busses;
    m_busses.clear();
  }

  for (const auto& bus : busses)
    bus->Clear();
  busses.clear();

  {
    std::unique_lock<CCriticalSection> mappingsLock(m_critSectionMappings);
    /* delete mappings */
    for (auto& mapping : m_mappings)
      mapping.m_settings.clear();
    m_mappings.clear();
  }

#if !defined(HAVE_LIBCEC)
  m_bMissingLibCecWarningDisplayed = false;
#endif
}

void CPeripherals::TriggerDeviceScan(const PeripheralBusType type /* = PERIPHERAL_BUS_UNKNOWN */)
{
  std::vector<PeripheralBusPtr> busses;
  {
    std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
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
  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);

  const auto& bus =
      std::find_if(m_busses.cbegin(), m_busses.cend(),
                   [type](const PeripheralBusPtr& bus) { return bus->Type() == type; });
  if (bus != m_busses.cend())
    return *bus;

  return nullptr;
}

PeripheralPtr CPeripherals::GetPeripheralAtLocation(
    const std::string& strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  PeripheralPtr result;

  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    /* check whether the bus matches if a bus type other than unknown was passed */
    if (busType != PERIPHERAL_BUS_UNKNOWN && bus->Type() != busType)
      continue;

    /* return the first device that matches */
    PeripheralPtr peripheral = bus->GetPeripheral(strLocation);
    if (peripheral)
    {
      result = peripheral;
      break;
    }
  }

  return result;
}

bool CPeripherals::HasPeripheralAtLocation(
    const std::string& strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  return (GetPeripheralAtLocation(strLocation, busType) != nullptr);
}

PeripheralBusPtr CPeripherals::GetBusWithDevice(const std::string& strLocation) const
{
  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);

  const auto& bus = std::find_if(m_busses.cbegin(), m_busses.cend(),
                                 [&strLocation](const PeripheralBusPtr& bus)
                                 { return bus->HasPeripheral(strLocation); });
  if (bus != m_busses.cend())
    return *bus;

  return nullptr;
}

bool CPeripherals::SupportsFeature(PeripheralFeature feature) const
{
  bool bSupportsFeature = false;

  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
    bSupportsFeature |= bus->SupportsFeature(feature);

  return bSupportsFeature;
}

int CPeripherals::GetPeripheralsWithFeature(
    PeripheralVector& results,
    const PeripheralFeature feature,
    PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
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
  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
    iReturn += bus->GetNumberOfPeripherals();

  return iReturn;
}

bool CPeripherals::HasPeripheralWithFeature(
    const PeripheralFeature feature, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  PeripheralVector dummy;
  return (GetPeripheralsWithFeature(dummy, feature, busType) > 0);
}

void CPeripherals::CreatePeripheral(CPeripheralBus& bus, const PeripheralScanResult& result)
{
  PeripheralPtr peripheral;
  PeripheralScanResult mappedResult = result;
  if (mappedResult.m_busType == PERIPHERAL_BUS_UNKNOWN)
    mappedResult.m_busType = bus.Type();

  /* check whether there's something mapped in peripherals.xml */
  GetMappingForDevice(bus, mappedResult);

  switch (mappedResult.m_mappedType)
  {
    case PERIPHERAL_HID:
      peripheral = PeripheralPtr(new CPeripheralHID(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_NIC:
      peripheral = PeripheralPtr(new CPeripheralNIC(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_DISK:
      peripheral = PeripheralPtr(new CPeripheralDisk(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_NYXBOARD:
      peripheral = PeripheralPtr(new CPeripheralNyxboard(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_TUNER:
      peripheral = PeripheralPtr(new CPeripheralTuner(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_BLUETOOTH:
      peripheral = PeripheralPtr(new CPeripheralBluetooth(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_CEC:
#if defined(HAVE_LIBCEC)
      if (bus.Type() == PERIPHERAL_BUS_CEC)
        peripheral = PeripheralPtr(new CPeripheralCecAdapter(*this, mappedResult, &bus));
#else
      if (!m_bMissingLibCecWarningDisplayed)
      {
        m_bMissingLibCecWarningDisplayed = true;
        CLog::Log(
            LOGWARNING,
            "{} - libCEC support has not been compiled in, so the CEC adapter cannot be used.",
            __FUNCTION__);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning,
                                              g_localizeStrings.Get(36000),
                                              g_localizeStrings.Get(36017));
      }
#endif
      break;

    case PERIPHERAL_IMON:
      peripheral = PeripheralPtr(new CPeripheralImon(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_JOYSTICK:
      peripheral = PeripheralPtr(new CPeripheralJoystick(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_KEYBOARD:
      peripheral = PeripheralPtr(new CPeripheralKeyboard(*this, mappedResult, &bus));
      break;

    case PERIPHERAL_MOUSE:
      peripheral = PeripheralPtr(new CPeripheralMouse(*this, mappedResult, &bus));
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
      CLog::Log(LOGDEBUG, "{} - failed to initialise peripheral on '{}'", __FUNCTION__,
                mappedResult.m_strLocation);
    }
  }
}

void CPeripherals::OnDeviceAdded(const CPeripheralBus& bus, const CPeripheral& peripheral)
{
  OnDeviceChanged();

  //! @todo Improve device notifications in v18
#if 0
  bool bNotify = true;

  // don't show a notification for devices detected during the initial scan
  if (!bus.IsInitialised())
    bNotify = false;

  if (bNotify)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35005), peripheral.DeviceName());
#endif
}

void CPeripherals::OnDeviceDeleted(const CPeripheralBus& bus, const CPeripheral& peripheral)
{
  OnDeviceChanged();

  //! @todo Improve device notifications in v18
#if 0
  bool bNotify = true;

  if (bNotify)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35006), peripheral.DeviceName());
#endif
}

void CPeripherals::OnDeviceChanged()
{
  // refresh settings (peripherals manager could be enabled/disabled now)
  CGUIMessage msgSettings(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msgSettings,
                                                                 WINDOW_SETTINGS_SYSTEM);

  SetChanged();
}

bool CPeripherals::GetMappingForDevice(const CPeripheralBus& bus,
                                       PeripheralScanResult& result) const
{
  std::unique_lock<CCriticalSection> lock(m_critSectionMappings);

  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (const auto& mapping : m_mappings)
  {
    bool bProductMatch = false;
    if (mapping.m_PeripheralID.empty())
      bProductMatch = true;
    else
    {
      for (const auto& peripheralID : mapping.m_PeripheralID)
        if (peripheralID.m_iVendorId == result.m_iVendorId &&
            peripheralID.m_iProductId == result.m_iProductId)
          bProductMatch = true;
    }

    bool bBusMatch =
        (mapping.m_busType == PERIPHERAL_BUS_UNKNOWN || mapping.m_busType == bus.Type());
    bool bClassMatch = (mapping.m_class == PERIPHERAL_UNKNOWN || mapping.m_class == result.m_type);

    if (bProductMatch && bBusMatch && bClassMatch)
    {
      std::string strVendorId, strProductId;
      PeripheralTypeTranslator::FormatHexString(result.m_iVendorId, strVendorId);
      PeripheralTypeTranslator::FormatHexString(result.m_iProductId, strProductId);
      CLog::Log(LOGDEBUG, "{} - device ({}:{}) mapped to {} (type = {})", __FUNCTION__, strVendorId,
                strProductId, mapping.m_strDeviceName,
                PeripheralTypeTranslator::TypeToString(mapping.m_mappedTo));
      result.m_mappedType = mapping.m_mappedTo;
      if (result.m_strDeviceName.empty() && !mapping.m_strDeviceName.empty())
        result.m_strDeviceName = mapping.m_strDeviceName;
      return true;
    }
  }

  return false;
}

void CPeripherals::GetSettingsFromMapping(CPeripheral& peripheral) const
{
  std::unique_lock<CCriticalSection> lock(m_critSectionMappings);

  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (const auto& mapping : m_mappings)
  {
    bool bProductMatch = false;
    if (mapping.m_PeripheralID.empty())
      bProductMatch = true;
    else
    {
      for (const auto& peripheralID : mapping.m_PeripheralID)
        if (peripheralID.m_iVendorId == peripheral.VendorId() &&
            peripheralID.m_iProductId == peripheral.ProductId())
          bProductMatch = true;
    }

    bool bBusMatch = (mapping.m_busType == PERIPHERAL_BUS_UNKNOWN ||
                      mapping.m_busType == peripheral.GetBusType());
    bool bClassMatch =
        (mapping.m_class == PERIPHERAL_UNKNOWN || mapping.m_class == peripheral.Type());

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
  std::unique_lock<CCriticalSection> lock(m_critSectionMappings);

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile("special://xbmc/system/peripherals.xml"))
  {
    CLog::LogF(LOGWARNING, "peripherals.xml does not exist");
    return true;
  }

  auto* pRootElement = xmlDoc.RootElement();
  if (pRootElement == nullptr ||
      StringUtils::CompareNoCase(pRootElement->Value(), "peripherals") != 0)
  {
    CLog::LogF(LOGERROR, "peripherals.xml does not contain <peripherals>");
    return false;
  }

  for (auto* currentNode = pRootElement->FirstChildElement("peripheral"); currentNode != nullptr;
       currentNode = currentNode->NextSiblingElement("peripheral"))
  {
    PeripheralID id;
    PeripheralDeviceMapping mapping;

    mapping.m_strDeviceName = XMLUtils::GetAttribute(currentNode, "name");

    // If there is no vendor_product attribute ignore this entry
    if (currentNode->Attribute("vendor_product"))
    {
      // The vendor_product attribute is a list of comma separated vendor:product pairs
      std::vector<std::string> vpArray =
          StringUtils::Split(currentNode->Attribute("vendor_product"), ",");
      for (const auto& i : vpArray)
      {
        std::vector<std::string> idArray = StringUtils::Split(i, ":");
        if (idArray.size() != 2)
        {
          CLog::LogF(LOGERROR, "ignoring node \"{}\" with invalid vendor_product attribute",
                     mapping.m_strDeviceName);
          continue;
        }

        id.m_iVendorId = PeripheralTypeTranslator::HexStringToInt(idArray[0].c_str());
        id.m_iProductId = PeripheralTypeTranslator::HexStringToInt(idArray[1].c_str());
        mapping.m_PeripheralID.push_back(id);
      }
    }

    mapping.m_busType =
        PeripheralTypeTranslator::GetBusTypeFromString(XMLUtils::GetAttribute(currentNode, "bus"));
    mapping.m_class =
        PeripheralTypeTranslator::GetTypeFromString(XMLUtils::GetAttribute(currentNode, "class"));
    mapping.m_mappedTo =
        PeripheralTypeTranslator::GetTypeFromString(XMLUtils::GetAttribute(currentNode, "mapTo"));
    GetSettingsFromMappingsFile(currentNode, mapping.m_settings);

    m_mappings.push_back(mapping);
    CLog::LogF(LOGDEBUG, "loaded node \"{}\"", mapping.m_strDeviceName);
  }

  return true;
}

void CPeripherals::GetSettingsFromMappingsFile(
    tinyxml2::XMLElement* xmlNode, std::map<std::string, PeripheralDeviceSetting>& settings)
{
  auto* currentNode = xmlNode->FirstChildElement("setting");
  int iMaxOrder = 0;

  while (currentNode != nullptr)
  {
    SettingPtr setting;
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
      setting = std::make_shared<CSettingBool>(strKey, iLabelId, bValue);
    }
    else if (strSettingsType == "int")
    {
      int iValue = currentNode->Attribute("value") ? atoi(currentNode->Attribute("value")) : 0;
      int iMin = currentNode->Attribute("min") ? atoi(currentNode->Attribute("min")) : 0;
      int iStep = currentNode->Attribute("step") ? atoi(currentNode->Attribute("step")) : 1;
      int iMax = currentNode->Attribute("max") ? atoi(currentNode->Attribute("max")) : 255;
      setting = std::make_shared<CSettingInt>(strKey, iLabelId, iValue, iMin, iStep, iMax);
    }
    else if (strSettingsType == "float")
    {
      float fValue =
          currentNode->Attribute("value") ? (float)atof(currentNode->Attribute("value")) : 0;
      float fMin = currentNode->Attribute("min") ? (float)atof(currentNode->Attribute("min")) : 0;
      float fStep =
          currentNode->Attribute("step") ? (float)atof(currentNode->Attribute("step")) : 0;
      float fMax = currentNode->Attribute("max") ? (float)atof(currentNode->Attribute("max")) : 0;
      setting = std::make_shared<CSettingNumber>(strKey, iLabelId, fValue, fMin, fStep, fMax);
    }
    else if (StringUtils::EqualsNoCase(strSettingsType, "enum"))
    {
      std::string strEnums = XMLUtils::GetAttribute(currentNode, "lvalues");
      if (!strEnums.empty())
      {
        TranslatableIntegerSettingOptions enums;
        std::vector<std::string> valuesVec;
        StringUtils::Tokenize(strEnums, valuesVec, "|");
        for (unsigned int i = 0; i < valuesVec.size(); i++)
          enums.emplace_back(atoi(valuesVec[i].c_str()), atoi(valuesVec[i].c_str()));
        int iValue = currentNode->Attribute("value") ? atoi(currentNode->Attribute("value")) : 0;
        setting = std::make_shared<CSettingInt>(strKey, iLabelId, iValue, enums);
      }
    }
    else if (StringUtils::EqualsNoCase(strSettingsType, "addon"))
    {
      std::string addonFilter = XMLUtils::GetAttribute(currentNode, "addontype");
      ADDON::AddonType addonType = ADDON::CAddonInfo::TranslateType(addonFilter);
      std::string strValue = XMLUtils::GetAttribute(currentNode, "value");
      setting = std::make_shared<CSettingAddon>(strKey, iLabelId, strValue);
      static_cast<CSettingAddon&>(*setting).SetAddonType(addonType);
    }
    else
    {
      std::string strValue = XMLUtils::GetAttribute(currentNode, "value");
      setting = std::make_shared<CSettingString>(strKey, iLabelId, strValue);
    }

    if (setting)
    {
      //! @todo add more types if needed

      /* set the visibility */
      setting->SetVisible(bConfigurable);

      /* set the order */
      int iOrder = 0;
      currentNode->Attribute("order", std::to_string(iOrder).c_str());
      /* if the order attribute is invalid or 0, then the setting will be added at the end */
      if (iOrder < 0)
        iOrder = 0;
      if (iOrder > iMaxOrder)
        iMaxOrder = iOrder;

      /* and add this new setting */
      PeripheralDeviceSetting deviceSetting = {setting, iOrder};
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

void CPeripherals::GetDirectory(const std::string& strPath, CFileItemList& items) const
{
  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    if (StringUtils::EqualsNoCase(strBus, "all") ||
        StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(bus->Type())))
      bus->GetDirectory(strPath, items);
  }
}

PeripheralPtr CPeripherals::GetByPath(const std::string& strPath) const
{
  PeripheralPtr result;

  if (!StringUtils::StartsWithNoCase(strPath, "peripherals://"))
    return result;

  std::string strPathCut = strPath.substr(14);
  std::string strBus = strPathCut.substr(0, strPathCut.find('/'));

  std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
  for (const auto& bus : m_busses)
  {
    if (StringUtils::EqualsNoCase(strBus, PeripheralTypeTranslator::BusTypeToString(bus->Type())))
    {
      result = bus->GetByPath(strPath);
      break;
    }
  }

  return result;
}

bool CPeripherals::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_MUTE)
  {
    return ToggleMute();
  }

  if (SupportsCEC() && action.GetAmount() &&
      (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN))
  {
    PeripheralVector peripherals;
    if (GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
    {
      for (auto& peripheral : peripherals)
      {
        std::shared_ptr<CPeripheralCecAdapter> cecDevice =
            std::static_pointer_cast<CPeripheralCecAdapter>(peripheral);
        if (cecDevice->HasAudioControl())
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
  PeripheralVector peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (const auto& peripheral : peripherals)
    {
      std::shared_ptr<CPeripheralCecAdapter> cecDevice =
          std::static_pointer_cast<CPeripheralCecAdapter>(peripheral);
      if (cecDevice->IsMuted())
        return true;
    }
  }

  return false;
}

bool CPeripherals::ToggleMute()
{
  PeripheralVector peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      std::shared_ptr<CPeripheralCecAdapter> cecDevice =
          std::static_pointer_cast<CPeripheralCecAdapter>(peripheral);
      if (cecDevice->HasAudioControl())
      {
        cecDevice->ToggleMute();
        return true;
      }
    }
  }

  return false;
}

bool CPeripherals::ToggleDeviceState(CecStateChange mode /*= STATE_SWITCH_TOGGLE */)
{
  bool ret(false);
  PeripheralVector peripherals;

  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      std::shared_ptr<CPeripheralCecAdapter> cecDevice =
          std::static_pointer_cast<CPeripheralCecAdapter>(peripheral);
      ret |= cecDevice->ToggleDeviceState(mode);
    }
  }

  return ret;
}

bool CPeripherals::GetNextKeypress(float frameTime, CKey& key)
{
  PeripheralVector peripherals;
  if (SupportsCEC() && GetPeripheralsWithFeature(peripherals, FEATURE_CEC))
  {
    for (auto& peripheral : peripherals)
    {
      std::shared_ptr<CPeripheralCecAdapter> cecDevice =
          std::static_pointer_cast<CPeripheralCecAdapter>(peripheral);
      if (cecDevice->GetButton())
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

EventPollHandlePtr CPeripherals::RegisterEventPoller()
{
  return m_eventScanner->RegisterPollHandle();
}

EventLockHandlePtr CPeripherals::RegisterEventLock()
{
  return m_eventScanner->RegisterLock();
}

void CPeripherals::OnUserNotification()
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_INPUT_RUMBLENOTIFY))
    return;

  PeripheralVector peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_RUMBLE);

  for (auto& peripheral : peripherals)
    peripheral->OnUserNotification();
}

void CPeripherals::TestFeature(PeripheralFeature feature)
{
  PeripheralVector peripherals;
  GetPeripheralsWithFeature(peripherals, feature);

  for (auto& peripheral : peripherals)
  {
    if (peripheral->TestFeature(feature))
    {
      CLog::Log(LOGDEBUG, "PERIPHERALS: Device \"{}\" tested {} feature", peripheral->DeviceName(),
                PeripheralTypeTranslator::FeatureToString(feature));
    }
    else
    {
      if (peripheral->HasFeature(feature))
        CLog::Log(LOGDEBUG, "PERIPHERALS: Device \"{}\" failed to test {} feature",
                  peripheral->DeviceName(), PeripheralTypeTranslator::FeatureToString(feature));
      else
        CLog::Log(LOGDEBUG, "PERIPHERALS: Device \"{}\" doesn't support {} feature",
                  peripheral->DeviceName(), PeripheralTypeTranslator::FeatureToString(feature));
    }
  }
}

void CPeripherals::PowerOffDevices()
{
  TestFeature(FEATURE_POWER_OFF);
}

void CPeripherals::ProcessEvents(void)
{
  std::vector<PeripheralBusPtr> busses;
  {
    std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
    busses = m_busses;
  }

  for (PeripheralBusPtr& bus : busses)
    bus->ProcessEvents();
}

void CPeripherals::EnableButtonMapping()
{
  std::vector<PeripheralBusPtr> busses;
  {
    std::unique_lock<CCriticalSection> lock(m_critSectionBusses);
    busses = m_busses;
  }

  for (PeripheralBusPtr& bus : busses)
    bus->EnableButtonMapping();
}

PeripheralAddonPtr CPeripherals::GetAddonWithButtonMap(const CPeripheral* device)
{
  PeripheralBusAddonPtr addonBus =
      std::static_pointer_cast<CPeripheralBusAddon>(GetBusByType(PERIPHERAL_BUS_ADDON));

  PeripheralAddonPtr addon;

  PeripheralAddonPtr addonWithButtonMap;
  if (addonBus && addonBus->GetAddonWithButtonMap(device, addonWithButtonMap))
    addon = std::move(addonWithButtonMap);

  return addon;
}

void CPeripherals::ResetButtonMaps(const std::string& controllerId)
{
  PeripheralBusAddonPtr addonBus =
      std::static_pointer_cast<CPeripheralBusAddon>(GetBusByType(PERIPHERAL_BUS_ADDON));

  PeripheralVector peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);

  for (auto& peripheral : peripherals)
  {
    PeripheralAddonPtr addon;
    if (addonBus->GetAddonWithButtonMap(peripheral.get(), addon))
    {
      CAddonButtonMap buttonMap(peripheral.get(), addon, controllerId);
      buttonMap.Reset();
    }
  }
}

void CPeripherals::RegisterJoystickButtonMapper(IButtonMapper* mapper)
{
  PeripheralVector peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);
  GetPeripheralsWithFeature(peripherals, FEATURE_KEYBOARD);
  GetPeripheralsWithFeature(peripherals, FEATURE_MOUSE);

  for (auto& peripheral : peripherals)
    peripheral->RegisterJoystickButtonMapper(mapper);
}

void CPeripherals::UnregisterJoystickButtonMapper(IButtonMapper* mapper)
{
  mapper->ResetButtonMapCallbacks();

  PeripheralVector peripherals;
  GetPeripheralsWithFeature(peripherals, FEATURE_JOYSTICK);
  GetPeripheralsWithFeature(peripherals, FEATURE_KEYBOARD);
  GetPeripheralsWithFeature(peripherals, FEATURE_MOUSE);

  for (auto& peripheral : peripherals)
    peripheral->UnregisterJoystickButtonMapper(mapper);
}

void CPeripherals::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    // user set language, no longer use the TV's language
    PeripheralVector cecDevices;
    if (GetPeripheralsWithFeature(cecDevices, FEATURE_CEC) > 0)
    {
      for (auto& cecDevice : cecDevices)
        cecDevice->SetSetting("use_tv_menu_language", false);
    }
  }
}

void CPeripherals::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_INPUT_PERIPHERALS)
    CGUIDialogPeripherals::Show(*this);
  else if (settingId == CSettings::SETTING_INPUT_CONTROLLERCONFIG)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
  else if (settingId == CSettings::SETTING_INPUT_TESTRUMBLE)
    TestFeature(FEATURE_RUMBLE);
  else if (settingId == CSettings::SETTING_INPUT_PERIPHERALLIBRARIES)
  {
    std::string strAddonId;
    if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::AddonType::PERIPHERALDLL, strAddonId, false,
                                              true, true, false, true) == 1 &&
        !strAddonId.empty())
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(strAddonId, addon, ADDON::OnlyEnabled::CHOICE_YES))
        CGUIDialogAddonSettings::ShowForAddon(addon);
    }
  }
}

void CPeripherals::OnApplicationMessage(MESSAGING::ThreadMessage* pMsg)
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

void CPeripherals::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                            const std::string& sender,
                            const std::string& message,
                            const CVariant& data)
{
  if (flag == ANNOUNCEMENT::Player &&
      sender == ANNOUNCEMENT::CAnnouncementManager::ANNOUNCEMENT_SENDER)
  {
    if (message == "OnQuit")
    {
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_INPUT_CONTROLLERPOWEROFF))
        PowerOffDevices();
    }
  }
}
