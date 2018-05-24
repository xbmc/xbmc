/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "PeripheralBusCEC.h"

#include <libcec/cec.h>

using namespace PERIPHERALS;
using namespace CEC;

CPeripheralBusCEC::CPeripheralBusCEC(CPeripherals& manager) :
    CPeripheralBus("PeripBusCEC", manager, PERIPHERAL_BUS_CEC)
{
  m_cecAdapter = CECInitialise(&m_configuration);
}

CPeripheralBusCEC::~CPeripheralBusCEC(void)
{
  if (m_cecAdapter)
    CECDestroy(m_cecAdapter);
}

bool CPeripheralBusCEC::PerformDeviceScan(PeripheralScanResults &results)
{
  cec_adapter_descriptor deviceList[10];
  int8_t iFound = m_cecAdapter->DetectAdapters(deviceList, 10, NULL, true);

  for (uint8_t iDevicePtr = 0; iDevicePtr < iFound; iDevicePtr++)
  {
    PeripheralScanResult result(m_type);
    result.m_iVendorId   = deviceList[iDevicePtr].iVendorId;
    result.m_iProductId  = deviceList[iDevicePtr].iProductId;
    result.m_strLocation = deviceList[iDevicePtr].strComName;
    result.m_type        = PERIPHERAL_CEC;

    // override the bus type, so users don't have to reconfigure their adapters
    switch(deviceList[iDevicePtr].adapterType)
    {
    case ADAPTERTYPE_P8_EXTERNAL:
    case ADAPTERTYPE_P8_DAUGHTERBOARD:
      result.m_mappedBusType = PERIPHERAL_BUS_USB;
      break;
    case ADAPTERTYPE_RPI:
      result.m_mappedBusType = PERIPHERAL_BUS_RPI;
      /** the Pi's adapter cannot be removed, no need to rescan */
      m_bNeedsPolling = false;
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
