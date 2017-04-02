/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "RumbleGenerator.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "games/GameServices.h"
#include "input/joysticks/IInputReceiver.h"
#include "ServiceBroker.h"

#include <algorithm>

#define RUMBLE_TEST_DURATION_MS          1000 // Per motor
#define RUMBLE_NOTIFICATION_DURATION_MS  300

 // From game.controller.default profile
#define WEAK_MOTOR_NAME        "rightmotor"

using namespace KODI;
using namespace JOYSTICK;

CRumbleGenerator::CRumbleGenerator(const std::string& controllerId) :
  CThread("RumbleGenerator"),
  m_motors(GetMotors(controllerId)),
  m_receiver(nullptr),
  m_type(RUMBLE_UNKNOWN)
{
}

void CRumbleGenerator::NotifyUser(IInputReceiver* receiver)
{
  if (receiver && !m_motors.empty())
  {
    if (IsRunning())
      StopThread(true);

    m_receiver = receiver;
    m_type = RUMBLE_NOTIFICATION;
    Create();
  }
}

bool CRumbleGenerator::DoTest(IInputReceiver* receiver)
{
  if (receiver && !m_motors.empty())
  {
    if (IsRunning())
      StopThread(true);

    m_receiver = receiver;
    m_type = RUMBLE_TEST;
    Create();

    return true;
  }
  return  false;
}

void CRumbleGenerator::Process(void)
{
  switch (m_type)
  {
  case RUMBLE_NOTIFICATION:
  {
    std::vector<std::string> motors;

    if (std::find(m_motors.begin(), m_motors.end(), WEAK_MOTOR_NAME) != m_motors.end())
      motors.push_back(WEAK_MOTOR_NAME);
    else
      motors = m_motors; // Not using default profile? Just rumble all motors

    for (const std::string& motor : motors)
      m_receiver->SetRumbleState(motor, 1.0f);

    Sleep(RUMBLE_NOTIFICATION_DURATION_MS);

    if (m_bStop)
      break;

    for (const std::string& motor : motors)
      m_receiver->SetRumbleState(motor, 0.0f);

    break;
  }
  case RUMBLE_TEST:
  {
    for (const std::string& motor : m_motors)
    {
      m_receiver->SetRumbleState(motor, 1.0f);

      Sleep(RUMBLE_TEST_DURATION_MS);

      if (m_bStop)
        break;

      m_receiver->SetRumbleState(motor, 0.0f);
    }
    break;
  }
  default:
    break;
  }
}

std::vector<std::string> CRumbleGenerator::GetMotors(const std::string& controllerId)
{
  using namespace GAME;

  std::vector<std::string> motors;

  CGameServices& gameServices = CServiceBroker::GetGameServices();
  ControllerPtr controller = gameServices.GetController(controllerId);
  if (controller)
  {
    for (const CControllerFeature& feature : controller->Layout().Features())
    {
      if (feature.Type() == FEATURE_TYPE::MOTOR)
        motors.push_back(feature.Name());
    }
  }

  return motors;
}
