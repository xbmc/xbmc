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

#include "PeripheralBusUSBLibUdev.h"
#include "peripherals/Peripherals.h"
extern "C" {
#include <libudev.h>
}
#include <poll.h>
#include "utils/log.h"

#ifndef USB_CLASS_PER_INTERFACE
#define USB_CLASS_PER_INTERFACE         0
#endif

#ifndef USB_CLASS_AUDIO
#define USB_CLASS_AUDIO                 1
#endif

#ifndef USB_CLASS_COMM
#define USB_CLASS_COMM                  2
#endif

#ifndef USB_CLASS_HID
#define USB_CLASS_HID                   3
#endif

#ifndef USB_CLASS_PHYSICAL
#define USB_CLASS_PHYSICAL              5
#endif

#ifndef USB_CLASS_PTP
#define USB_CLASS_PTP                   6
#endif

#ifndef USB_CLASS_PRINTER
#define USB_CLASS_PRINTER               7
#endif

#ifndef USB_CLASS_MASS_STORAGE
#define USB_CLASS_MASS_STORAGE          8
#endif

#ifndef USB_CLASS_HUB
#define USB_CLASS_HUB                   9
#endif

#ifndef USB_CLASS_DATA
#define USB_CLASS_DATA                  10
#endif

#ifndef USB_CLASS_APP_SPEC
#define USB_CLASS_APP_SPEC              0xfe
#endif

#ifndef USB_CLASS_VENDOR_SPEC
#define USB_CLASS_VENDOR_SPEC           0xff
#endif

using namespace PERIPHERALS;

CPeripheralBusUSB::CPeripheralBusUSB(CPeripherals *manager) :
    CPeripheralBus(manager, PERIPHERAL_BUS_USB)
{
  /* the Process() method in this class overrides the one in CPeripheralBus, so leave this set to true */
  m_bNeedsPolling = true;

  m_udev          = NULL;
  m_udevMon       = NULL;

  if (!(m_udev = udev_new()))
  {
    CLog::Log(LOGERROR, "%s - failed to allocate udev context", __FUNCTION__);
    return;
  }

  /* set up a devices monitor that listen for any device change */
  m_udevMon = udev_monitor_new_from_netlink(m_udev, "udev");
  udev_monitor_enable_receiving(m_udevMon);

  CLog::Log(LOGDEBUG, "%s - initialised udev monitor", __FUNCTION__);
}

CPeripheralBusUSB::~CPeripheralBusUSB(void)
{
  StopThread(true);
  udev_monitor_unref(m_udevMon);
  udev_unref(m_udev);
}

bool CPeripheralBusUSB::PerformDeviceScan(PeripheralScanResults &results)
{
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev(NULL), *parent(NULL);
  enumerate = udev_enumerate_new(m_udev);
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  bool bContinue(true);
  CStdString strPath, strClass;
  udev_list_entry_foreach(dev_list_entry, devices)
  {
    strPath = udev_list_entry_get_name(dev_list_entry);
    if (strPath.IsEmpty())
      bContinue = false;

    if (bContinue)
    {
      if (!(parent = udev_device_new_from_syspath(m_udev, strPath)))
        bContinue = false;
    }

    if (bContinue)
    {
      dev = udev_device_get_parent(udev_device_get_parent(parent));
      if (!dev || !udev_device_get_sysattr_value(dev,"idVendor") || !udev_device_get_sysattr_value(dev, "idProduct"))
        bContinue = false;
    }

    if (bContinue)
    {
      strClass = udev_device_get_sysattr_value(dev, "bDeviceClass");
      if (strClass.IsEmpty())
        bContinue = false;
    }

    if (bContinue)
    {
      int iClass = PeripheralTypeTranslator::HexStringToInt(strClass.c_str());
      if (iClass == USB_CLASS_PER_INTERFACE)
      {
        //TODO just assume this is a HID device for now, since the only devices that we're currently
        //     interested in are HID devices
        iClass = USB_CLASS_HID;
      }

      PeripheralScanResult result;
      result.m_iVendorId   = PeripheralTypeTranslator::HexStringToInt(udev_device_get_sysattr_value(dev, "idVendor"));
      result.m_iProductId  = PeripheralTypeTranslator::HexStringToInt(udev_device_get_sysattr_value(dev, "idProduct"));
      result.m_type        = GetType(iClass);
      result.m_strLocation = udev_device_get_syspath(dev);

      if (!results.ContainsResult(result))
        results.m_results.push_back(result);
    }

    bContinue = true;
    if (parent)
    {
      /* unref the _parent_ device */
      udev_device_unref(parent);
      parent = NULL;
    }
  }
  /* Free the enumerator object */
  udev_enumerate_unref(enumerate);

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

void CPeripheralBusUSB::Process(void)
{
  bool bUpdated(false);
  ScanForDevices();
  while (!m_bStop)
  {
    bUpdated = WaitForUpdate();
    if (bUpdated && !m_bStop)
      ScanForDevices();
  }

  m_bIsStarted = false;
}

void CPeripheralBusUSB::Clear(void)
{
  StopThread(false);

  CPeripheralBus::Clear();
}

bool CPeripheralBusUSB::WaitForUpdate()
{
  int m_udevFd = udev_monitor_get_fd(m_udevMon);

  if (m_udevFd < 0)
  {
    CLog::Log(LOGERROR, "%s - get udev monitor", __FUNCTION__);
    return false;
  }

  /* poll for udev changes */
  struct pollfd pollFd;
  pollFd.fd = m_udevFd;
  pollFd.events = POLLIN;
  int iPollResult;
  while (!m_bStop && ((iPollResult = poll(&pollFd, 1, 100)) <= 0))
    if (errno != EINTR && iPollResult != 0)
      break;

  /* the thread is being stopped, so just return false */
  if (m_bStop)
    return false;

  /* we have to read the message from the queue, even though we're not actually using it */
  struct udev_device *dev = udev_monitor_receive_device(m_udevMon);
  if (dev)
    udev_device_unref(dev);
  else
  {
    CLog::Log(LOGERROR, "%s - failed to get device from udev_monitor_receive_device()", __FUNCTION__);
    Clear();
    return false;
  }

  return true;
}
