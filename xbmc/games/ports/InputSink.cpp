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

#include "InputSink.h"
#include "games/addons/GameClient.h"
#include "input/joysticks/JoystickIDs.h"

using namespace KODI;
using namespace GAME;

CInputSink::CInputSink(CGameClient &gameClient) :
  m_gameClient(gameClient)
{
}

std::string CInputSink::ControllerID(void) const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CInputSink::AcceptsInput(const std::string& feature) const
{
  return m_gameClient.AcceptsInput();
}

bool CInputSink::OnButtonPress(const std::string& feature, bool bPressed)
{
  return true;
}

bool CInputSink::OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return true;
}
