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

#ifndef LINUX_PERIPHERALS_BUS_PERIPHERALBUS_H_INCLUDED
#define LINUX_PERIPHERALS_BUS_PERIPHERALBUS_H_INCLUDED
#include "peripherals/bus/PeripheralBus.h"
#endif

#ifndef LINUX_PERIPHERALS_DEVICES_PERIPHERAL_H_INCLUDED
#define LINUX_PERIPHERALS_DEVICES_PERIPHERAL_H_INCLUDED
#include "peripherals/devices/Peripheral.h"
#endif


struct udev;
struct udev_monitor;

namespace PERIPHERALS
{
  class CPeripherals;

  class CPeripheralBusUSB : public CPeripheralBus
  {
  public:
    CPeripheralBusUSB(CPeripherals *manager);
    virtual ~CPeripheralBusUSB(void);

    virtual void Clear(void);

    /*!
     * @see PeripheralBus::PerformDeviceScan()
     */
    bool PerformDeviceScan(PeripheralScanResults &results);

  protected:
    static const PeripheralType GetType(int iDeviceClass);

    virtual void Process(void);
    bool WaitForUpdate(void);

    struct udev *        m_udev;
    struct udev_monitor *m_udevMon;
  };
}
