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

#ifndef OSX_PERIPHERALS_BUS_PERIPHERALBUS_H_INCLUDED
#define OSX_PERIPHERALS_BUS_PERIPHERALBUS_H_INCLUDED
#include "peripherals/bus/PeripheralBus.h"
#endif

#ifndef OSX_PERIPHERALS_DEVICES_PERIPHERAL_H_INCLUDED
#define OSX_PERIPHERALS_DEVICES_PERIPHERAL_H_INCLUDED
#include "peripherals/devices/Peripheral.h"
#endif


#ifdef TARGET_DARWIN_OSX
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#endif

namespace PERIPHERALS
{
  class CPeripherals;

  class CPeripheralBusUSB : public CPeripheralBus
  {
  public:
    CPeripheralBusUSB(CPeripherals *manager);
    virtual ~CPeripheralBusUSB();

    /*!
     * @see PeripheralBus::PerformDeviceScan()
     */
    bool PerformDeviceScan(PeripheralScanResults &results);

  protected:
    PeripheralScanResults m_scan_results;
  #ifdef TARGET_DARWIN_OSX
    static const PeripheralType GetType(int iDeviceClass);
    static void  DeviceDetachCallback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
    static void  DeviceAttachCallback(CPeripheralBusUSB* refCon, io_iterator_t iterator);

    IONotificationPortRef m_notify_port;
    io_iterator_t         m_attach_iterator;
  #endif
  };

}
