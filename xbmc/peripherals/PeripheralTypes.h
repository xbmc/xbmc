/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>
#ifdef TARGET_WINDOWS
#include "PlatformDefs.h"
#endif
#include "utils/StringUtils.h"

class CSetting;

namespace PERIPHERALS
{
/// \ingroup peripherals
/// \{

/*!
 * \brief Indicates a joystick has no preference for port number
 */
constexpr auto JOYSTICK_NO_PORT_REQUESTED = -1;

enum PeripheralBusType
{
  PERIPHERAL_BUS_UNKNOWN = 0,
  PERIPHERAL_BUS_USB,
  PERIPHERAL_BUS_PCI,
  PERIPHERAL_BUS_CEC,
  PERIPHERAL_BUS_ADDON,
#ifdef TARGET_ANDROID
  PERIPHERAL_BUS_ANDROID,
#endif
#if defined(TARGET_DARWIN)
  PERIPHERAL_BUS_GCCONTROLLER,
#endif
  PERIPHERAL_BUS_APPLICATION,
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
  FEATURE_POWER_OFF,
  FEATURE_KEYBOARD,
  FEATURE_MOUSE,
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
  PERIPHERAL_KEYBOARD,
  PERIPHERAL_MOUSE,
};

class CPeripheral;
using PeripheralPtr = std::shared_ptr<CPeripheral>;
using PeripheralVector = std::vector<PeripheralPtr>;

class CPeripheralAddon;
using PeripheralAddonPtr = std::shared_ptr<CPeripheralAddon>;
using PeripheralAddonVector = std::vector<PeripheralAddonPtr>;

class CEventPollHandle;
using EventPollHandlePtr = std::unique_ptr<CEventPollHandle>;

class CEventLockHandle;
using EventLockHandlePtr = std::unique_ptr<CEventLockHandle>;

struct PeripheralID
{
  int m_iVendorId;
  int m_iProductId;
};

struct PeripheralDeviceSetting
{
  std::shared_ptr<CSetting> m_setting;
  int m_order;
};

struct PeripheralDeviceMapping
{
  std::vector<PeripheralID> m_PeripheralID;
  PeripheralBusType m_busType;
  PeripheralType m_class;
  std::string m_strDeviceName;
  PeripheralType m_mappedTo;
  std::map<std::string, PeripheralDeviceSetting> m_settings;
};

class PeripheralTypeTranslator
{
public:
  static const char* TypeToString(const PeripheralType type)
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
      case PERIPHERAL_KEYBOARD:
        return "keyboard";
      case PERIPHERAL_MOUSE:
        return "mouse";
      default:
        return "unknown";
    }
  };

  static PeripheralType GetTypeFromString(const std::string& strType)
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
    else if (strTypeLowerCase == "keyboard")
      return PERIPHERAL_KEYBOARD;
    else if (strTypeLowerCase == "mouse")
      return PERIPHERAL_MOUSE;

    return PERIPHERAL_UNKNOWN;
  };

  static const char* BusTypeToString(const PeripheralBusType type)
  {
    switch (type)
    {
      case PERIPHERAL_BUS_USB:
        return "usb";
      case PERIPHERAL_BUS_PCI:
        return "pci";
      case PERIPHERAL_BUS_CEC:
        return "cec";
      case PERIPHERAL_BUS_ADDON:
        return "addon";
#ifdef TARGET_ANDROID
      case PERIPHERAL_BUS_ANDROID:
        return "android";
#endif
#if defined(TARGET_DARWIN)
      case PERIPHERAL_BUS_GCCONTROLLER:
        return "darwin_gccontroller";
#endif
      case PERIPHERAL_BUS_APPLICATION:
        return "application";
      default:
        return "unknown";
    }
  };

  static PeripheralBusType GetBusTypeFromString(const std::string& strType)
  {
    std::string strTypeLowerCase(strType);
    StringUtils::ToLower(strTypeLowerCase);

    if (strTypeLowerCase == "usb")
      return PERIPHERAL_BUS_USB;
    else if (strTypeLowerCase == "pci")
      return PERIPHERAL_BUS_PCI;
    else if (strTypeLowerCase == "cec")
      return PERIPHERAL_BUS_CEC;
    else if (strTypeLowerCase == "addon")
      return PERIPHERAL_BUS_ADDON;
#ifdef TARGET_ANDROID
    else if (strTypeLowerCase == "android")
      return PERIPHERAL_BUS_ANDROID;
#endif
#if defined(TARGET_DARWIN)
    else if (strTypeLowerCase == "darwin_gccontroller")
      return PERIPHERAL_BUS_GCCONTROLLER;
#endif
    else if (strTypeLowerCase == "application")
      return PERIPHERAL_BUS_APPLICATION;

    return PERIPHERAL_BUS_UNKNOWN;
  };

  static const char* FeatureToString(const PeripheralFeature type)
  {
    switch (type)
    {
      case FEATURE_HID:
        return "HID";
      case FEATURE_NIC:
        return "NIC";
      case FEATURE_DISK:
        return "disk";
      case FEATURE_NYXBOARD:
        return "nyxboard";
      case FEATURE_CEC:
        return "CEC";
      case FEATURE_BLUETOOTH:
        return "bluetooth";
      case FEATURE_TUNER:
        return "tuner";
      case FEATURE_IMON:
        return "imon";
      case FEATURE_JOYSTICK:
        return "joystick";
      case FEATURE_RUMBLE:
        return "rumble";
      case FEATURE_POWER_OFF:
        return "poweroff";
      case FEATURE_KEYBOARD:
        return "keyboard";
      case FEATURE_MOUSE:
        return "mouse";
      case FEATURE_UNKNOWN:
      default:
        return "unknown";
    }
  };

  static PeripheralFeature GetFeatureTypeFromString(const std::string& strType)
  {
    std::string strTypeLowerCase(strType);
    StringUtils::ToLower(strTypeLowerCase);

    if (strTypeLowerCase == "hid")
      return FEATURE_HID;
    else if (strTypeLowerCase == "cec")
      return FEATURE_CEC;
    else if (strTypeLowerCase == "disk")
      return FEATURE_DISK;
    else if (strTypeLowerCase == "nyxboard")
      return FEATURE_NYXBOARD;
    else if (strTypeLowerCase == "bluetooth")
      return FEATURE_BLUETOOTH;
    else if (strTypeLowerCase == "tuner")
      return FEATURE_TUNER;
    else if (strTypeLowerCase == "imon")
      return FEATURE_IMON;
    else if (strTypeLowerCase == "joystick")
      return FEATURE_JOYSTICK;
    else if (strTypeLowerCase == "rumble")
      return FEATURE_RUMBLE;
    else if (strTypeLowerCase == "poweroff")
      return FEATURE_POWER_OFF;
    else if (strTypeLowerCase == "keyboard")
      return FEATURE_KEYBOARD;
    else if (strTypeLowerCase == "mouse")
      return FEATURE_MOUSE;

    return FEATURE_UNKNOWN;
  };

  static int HexStringToInt(const char* strHex)
  {
    int iVal;
    sscanf(strHex, "%x", &iVal);
    return iVal;
  };

  static void FormatHexString(int iVal, std::string& strHexString)
  {
    if (iVal < 0)
      iVal = 0;
    if (iVal > 65536)
      iVal = 65536;

    strHexString = StringUtils::Format("{:04X}", iVal);
  };
};

class PeripheralScanResult
{
public:
  explicit PeripheralScanResult(const PeripheralBusType busType)
    : m_busType(busType), m_mappedBusType(busType)
  {
  }

  PeripheralScanResult(void) = default;

  bool operator==(const PeripheralScanResult& right) const
  {
    return m_iVendorId == right.m_iVendorId && m_iProductId == right.m_iProductId &&
           m_type == right.m_type && m_busType == right.m_busType &&
           StringUtils::EqualsNoCase(m_strLocation, right.m_strLocation);
  }

  bool operator!=(const PeripheralScanResult& right) const { return !(*this == right); }

  PeripheralType m_type = PERIPHERAL_UNKNOWN;
  std::string m_strLocation;
  int m_iVendorId = 0;
  int m_iProductId = 0;
  PeripheralType m_mappedType = PERIPHERAL_UNKNOWN;
  std::string m_strDeviceName;
  PeripheralBusType m_busType = PERIPHERAL_BUS_UNKNOWN;
  PeripheralBusType m_mappedBusType = PERIPHERAL_BUS_UNKNOWN;
  unsigned int m_iSequence = 0; // when more than one adapter of the same type is found
};

struct PeripheralScanResults
{
  bool GetDeviceOnLocation(const std::string& strLocation, PeripheralScanResult* result) const
  {
    for (const auto& it : m_results)
    {
      if (it.m_strLocation == strLocation)
      {
        *result = it;
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

/// \}
} // namespace PERIPHERALS
