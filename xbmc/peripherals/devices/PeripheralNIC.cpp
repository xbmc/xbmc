/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralNIC.h"

#include "guilib/LocalizeStrings.h"

using namespace PERIPHERALS;

CPeripheralNIC::CPeripheralNIC(CPeripherals& manager,
                               const PeripheralScanResult& scanResult,
                               CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35002)
                                                       : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_NIC);
}
