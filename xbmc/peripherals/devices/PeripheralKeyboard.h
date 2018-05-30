/*
 *      Copyright (C) 2017 Team Kodi
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
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "threads/CriticalSection.h"

#include <vector>

namespace PERIPHERALS
{
  class CPeripheralKeyboard : public CPeripheral,
                              public KODI::KEYBOARD::IKeyboardDriverHandler
  {
  public:
    CPeripheralKeyboard(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus);

    ~CPeripheralKeyboard(void) override;

    // implementation of CPeripheral
    bool InitialiseFeature(const PeripheralFeature feature) override;
    void RegisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler, bool bPromiscuous) override;
    void UnregisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler) override;

    // implementation of IKeyboardDriverHandler
    bool OnKeyPress(const CKey& key) override;
    void OnKeyRelease(const CKey& key) override;

  private:
    struct KeyboardHandle
    {
      KODI::KEYBOARD::IKeyboardDriverHandler* handler;
      bool bPromiscuous;
    };

    std::vector<KeyboardHandle> m_keyboardHandlers;
    CCriticalSection m_mutex;
  };
}
