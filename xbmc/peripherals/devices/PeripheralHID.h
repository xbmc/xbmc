/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Peripheral.h"
#include "input/keyboard/XBMC_keyboard.h"

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralHID : public CPeripheral
{
public:
  CPeripheralHID(CPeripherals& manager,
                 const PeripheralScanResult& scanResult,
                 CPeripheralBus* bus);
  ~CPeripheralHID(void) override;
  bool InitialiseFeature(const PeripheralFeature feature) override;
  virtual bool LookupSymAndUnicode(XBMC_keysym& keysym, uint8_t* key, char* unicode)
  {
    return false;
  }
  void OnSettingChanged(const std::string& strChangedSetting) override;

protected:
  std::string m_strKeymap;
};
} // namespace PERIPHERALS
