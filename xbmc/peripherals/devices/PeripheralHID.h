/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "Peripheral.h"
#include "input/XBMC_keyboard.h"

namespace PERIPHERALS
{
  class CPeripheralHID : public CPeripheral
  {
  public:
    CPeripheralHID(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus);
    ~CPeripheralHID(void) override;
    bool InitialiseFeature(const PeripheralFeature feature) override;
    virtual bool LookupSymAndUnicode(XBMC_keysym &keysym, uint8_t *key, char *unicode) { return false; }
    void OnSettingChanged(const std::string &strChangedSetting) override;

  protected:
    std::string m_strKeymap;
  };
}
