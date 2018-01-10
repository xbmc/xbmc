/*
 *      Copyright (C) 2015-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralBusApplication.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

CPeripheralBusApplication::CPeripheralBusApplication(CPeripherals& manager) :
    CPeripheralBus("PeripBusApplication", manager, PERIPHERAL_BUS_APPLICATION)
{
  // Initialize CPeripheralBus
  m_bNeedsPolling = false;
}

void CPeripheralBusApplication::Initialise(void)
{
  CPeripheralBus::Initialise();
  TriggerDeviceScan();
}

bool CPeripheralBusApplication::PerformDeviceScan(PeripheralScanResults& results)
{
  return true;
}

void CPeripheralBusApplication::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  // Don't list emulated joysticks in the GUI
}

std::string CPeripheralBusApplication::MakeLocation(unsigned int controllerIndex) const
{
  return StringUtils::Format("%u", controllerIndex);
}
