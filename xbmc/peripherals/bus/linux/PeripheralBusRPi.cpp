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

#include "PeripheralBusRPi.h"
#include <libcec/cectypes.h>

extern "C" {
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>
}

using namespace PERIPHERALS;

#define RPI_PERIPHERAL_BUS_VID 0x2708
#define RPI_PERIPHERAL_CEC_PID 0x1001

CPeripheralBusRPi::CPeripheralBusRPi(CPeripherals *manager) :
    CPeripheralBus(manager, PERIPHERAL_BUS_RPI)
{
  m_bNeedsPolling = false;
}

bool CPeripheralBusRPi::PerformDeviceScan(PeripheralScanResults &results)
{
  if (FindAdapter())
  {
    PeripheralScanResult result;
    result.m_iVendorId   = RPI_PERIPHERAL_BUS_VID;
    result.m_iProductId  = RPI_PERIPHERAL_CEC_PID;
    result.m_type        = PERIPHERAL_CEC;
    result.m_strLocation = CEC_RPI_VIRTUAL_COM;

    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }

  return true;
}

bool CPeripheralBusRPi::FindAdapter(void)
{
  uint8_t iResult;

  VCHI_INSTANCE_T vchiq_instance;
  if ((iResult = vchi_initialise(&vchiq_instance)) != VCHIQ_SUCCESS)
    return false;

  if ((iResult = vchi_connect(NULL, 0, vchiq_instance)) != VCHIQ_SUCCESS)
    return false;

  return true;
}
