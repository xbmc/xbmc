/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PeripheralBusIP.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

CPeripheralBusIP::CPeripheralBusIP(CPeripherals *manager) :
    CPeripheralBus(manager, PERIPHERAL_BUS_IP)
{
  m_bNeedsPolling = false;
}

bool CPeripheralBusIP::PerformDeviceScan(PeripheralScanResults &results)
{
	detectAmbiPi(results);
	return true;
}

void CPeripheralBusIP::detectAmbiPi(PeripheralScanResults &results)
{
	CStdString devicePath("AMBIPI");

	if (alreadyExistsOnPath(devicePath, results))
		return;

	addAmbiPiResult(devicePath, results);
}

bool CPeripheralBusIP::alreadyExistsOnPath(const CStdString& devicePath, PeripheralScanResults &results)
{
	PeripheralScanResult prevDevice;
	return results.GetDeviceOnLocation(devicePath, &prevDevice);
}

void CPeripheralBusIP::addAmbiPiResult(const CStdString& devicePath, PeripheralScanResults &results)
{
    PeripheralScanResult result;
    result.m_strLocation  = devicePath;
    result.m_type         = PERIPHERAL_AMBIPI;
    result.m_iVendorId    = 0;
    result.m_iProductId   = 0;

    if (results.ContainsResult(result))
		return;

    results.m_results.push_back(result);
}


