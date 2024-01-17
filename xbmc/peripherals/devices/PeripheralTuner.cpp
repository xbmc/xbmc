/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralTuner.h"

using namespace PERIPHERALS;

CPeripheralTuner::CPeripheralTuner(CPeripherals& manager,
                                   const PeripheralScanResult& scanResult,
                                   CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus)
{
  m_features.push_back(FEATURE_TUNER);
}
