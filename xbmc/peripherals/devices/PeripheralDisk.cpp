/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralDisk.h"

#include "guilib/LocalizeStrings.h"

using namespace PERIPHERALS;

CPeripheralDisk::CPeripheralDisk(CPeripherals& manager,
                                 const PeripheralScanResult& scanResult,
                                 CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35003)
                                                       : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_DISK);
}
