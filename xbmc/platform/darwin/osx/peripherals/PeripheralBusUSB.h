/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "peripherals/bus/PeripheralBus.h"
#include "peripherals/devices/Peripheral.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>

namespace PERIPHERALS
{
  class CPeripherals;

  class CPeripheralBusUSB : public CPeripheralBus
  {
  public:
    explicit CPeripheralBusUSB(CPeripherals& manager);
    virtual ~CPeripheralBusUSB();

    /*!
     * @see PeripheralBus::PerformDeviceScan()
     */
    bool PerformDeviceScan(PeripheralScanResults &results);

  protected:
    PeripheralScanResults m_scan_results;
    static PeripheralType GetType(int iDeviceClass);
    static void  DeviceDetachCallback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
    static void  DeviceAttachCallback(CPeripheralBusUSB* refCon, io_iterator_t iterator);

    IONotificationPortRef m_notify_port;
    io_iterator_t         m_attach_iterator;
  };

}
