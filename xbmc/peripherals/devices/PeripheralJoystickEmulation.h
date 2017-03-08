/*
 *      Copyright (C) 2015-2016 Team Kodi
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
#include "input/keyboard/IKeyboardHandler.h"
#include "threads/CriticalSection.h"

namespace PERIPHERALS
{
  class CPeripheralJoystickEmulation : public CPeripheral,
                                       public KODI::KEYBOARD::IKeyboardHandler
  {
  public:
    CPeripheralJoystickEmulation(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus);

    virtual ~CPeripheralJoystickEmulation(void);

    // implementation of CPeripheral
    virtual bool InitialiseFeature(const PeripheralFeature feature) override;
    virtual void RegisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler, bool bPromiscuous) override;
    virtual void UnregisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler) override;

    // implementation of IKeyboardHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override;

    /*!
     * \brief Number of the emulated controller (1-indexed)
     */
    unsigned int ControllerNumber(void) const;

  private:
    struct KeyboardHandle
    {
      KODI::KEYBOARD::IKeyboardHandler* handler;
      bool bPromiscuous;
    };

    typedef std::map<KODI::JOYSTICK::IDriverHandler*, KeyboardHandle> KeyboardHandlers;

    KeyboardHandlers m_keyboardHandlers;
    CCriticalSection m_mutex;
  };
}
