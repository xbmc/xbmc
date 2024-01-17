/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Peripheral.h"

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralTuner : public CPeripheral
{
public:
  CPeripheralTuner(CPeripherals& manager,
                   const PeripheralScanResult& scanResult,
                   CPeripheralBus* bus);
  ~CPeripheralTuner(void) override = default;
};
} // namespace PERIPHERALS
