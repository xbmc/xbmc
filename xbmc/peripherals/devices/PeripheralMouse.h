/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Peripheral.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "threads/CriticalSection.h"

#include <vector>

namespace PERIPHERALS
{
class CPeripheralMouse : public CPeripheral, public KODI::MOUSE::IMouseDriverHandler
{
public:
  CPeripheralMouse(CPeripherals& manager,
                   const PeripheralScanResult& scanResult,
                   CPeripheralBus* bus);

  ~CPeripheralMouse(void) override;

  // implementation of CPeripheral
  bool InitialiseFeature(const PeripheralFeature feature) override;
  void RegisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler,
                                  bool bPromiscuous) override;
  void UnregisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler) override;
  CDateTime LastActive() override { return m_lastActive; }

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
  CDateTime m_lastActive;
};
} // namespace PERIPHERALS
