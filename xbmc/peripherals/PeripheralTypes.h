#pragma once
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

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <stdio.h>
#include <vector>
#ifdef TARGET_WINDOWS
#include "PlatformDefs.h"
#endif
#include "utils/StringUtils.h"

class CSetting;

namespace PERIPHERALS
{
  enum PeripheralBusType
  {
    PERIPHERAL_BUS_UNKNOWN = 0,
    PERIPHERAL_BUS_USB,
    PERIPHERAL_BUS_PCI,
    PERIPHERAL_BUS_RPI,
    PERIPHERAL_BUS_CEC,
    PERIPHERAL_BUS_ADDON,
#ifdef TARGET_ANDROID
    PERIPHERAL_BUS_ANDROID,
#endif
  };

  enum PeripheralFeature
  {
    FEATURE_UNKNOWN = 0,
    FEATURE_HID,
    FEATURE_NIC,
    FEATURE_DISK,
    FEATURE_NYXBOARD,
    FEATURE_CEC,
    FEATURE_BLUETOOTH,
    FEATURE_TUNER,
    FEATURE_IMON,
    FEATURE_JOYSTICK,
    FEATURE_RUMBLE,
  };

  enum PeripheralType
  {
    PERIPHERAL_UNKNOWN = 0,
    PERIPHERAL_HID,
    PERIPHERAL_NIC,
    PERIPHERAL_DISK,
    PERIPHERAL_NYXBOARD,
    PERIPHERAL_CEC,
    PERIPHERAL_BLUETOOTH,
    PERIPHERAL_TUNER,
    PERIPHERAL_IMON,
    PERIPHERAL_JOYSTICK,
  };

  class CPeripheralAddon;
  typedef std::shared_ptr<CPeripheralAddon> PeripheralAddonPtr;
  typedef std::vector<PeripheralAddonPtr>   PeripheralAddonVector;

  struct PeripheralID
  {
    int m_iVendorId;
    int m_iProductId;
  };

  struct PeripheralDeviceSetting
  {
    CSetting* m_setting;
    int       m_order;
  };

  struct PeripheralDeviceMapping
  {
    std::vector<PeripheralID>                     m_PeripheralID;
    PeripheralBusType                             m_busType;
    PeripheralType                                m_class;
    std::string                                    m_strDeviceName;
    PeripheralType                                m_mappedTo;
    std::map<std::string, PeripheralDeviceSetting> m_settings;
  };

  class PeripheralTypeTranslator
  {
  public:
    static const char *TypeToString(const PeripheralType type)
    {
      switch (type)
      {
      case PERIPHERAL_BLUETOOTH:
        return "bluetooth";
      case PERIPHERAL_CEC:
        return "cec";
      case PERIPHERAL_DISK:
        return "disk";
      case PERIPHERAL_HID:
        return "hid";
      case PERIPHERAL_NIC:
        return "nic";
      case PERIPHERAL_NYXBOARD:
        return "nyxboard";
      case PERIPHERAL_TUNER:
        return "tuner";
      case PERIPHERAL_IMON:
        return "imon";
      case PERIPHERAL_JOYSTICK:
        return "joystick";
      default:
        return "unknown";
      }
    };

    static PeripheralType GetTypeFromString(const std::string &strType)
    {
      std::string strTypeLowerCase(strType);
      StringUtils::ToLower(strTypeLowerCase);

      if (strTypeLowerCase == "bluetooth")
        return PERIPHERAL_BLUETOOTH;
      else if (strTypeLowerCase == "cec")
        return PERIPHERAL_CEC;
      else if (strTypeLowerCase == "disk")
          return PERIPHERAL_DISK;
      else if (strTypeLowerCase == "hid")
        return PERIPHERAL_HID;
      else if (strTypeLowerCase == "nic")
        return PERIPHERAL_NIC;
      else if (strTypeLowerCase == "nyxboard")
        return PERIPHERAL_NYXBOARD;
      else if (strTypeLowerCase == "tuner")
        return PERIPHERAL_TUNER;
      else if (strTypeLowerCase == "imon")
        return PERIPHERAL_IMON;
      else if (strTypeLowerCase == "joystick")
        return PERIPHERAL_JOYSTICK;

      return PERIPHERAL_UNKNOWN;
    };

    static const char *BusTypeToString(const PeripheralBusType type)
    {
      switch(type)
      {
      case PERIPHERAL_BUS_USB:
        return "usb";
      case PERIPHERAL_BUS_PCI:
        return "pci";
      case PERIPHERAL_BUS_RPI:
        return "rpi";
      case PERIPHERAL_BUS_CEC:
        return "cec";
      case PERIPHERAL_BUS_ADDON:
        return "addon";
#ifdef TARGET_ANDROID
      case PERIPHERAL_BUS_ANDROID:
        return "android";
#endif
      default:
        return "unknown";
      }
    };

    static PeripheralBusType GetBusTypeFromString(const std::string &strType)
    {
      std::string strTypeLowerCase(strType);
      StringUtils::ToLower(strTypeLowerCase);

      if (strTypeLowerCase == "usb")
        return PERIPHERAL_BUS_USB;
      else if (strTypeLowerCase == "pci")
        return PERIPHERAL_BUS_PCI;
      else if (strTypeLowerCase == "rpi")
        return PERIPHERAL_BUS_RPI;
      else if (strTypeLowerCase == "cec")
        return PERIPHERAL_BUS_CEC;
      else if (strTypeLowerCase == "addon")
        return PERIPHERAL_BUS_ADDON;
#ifdef TARGET_ANDROID
      else if (strTypeLowerCase == "android")
        return PERIPHERAL_BUS_ANDROID;
#endif

      return PERIPHERAL_BUS_UNKNOWN;
    };

    static int HexStringToInt(const char *strHex)
    {
      int iVal;
      sscanf(strHex, "%x", &iVal);
      return iVal;
    };

    static void FormatHexString(int iVal, std::string &strHexString)
    {
      if (iVal < 0)
        iVal = 0;
      if (iVal > 65536)
        iVal = 65536;

      strHexString = StringUtils::Format("%04X", iVal);
    };
  };

  class PeripheralScanResult
  {
  public:
    PeripheralScanResult(const PeripheralBusType busType) :
      m_type(PERIPHERAL_UNKNOWN),
      m_iVendorId(0),
      m_iProductId(0),
      m_mappedType(PERIPHERAL_UNKNOWN),
      m_busType(busType),
      m_mappedBusType(busType),
      m_iSequence(0) {}

    PeripheralScanResult(void) :
      m_type(PERIPHERAL_UNKNOWN),
      m_iVendorId(0),
      m_iProductId(0),
      m_mappedType(PERIPHERAL_UNKNOWN),
      m_busType(PERIPHERAL_BUS_UNKNOWN),
      m_mappedBusType(PERIPHERAL_BUS_UNKNOWN),
      m_iSequence(0) {}

    bool operator ==(const PeripheralScanResult& right) const
    {
      return m_iVendorId  == right.m_iVendorId &&
             m_iProductId == right.m_iProductId &&
             m_type       == right.m_type &&
             m_busType    == right.m_busType &&
             StringUtils::EqualsNoCase(m_strLocation, right.m_strLocation);
    }

    bool operator !=(const PeripheralScanResult& right) const
    {
      return !(*this == right);
    }

    PeripheralType    m_type;
    std::string        m_strLocation;
    int               m_iVendorId;
    int               m_iProductId;
    PeripheralType    m_mappedType;
    std::string        m_strDeviceName;
    PeripheralBusType m_busType;
    PeripheralBusType m_mappedBusType;
    unsigned int      m_iSequence; // when more than one adapter of the same type is found
  };

  struct PeripheralScanResults
  {
    bool GetDeviceOnLocation(const std::string& strLocation, PeripheralScanResult* result) const
    {
      for (std::vector<PeripheralScanResult>::const_iterator it = m_results.begin(); it != m_results.end(); it++)
      {
        if ((*it).m_strLocation == strLocation)
        {
          *result = *it;
          return true;
        }
      }
      return false;
    }

    bool ContainsResult(const PeripheralScanResult& result) const
    {
      return std::find(m_results.begin(), m_results.end(), result) != m_results.end();
    }

    std::vector<PeripheralScanResult> m_results;
  };
}
