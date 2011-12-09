/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#if defined(HAVE_LIBCEC)
#include "devices/PeripheralCecAdapter.h"
#endif
#include "bus/PeripheralBusUSB.h"
#include "dialogs/GUIDialogPeripheralManager.h"

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "settings/GUISettings.h"
#include "tinyXML/tinyxml.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;
using namespace XFILE;
using namespace std;

CPeripherals::CPeripherals(void)
{
  CDirectory::Create("special://profile/peripheral_data");

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

    /* load mappings from peripherals.xml */
    LoadMappings();

#if defined(HAVE_PERIPHERAL_BUS_USB)
    m_busses.push_back(new CPeripheralBusUSB(this));
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
    m_mappings.at(iMappingPtr).m_settings.clear();
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
  CPeripheralBus *bus(NULL);

  CSingleLock lock(m_critSection);
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

CPeripheral *CPeripherals::GetPeripheralAtLocation(const CStdString &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  CPeripheral *peripheral(NULL);
  CSingleLock lock(m_critSection);
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

bool CPeripherals::HasPeripheralAtLocation(const CStdString &strLocation, PeripheralBusType busType /* = PERIPHERAL_BUS_UNKNOWN */) const
{
  return (GetPeripheralAtLocation(strLocation, busType) != NULL);
}

CPeripheralBus *CPeripherals::GetBusWithDevice(const CStdString &strLocation) const
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
  int iReturn(0);
  CSingleLock lock(m_critSection);
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

CPeripheral *CPeripherals::CreatePeripheral(CPeripheralBus &bus, const PeripheralType type, const CStdString &strLocation, int iVendorId /* = 0 */, int iProductId /* = 0 */)
{
  CPeripheral *peripheral = NULL;
  /* check whether there's something mapped in peripherals.xml */
  PeripheralType mappedType = type;
  CStdString strDeviceName;
  int iMappingPtr = GetMappingForDevice(bus, type, iVendorId, iProductId);
  bool bHasMapping(iMappingPtr >= 0);
  if (bHasMapping)
  {
    mappedType    = m_mappings[iMappingPtr].m_mappedTo;
    strDeviceName = m_mappings[iMappingPtr].m_strDeviceName;
  }
  else
  {
    /* don't create instances for devices that aren't mapped in peripherals.xml */
    return NULL;
  }

  switch(mappedType)
  {
  case PERIPHERAL_HID:
    peripheral = new CPeripheralHID(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_NIC:
    peripheral = new CPeripheralNIC(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_DISK:
    peripheral = new CPeripheralDisk(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_NYXBOARD:
    peripheral = new CPeripheralNyxboard(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_TUNER:
    peripheral = new CPeripheralTuner(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_BLUETOOTH:
    peripheral = new CPeripheralBluetooth(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
    break;

  case PERIPHERAL_CEC:
#if defined(HAVE_LIBCEC)
    peripheral = new CPeripheralCecAdapter(type, bus.Type(), strLocation, strDeviceName, iVendorId, iProductId);
#else
    if (!m_bMissingLibCecWarningDisplayed)
    {
      m_bMissingLibCecWarningDisplayed = true;
      CLog::Log(LOGWARNING, "%s - libCEC support has not been compiled in, so the CEC adapter cannot be used.", __FUNCTION__);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(36000), g_localizeStrings.Get(36017));
    }
#endif
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
      if (!bHasMapping)
        peripheral->SetHidden(true);
      bus.Register(peripheral);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - failed to initialise peripheral on '%s'", __FUNCTION__, strLocation.c_str());
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

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35005), peripheral.DeviceName());
}

void CPeripherals::OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral)
{
  CGUIDialogPeripheralManager *dialog = (CGUIDialogPeripheralManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_MANAGER);
  if (dialog && dialog->IsActive())
    dialog->Update();

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(35006), peripheral.DeviceName());
}

int CPeripherals::GetMappingForDevice(const CPeripheralBus &bus, const PeripheralType classType, int iVendorId, int iProductId) const
{
  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (unsigned int iMappingPtr = 0; iMappingPtr < m_mappings.size(); iMappingPtr++)
  {
    PeripheralDeviceMapping mapping = m_mappings.at(iMappingPtr);

    bool bProductMatch = false;
    for (unsigned int i = 0; i < mapping.m_PeripheralID.size(); i++)
      if (mapping.m_PeripheralID[i].m_iVendorId == iVendorId && mapping.m_PeripheralID[i].m_iProductId == iProductId)
        bProductMatch = true;

    bool bBusMatch = (mapping.m_busType == PERIPHERAL_BUS_UNKNOWN || mapping.m_busType == bus.Type());
    bool bClassMatch = (mapping.m_class == PERIPHERAL_UNKNOWN || mapping.m_class == classType);

    if (bProductMatch && bBusMatch && bClassMatch)
    {
      CStdString strVendorId, strProductId;
      PeripheralTypeTranslator::FormatHexString(iVendorId, strVendorId);
      PeripheralTypeTranslator::FormatHexString(iProductId, strProductId);
      CLog::Log(LOGDEBUG, "%s - device (%s:%s) mapped to %s (type = %s)", __FUNCTION__, strVendorId.c_str(), strProductId.c_str(), mapping.m_strDeviceName.c_str(), PeripheralTypeTranslator::TypeToString(mapping.m_mappedTo));
      return iMappingPtr;
    }
  }

  return -1;
}

void CPeripherals::GetSettingsFromMapping(CPeripheral &peripheral) const
{
  /* check all mappings in the order in which they are defined in peripherals.xml */
  for (unsigned int iMappingPtr = 0; iMappingPtr < m_mappings.size(); iMappingPtr++)
  {
    const PeripheralDeviceMapping *mapping = &m_mappings.at(iMappingPtr);

    bool bProductMatch = false;
    for (unsigned int i = 0; i < mapping->m_PeripheralID.size(); i++)
      if (mapping->m_PeripheralID[i].m_iVendorId == peripheral.VendorId() && mapping->m_PeripheralID[i].m_iProductId == peripheral.ProductId())
        bProductMatch = true;

    bool bBusMatch = (mapping->m_busType == PERIPHERAL_BUS_UNKNOWN || mapping->m_busType == peripheral.GetBusType());
    bool bClassMatch = (mapping->m_class == PERIPHERAL_UNKNOWN || mapping->m_class == peripheral.Type());

    if (bBusMatch && bProductMatch && bClassMatch)
    {
      for (map<CStdString, CSetting *>::const_iterator itr = mapping->m_settings.begin(); itr != mapping->m_settings.end(); itr++)
        peripheral.AddSetting((*itr).first, (*itr).second);
    }
  }
}

bool CPeripherals::LoadMappings(void)
{
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile("special://xbmc/system/peripherals.xml"))
  {
    CLog::Log(LOGWARNING, "%s - peripherals.xml does not exist", __FUNCTION__);
    return true;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "peripherals") != 0)
  {
    CLog::Log(LOGERROR, "%s - peripherals.xml does not contain <peripherals>", __FUNCTION__);
    return false;
  }

  for (TiXmlElement *currentNode = pRootElement->FirstChildElement("peripheral"); currentNode; currentNode = currentNode->NextSiblingElement("peripheral"))
  {
    CStdStringArray vpArray, idArray;
    PeripheralID id;
    PeripheralDeviceMapping mapping;

    mapping.m_strDeviceName = currentNode->Attribute("name") ? CStdString(currentNode->Attribute("name")) : StringUtils::EmptyString;

    // If there is no vendor_product attribute ignore this entry
    if (!currentNode->Attribute("vendor_product"))
    {
      CLog::Log(LOGERROR, "%s - ignoring node \"%s\" with missing vendor_product attribute", __FUNCTION__, mapping.m_strDeviceName.c_str());
      continue;
    }

    // The vendor_product attribute is a list of comma separated vendor:product pairs
    StringUtils::SplitString(currentNode->Attribute("vendor_product"), ",", vpArray);
    for (unsigned int i = 0; i < vpArray.size(); i++)
    {
      StringUtils::SplitString(vpArray[i], ":", idArray);
      if (idArray.size() != 2)
      {
        CLog::Log(LOGERROR, "%s - ignoring node \"%s\" with invalid vendor_product attribute", __FUNCTION__, mapping.m_strDeviceName.c_str());
        continue;
      }

      id.m_iVendorId = PeripheralTypeTranslator::HexStringToInt(idArray[0]);
      id.m_iProductId = PeripheralTypeTranslator::HexStringToInt(idArray[1]);
      mapping.m_PeripheralID.push_back(id);
    }
    if (mapping.m_PeripheralID.size() == 0)
      continue;

    mapping.m_busType       = PeripheralTypeTranslator::GetBusTypeFromString(currentNode->Attribute("bus"));
    mapping.m_class         = PeripheralTypeTranslator::GetTypeFromString(currentNode->Attribute("class"));
    mapping.m_mappedTo      = PeripheralTypeTranslator::GetTypeFromString(currentNode->Attribute("mapTo"));
    GetSettingsFromMappingsFile(currentNode, mapping.m_settings);

    m_mappings.push_back(mapping);
    CLog::Log(LOGDEBUG, "%s - loaded node \"%s\"", __FUNCTION__, mapping.m_strDeviceName.c_str());
  }

  return true;
}

void CPeripherals::GetSettingsFromMappingsFile(TiXmlElement *xmlNode, map<CStdString, CSetting *> &m_settings)
{
  TiXmlElement *currentNode = xmlNode->FirstChildElement("setting");
  while (currentNode)
  {
    CSetting *setting = NULL;
    CStdString strKey(currentNode->Attribute("key"));
    if (strKey.IsEmpty())
      continue;

    CStdString strSettingsType(currentNode->Attribute("type"));
    int iLabelId = currentNode->Attribute("label") ? atoi(currentNode->Attribute("label")) : -1;
    bool bConfigurable = (!currentNode->Attribute("configurable") ||
                          strcmp(currentNode->Attribute("configurable"), "") == 0 ||
                           (strcmp(currentNode->Attribute("configurable"), "no") != 0 &&
                            strcmp(currentNode->Attribute("configurable"), "false") != 0 &&
                            strcmp(currentNode->Attribute("configurable"), "0") != 0));
    if (strSettingsType.Equals("bool"))
    {
      bool bValue = (strcmp(currentNode->Attribute("value"), "no") != 0 &&
                     strcmp(currentNode->Attribute("value"), "false") != 0 &&
                     strcmp(currentNode->Attribute("value"), "0") != 0);
      setting = new CSettingBool(0, strKey, iLabelId, bValue, CHECKMARK_CONTROL);
    }
    else if (strSettingsType.Equals("int"))
    {
      int iValue = currentNode->Attribute("value") ? atoi(currentNode->Attribute("value")) : 0;
      int iMin   = currentNode->Attribute("min") ? atoi(currentNode->Attribute("min")) : 0;
      int iStep  = currentNode->Attribute("step") ? atoi(currentNode->Attribute("step")) : 1;
      int iMax   = currentNode->Attribute("max") ? atoi(currentNode->Attribute("max")) : 255;
      CStdString strFormat(currentNode->Attribute("format"));
      setting = new CSettingInt(0, strKey, iLabelId, iValue, iMin, iStep, iMax, SPIN_CONTROL_INT, strFormat);
    }
    else if (strSettingsType.Equals("float"))
    {
      float fValue = currentNode->Attribute("value") ? (float) atof(currentNode->Attribute("value")) : 0;
      float fMin   = currentNode->Attribute("min") ? (float) atof(currentNode->Attribute("min")) : 0;
      float fStep  = currentNode->Attribute("step") ? (float) atof(currentNode->Attribute("step")) : 0;
      float fMax   = currentNode->Attribute("max") ? (float) atof(currentNode->Attribute("max")) : 0;
      setting = new CSettingFloat(0, strKey, iLabelId, fValue, fMin, fStep, fMax, SPIN_CONTROL_FLOAT);
    }
    else
    {
      CStdString strValue(currentNode->Attribute("value"));
      setting = new CSettingString(0, strKey, iLabelId, strValue, EDIT_CONTROL_INPUT, !bConfigurable, -1);
    }

    //TODO add more types if needed
    setting->SetVisible(bConfigurable);
    m_settings[strKey] = setting;
    currentNode = currentNode->NextSiblingElement("setting");
  }
}

void CPeripherals::GetDirectory(const CStdString &strPath, CFileItemList &items) const
{
  if (!strPath.Left(14).Equals("peripherals://"))
    return;

  CStdString strPathCut = strPath.Right(strPath.length() - 14);
  CStdString strBus = strPathCut.Left(strPathCut.Find('/'));

  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (strBus.Equals("all") || strBus.Equals(PeripheralTypeTranslator::BusTypeToString(m_busses.at(iBusPtr)->Type())))
      m_busses.at(iBusPtr)->GetDirectory(strPath, items);
  }
}

CPeripheral *CPeripherals::GetByPath(const CStdString &strPath) const
{
  if (!strPath.Left(14).Equals("peripherals://"))
    return NULL;

  CStdString strPathCut = strPath.Right(strPath.length() - 14);
  CStdString strBus = strPathCut.Left(strPathCut.Find('/'));

  CSingleLock lock(m_critSection);
  for (unsigned int iBusPtr = 0; iBusPtr < m_busses.size(); iBusPtr++)
  {
    if (strBus.Equals(PeripheralTypeTranslator::BusTypeToString(m_busses.at(iBusPtr)->Type())))
      return m_busses.at(iBusPtr)->GetByPath(strPath);
  }

  return NULL;
}
