/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusUSBLibUSB.h"
#include "peripherals/Peripherals.h"
#include <usb.h>
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals& manager) :
    CPeripheralBus("PeripBusUSB", manager, PERIPHERAL_BUS_USB)
{
  usb_init();
  usb_find_busses();
  m_busses = usb_get_busses();
  CLog::Log(LOGDEBUG, "%s - using libusb peripheral scanning", __FUNCTION__);
}

bool CPeripheralBusUSB::PerformDeviceScan(PeripheralScanResults &results)
{
  struct usb_bus *bus;
  usb_find_devices();
  for (bus = m_busses; bus; bus = bus->next)
  {
    struct usb_device *dev;
    for (dev = bus->devices; dev; dev = dev->next)
    {
      PeripheralScanResult result(m_type);
      result.m_iVendorId  = dev->descriptor.idVendor;
      result.m_iProductId = dev->descriptor.idProduct;
      result.m_type       = (dev->descriptor.bDeviceClass == USB_CLASS_PER_INTERFACE && dev->descriptor.bNumConfigurations > 0 &&
                             dev->config[0].bNumInterfaces > 0 && dev->config[0].interface[0].num_altsetting > 0) ?
                                 GetType(dev->config[0].interface[0].altsetting[0].bInterfaceClass) :
                                 GetType(dev->descriptor.bDeviceClass);
#ifdef TARGET_FREEBSD
      result.m_strLocation = StringUtils::Format("%s", dev->filename);
#else
      result.m_strLocation = StringUtils::Format("/bus%s/dev%s", bus->dirname, dev->filename);
#endif
      result.m_iSequence   = GetNumberOfPeripheralsWithId(result.m_iVendorId, result.m_iProductId);
      if (!results.ContainsResult(result))
        results.m_results.push_back(result);
    }
  }

  return true;
}

const PeripheralType CPeripheralBusUSB::GetType(int iDeviceClass)
{
  switch (iDeviceClass)
  {
  case USB_CLASS_HID:
    return PERIPHERAL_HID;
  case USB_CLASS_COMM:
    return PERIPHERAL_NIC;
  case USB_CLASS_MASS_STORAGE:
    return PERIPHERAL_DISK;
  case USB_CLASS_PER_INTERFACE:
  case USB_CLASS_AUDIO:
  case USB_CLASS_PRINTER:
  case USB_CLASS_PTP:
  case USB_CLASS_HUB:
  case USB_CLASS_DATA:
  case USB_CLASS_VENDOR_SPEC:
  default:
    return PERIPHERAL_UNKNOWN;
  }
}
