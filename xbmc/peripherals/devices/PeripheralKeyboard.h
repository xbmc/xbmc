/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Peripheral.h"
#include "XBDateTime.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "threads/CriticalSection.h"

#include <vector>

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralKeyboard : public CPeripheral, public KODI::KEYBOARD::IKeyboardDriverHandler
{
public:
  CPeripheralKeyboard(CPeripherals& manager,
                      const PeripheralScanResult& scanResult,
                      CPeripheralBus* bus);

  ~CPeripheralKeyboard(void) override;

  // implementation of CPeripheral
  bool InitialiseFeature(const PeripheralFeature feature) override;
  void RegisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler,
                                     bool bPromiscuous) override;
  void UnregisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler) override;
  CDateTime LastActive() const override { return m_lastActive; }
  KODI::GAME::ControllerPtr ControllerProfile() const override;

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
  CDateTime m_lastActive;
};
} // namespace PERIPHERALS
