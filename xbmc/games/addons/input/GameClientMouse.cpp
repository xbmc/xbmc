/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientMouse.h"

#include "GameClientInput.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/addons/GameClient.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/mouse/interfaces/IMouseInputProvider.h"

#include <utility>

using namespace KODI;
using namespace GAME;

CGameClientMouse::CGameClientMouse(CGameClient& gameClient,
                                   std::string controllerId,
                                   MOUSE::IMouseInputProvider* inputProvider)
  : m_gameClient(gameClient),
    m_controllerId(std::move(controllerId)),
    m_inputProvider(inputProvider),
    m_mouseActivity(std::make_unique<CControllerActivity>())
{
  inputProvider->RegisterMouseHandler(this, false, false);
}

CGameClientMouse::~CGameClientMouse()
{
  m_inputProvider->UnregisterMouseHandler(this);
}

std::string CGameClientMouse::ControllerID(void) const
{
  return m_controllerId;
}

bool CGameClientMouse::OnMotion(const std::string& relpointer, int dx, int dy)
{
  //! @todo Allow mouse motion to activate controller
  //! This will follow a larger refactoring of the mouse input system
  //m_mouseActivity->OnMouseMotion(relpointer, differenceX, differenceY);

  // Only allow activated input in fullscreen game
  if (!m_gameClient.Input().AcceptsInput())
  {
    return false;
  }

  const std::string controllerId = ControllerID();

  game_input_event event;

  event.type = GAME_INPUT_EVENT_RELATIVE_POINTER;
  event.controller_id = m_controllerId.c_str();
  event.port_type = GAME_PORT_MOUSE;
  event.port_address = MOUSE_PORT_ADDRESS;
  event.feature_name = relpointer.c_str();
  event.rel_pointer.x = dx;
  event.rel_pointer.y = dy;

  return m_gameClient.Input().InputEvent(event);
}

bool CGameClientMouse::OnButtonPress(const std::string& button)
{
  m_mouseActivity->OnMouseButtonPress(button);
  m_mouseActivity->OnInputFrame();

  // Only allow activated input in fullscreen game
  if (!m_gameClient.Input().AcceptsInput())
  {
    return false;
  }

  game_input_event event;

  event.type = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.controller_id = m_controllerId.c_str();
  event.port_type = GAME_PORT_MOUSE;
  event.port_address = MOUSE_PORT_ADDRESS;
  event.feature_name = button.c_str();
  event.digital_button.pressed = true;

  return m_gameClient.Input().InputEvent(event);
}

void CGameClientMouse::OnButtonRelease(const std::string& button)
{
  m_mouseActivity->OnMouseButtonRelease(button);
  m_mouseActivity->OnInputFrame();

  game_input_event event;

  event.type = GAME_INPUT_EVENT_DIGITAL_BUTTON;
  event.controller_id = m_controllerId.c_str();
  event.port_type = GAME_PORT_MOUSE;
  event.port_address = MOUSE_PORT_ADDRESS;
  event.feature_name = button.c_str();
  event.digital_button.pressed = false;

  m_gameClient.Input().InputEvent(event);
}

void CGameClientMouse::OnInputFrame()
{
  m_mouseActivity->OnInputFrame();
}

float CGameClientMouse::GetActivation() const
{
  return m_mouseActivity->GetActivation();
}

void CGameClientMouse::SetSource(PERIPHERALS::PeripheralPtr sourcePeripheral)
{
  m_sourcePeripheral = std::move(sourcePeripheral);
}

void CGameClientMouse::ClearSource()
{
  m_sourcePeripheral.reset();
}
