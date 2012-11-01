#pragma once
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

#include <map>
#include <stdio.h>
#ifdef _WIN32
#include "PlatformDefs.h"
#endif
#include "utils/StdString.h"

class CSetting;

namespace PERIPHERALS
{
  enum PeripheralBusType
  {
    PERIPHERAL_BUS_UNKNOWN = 0,
    PERIPHERAL_BUS_USB,
    PERIPHERAL_BUS_PCI,
    PERIPHERAL_BUS_RPI
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
    FEATURE_IMON
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
    PERIPHERAL_IMON 
  };

  struct PeripheralID
  {
    int m_iVendorId;
    int m_iProductId;
  };

  struct PeripheralDeviceMapping
  {
    std::vector<PeripheralID>        m_PeripheralID;
    PeripheralBusType                m_busType;
    PeripheralType                   m_class;
    CStdString                       m_strDeviceName;
    PeripheralType                   m_mappedTo;
    std::map<CStdString, CSetting *> m_settings;
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
      default:
        return "unknown";
      }
    };

    static PeripheralType GetTypeFromString(const CStdString &strType)
    {
      CStdString strTypeLowerCase(strType);
      strTypeLowerCase.ToLower();

      if (strTypeLowerCase.Equals("bluetooth"))
        return PERIPHERAL_BLUETOOTH;
      else if (strTypeLowerCase.Equals("cec"))
        return PERIPHERAL_CEC;
      else if (strTypeLowerCase.Equals("disk"))
          return PERIPHERAL_DISK;
      else if (strTypeLowerCase.Equals("hid"))
        return PERIPHERAL_HID;
      else if (strTypeLowerCase.Equals("nic"))
        return PERIPHERAL_NIC;
      else if (strTypeLowerCase.Equals("nyxboard"))
        return PERIPHERAL_NYXBOARD;
      else if (strTypeLowerCase.Equals("tuner"))
        return PERIPHERAL_TUNER;
      else if (strTypeLowerCase.Equals("imon"))
        return PERIPHERAL_IMON;

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
      default:
        return "unknown";
      }
    };

    static PeripheralBusType GetBusTypeFromString(const CStdString &strType)
    {
      CStdString strTypeLowerCase(strType);
      strTypeLowerCase.ToLower();

      if (strTypeLowerCase.Equals("usb"))
        return PERIPHERAL_BUS_USB;
      else if (strTypeLowerCase.Equals("pci"))
        return PERIPHERAL_BUS_PCI;
      else if (strTypeLowerCase.Equals("rpi"))
        return PERIPHERAL_BUS_RPI;

      return PERIPHERAL_BUS_UNKNOWN;
    };

    static int HexStringToInt(const char *strHex)
    {
      int iVal;
      sscanf(strHex, "%x", &iVal);
      return iVal;
    };

    static void FormatHexString(int iVal, CStdString &strHexString)
    {
      if (iVal < 0)
        iVal = 0;
      if (iVal > 65536)
        iVal = 65536;

      strHexString.Format("%04X", iVal);
    };
  };
}
