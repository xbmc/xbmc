/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusCEC.h"

#include <libcec/cec.h>

using namespace PERIPHERALS;
using namespace CEC;

CPeripheralBusCEC::CPeripheralBusCEC(CPeripherals& manager)
  : CPeripheralBus("PeripBusCEC", manager, PERIPHERAL_BUS_CEC)
{
  m_cecAdapter = CECInitialise(&m_configuration);
}

CPeripheralBusCEC::~CPeripheralBusCEC(void)
{
  if (m_cecAdapter)
    CECDestroy(m_cecAdapter);
}

bool CPeripheralBusCEC::PerformDeviceScan(PeripheralScanResults& results)
{
  cec_adapter_descriptor deviceList[10];
  int8_t iFound = m_cecAdapter->DetectAdapters(deviceList, 10, NULL, true);

  for (uint8_t iDevicePtr = 0; iDevicePtr < iFound; iDevicePtr++)
  {
    PeripheralScanResult result(m_type);
    result.m_iVendorId = deviceList[iDevicePtr].iVendorId;
    result.m_iProductId = deviceList[iDevicePtr].iProductId;
    result.m_strLocation = deviceList[iDevicePtr].strComName;
    result.m_type = PERIPHERAL_CEC;

    // override the bus type, so users don't have to reconfigure their adapters
    switch (deviceList[iDevicePtr].adapterType)
    {
      case ADAPTERTYPE_P8_EXTERNAL:
      case ADAPTERTYPE_P8_DAUGHTERBOARD:
        result.m_mappedBusType = PERIPHERAL_BUS_USB;
        break;
      default:
        break;
    }

    result.m_iSequence = GetNumberOfPeripheralsWithId(result.m_iVendorId, result.m_iProductId);
    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }
  return true;
}
