/*
 *      Copyright (C) 2017-2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "Peripheral.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "threads/CriticalSection.h"

#include <vector>

namespace PERIPHERALS
{
  class CPeripheralMouse : public CPeripheral,
                           public KODI::MOUSE::IMouseDriverHandler
  {
  public:
    CPeripheralMouse(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus);

    ~CPeripheralMouse(void) override;

    // implementation of CPeripheral
    bool InitialiseFeature(const PeripheralFeature feature) override;
    void RegisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler, bool bPromiscuous) override;
    void UnregisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler) override;

    // implementation of IMouseDriverHandler
    bool OnPosition(int x, int y) override;
    bool OnButtonPress(KODI::MOUSE::BUTTON_ID button) override;
    void OnButtonRelease(KODI::MOUSE::BUTTON_ID button) override;

  private:
    struct MouseHandle
    {
      KODI::MOUSE::IMouseDriverHandler* handler;
      bool bPromiscuous;
    };

    std::vector<MouseHandle> m_mouseHandlers;
    CCriticalSection m_mutex;
  };
}
