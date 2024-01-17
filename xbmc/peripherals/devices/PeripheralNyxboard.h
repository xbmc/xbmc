/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PeripheralHID.h"

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralNyxboard : public CPeripheralHID
{
public:
  CPeripheralNyxboard(CPeripherals& manager,
                      const PeripheralScanResult& scanResult,
                      CPeripheralBus* bus);
  ~CPeripheralNyxboard(void) override = default;
  bool LookupSymAndUnicode(XBMC_keysym& keysym, uint8_t* key, char* unicode) override;
};
} // namespace PERIPHERALS
