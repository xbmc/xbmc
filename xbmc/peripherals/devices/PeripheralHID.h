#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Peripheral.h"
#include "input/XBMC_keyboard.h"

namespace PERIPHERALS
{
  class CPeripheralHID : public CPeripheral
  {
  public:
    CPeripheralHID(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralHID(void);
    virtual bool InitialiseFeature(const PeripheralFeature feature);
    virtual bool LookupSymAndUnicode(XBMC_keysym &keysym, uint8_t *key, char *unicode) { return false; }
    virtual void OnSettingChanged(const CStdString &strChangedSetting);

  protected:
    bool       m_bInitialised;
    CStdString m_strKeymap;
  };
}
