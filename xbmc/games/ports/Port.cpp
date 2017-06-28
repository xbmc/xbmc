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

#include "Port.h"
#include "PortInput.h"
#include "InputSink.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CPort::CPort(JOYSTICK::IInputHandler *gameInput, CGameClient &gameClient) :
  m_gameInput(gameInput),
  m_appInput(new CPortInput(gameClient)),
  m_inputSink(new CInputSink(gameClient))
{
}

CPort::~CPort()
{
}

void CPort::RegisterInput(JOYSTICK::IInputProvider *provider)
{
  // Register GUI input  as promiscuous to not block any input from the game
  provider->RegisterInputHandler(m_appInput.get(), true);

  // Give input sink the lowest priority by registering it before the other
  // non-promiscuous input handlers
  provider->RegisterInputHandler(m_inputSink.get(), false);

  // Register input handler
  provider->RegisterInputHandler(m_gameInput, false);
}

void CPort::UnregisterInput(JOYSTICK::IInputProvider *provider)
{
  // Unregister in reverse order
  provider->UnregisterInputHandler(m_gameInput);
  provider->UnregisterInputHandler(m_inputSink.get());
  provider->UnregisterInputHandler(m_appInput.get());
}
