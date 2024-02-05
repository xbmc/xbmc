/*
*  Copyright (C) 2024 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "AgentMouse.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/mouse/interfaces/IMouseInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CAgentMouse::CAgentMouse(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral)), m_mouseActivity(std::make_unique<CControllerActivity>())
{
}

CAgentMouse::~CAgentMouse() = default;

void CAgentMouse::Initialize()
{
  // Record appearance to detect changes
  m_controllerAppearance = m_peripheral->ControllerProfile();

  // Upcast peripheral to input provider interface
  MOUSE::IMouseInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to silently observe all input
  inputProvider->RegisterMouseHandler(this, true, true);
}

void CAgentMouse::Deinitialize()
{
  // Upcast peripheral to input interface
  MOUSE::IMouseInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterMouseHandler(this);

  // Reset appearance
  m_controllerAppearance.reset();
}

void CAgentMouse::ClearButtonState()
{
  return m_mouseActivity->ClearButtonState();
}

float CAgentMouse::GetActivation() const
{
  return m_mouseActivity->GetActivation();
}

std::string CAgentMouse::ControllerID(void) const
{
  return DEFAULT_MOUSE_ID;
}

bool CAgentMouse::OnMotion(const MOUSE::PointerName& relpointer, int differenceX, int differenceY)
{
  m_mouseActivity->OnMouseMotion(relpointer, differenceX, differenceY);
  return true;
}

bool CAgentMouse::OnButtonPress(const MOUSE::ButtonName& button)
{
  m_mouseActivity->OnMouseButtonPress(button);
  return true;
}

void CAgentMouse::OnButtonRelease(const MOUSE::ButtonName& button)
{
  m_mouseActivity->OnMouseButtonRelease(button);
}

void CAgentMouse::OnInputFrame()
{
  m_mouseActivity->OnInputFrame();
}
