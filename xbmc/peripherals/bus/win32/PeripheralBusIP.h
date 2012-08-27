#pragma once
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

#include "peripherals/bus/PeripheralBus.h"
#include "peripherals/devices/Peripheral.h"
#include <setupapi.h> //needed for GUID

namespace PERIPHERALS
{
  class CPeripherals;

  /*!
   * @class CPeripheralBusIP
   * This represents an IP bus on the system to support devices connected to the machine running XBMC via tcp/ip.
   *
   * TODO Implement support for non-hardcoded ip addresses and ports
   *  Either find devices by a) checking the manager's peripheral mappings and connect to devices (simple)
   *  b) OR issue a speacial new XBMC broadcast on the lan and process anything that comes back (hard)
   *
   * TODO Current implementation only supports a single AmbiPi device
   */
  class CPeripheralBusIP : public CPeripheralBus    
  {
  public:
    CPeripheralBusIP(CPeripherals *manager);

    /*!
     * @see PeripheralBus::PerformDeviceScan()
     */
    bool PerformDeviceScan(PeripheralScanResults &results);

  private:
	void CPeripheralBusIP::detectAmbiPi(PeripheralScanResults &results);
	bool CPeripheralBusIP::alreadyExistsOnPath(const CStdString& devicePath, PeripheralScanResults &results);
	void CPeripheralBusIP::addAmbiPiResult(const CStdString& devicePath, PeripheralScanResults &results);
  };
}
