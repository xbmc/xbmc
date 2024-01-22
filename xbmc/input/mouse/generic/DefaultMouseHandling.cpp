/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DefaultMouseHandling.h"

#include "MouseInputHandling.h"
#include "games/controllers/input/DefaultButtonMap.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"

using namespace KODI;
using namespace MOUSE;

CDefaultMouseHandling::CDefaultMouseHandling(PERIPHERALS::CPeripheral* peripheral,
                                             IMouseInputHandler* handler)
  : m_peripheral(peripheral), m_inputHandler(handler)
{
}

CDefaultMouseHandling::~CDefaultMouseHandling(void)
{
  m_driverHandler.reset();
  m_buttonMap.reset();
}

bool CDefaultMouseHandling::Load()
{
  std::string controllerId;
  if (m_inputHandler != nullptr)
    controllerId = m_inputHandler->ControllerID();

  if (!controllerId.empty())
    m_buttonMap = std::make_unique<GAME::CDefaultButtonMap>(m_peripheral, std::move(controllerId));

  if (m_buttonMap && m_buttonMap->Load())
  {
    m_driverHandler = std::make_unique<CMouseInputHandling>(m_inputHandler, m_buttonMap.get());
    return true;
  }

  return false;
}

bool CDefaultMouseHandling::OnPosition(int x, int y)
{
  if (m_driverHandler)
    return m_driverHandler->OnPosition(x, y);

  return false;
}

bool CDefaultMouseHandling::OnButtonPress(BUTTON_ID button)
{
  if (m_driverHandler)
    return m_driverHandler->OnButtonPress(button);

  return false;
}

void CDefaultMouseHandling::OnButtonRelease(BUTTON_ID button)
{
  if (m_driverHandler)
    m_driverHandler->OnButtonRelease(button);
}
