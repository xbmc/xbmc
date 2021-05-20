/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusUSB.h"

#include "peripherals/Peripherals.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

const static GUID USB_RAW_GUID = { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
const static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
const static GUID USB_DISK_GUID = { 0x53F56307, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };
const static GUID USB_NIC_GUID = { 0xAD498944, 0x762F, 0x11D0, { 0x8D, 0xCB, 0x00, 0xC0, 0x4F, 0xC3, 0x35, 0x8C } };

using namespace PERIPHERALS;
using namespace std::chrono_literals;

// Only to avoid endless loops while scanning for devices
#define MAX_BUS_DEVICES 2000

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals &manager) :
    CPeripheralBus("PeripBusUSB", manager, PERIPHERAL_BUS_USB)
{
  /* device removals aren't always triggering OnDeviceRemoved events, so poll for changes every 5 seconds to be sure we don't miss anything */
  m_iRescanTime = 5000ms;
}

bool CPeripheralBusUSB::PerformDeviceScan(PeripheralScanResults &results)
{
  /* XXX we'll just scan the RAW guid and find all devices. they'll show up as type 'unknown' in the UI,
     but the other option is that they're detected more than once, because RAW will return all devices.
     we have to scan the RAW guid here, because not every device is found by it's GUID correctly, e.g. CDC adapters. */
  return PerformDeviceScan(&USB_RAW_GUID, PERIPHERAL_UNKNOWN, results);
}

bool CPeripheralBusUSB::PerformDeviceScan(const GUID *guid, const PeripheralType defaultType, PeripheralScanResults &results)
{
  bool bReturn(false);

  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);

  return bReturn;
}

bool GetProductAndVendorId(const PeripheralType type, const std::string &strDeviceLocation, int *iVendorId, int *iProductId)
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);

  return false;
}

