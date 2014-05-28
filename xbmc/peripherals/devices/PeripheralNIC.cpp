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

#ifndef DEVICES_PERIPHERALNIC_H_INCLUDED
#define DEVICES_PERIPHERALNIC_H_INCLUDED
#include "PeripheralNIC.h"
#endif

#ifndef DEVICES_UTILS_LOG_H_INCLUDED
#define DEVICES_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef DEVICES_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define DEVICES_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif


using namespace PERIPHERALS;
using namespace std;

CPeripheralNIC::CPeripheralNIC(const PeripheralScanResult& scanResult) :
  CPeripheral(scanResult)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35002) : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_NIC);
}
