#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include "PeripheralHID.h"

namespace PERIPHERALS
{
  class CPeripheralImon : public CPeripheralHID
  {
  public:
    CPeripheralImon(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralImon(void) {}
    virtual bool InitialiseFeature(const PeripheralFeature feature);
    virtual void OnDeviceRemoved();
    inline bool IsImonConflictsWithDInput() 
    { return m_ImonConflictsWithDInput;}
    static inline long GetCountOfImonsConflictWithDInput()
    { return m_CountOfImonsConflictWithDInput; }
    static void ActionOnImonConflict(bool deviceInserted = true);
  private:
    bool m_ImonConflictsWithDInput;
    static volatile long m_CountOfImonsConflictWithDInput;
  };
}
