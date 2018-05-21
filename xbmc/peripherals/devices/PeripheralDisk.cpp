/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralDisk.h"
#include "guilib/LocalizeStrings.h"

using namespace PERIPHERALS;

CPeripheralDisk::CPeripheralDisk(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35003) : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_DISK);
}
