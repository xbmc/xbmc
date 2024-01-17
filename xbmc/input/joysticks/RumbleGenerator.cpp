/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RumbleGenerator.h"

#include "ServiceBroker.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/ControllerManager.h"
#include "input/joysticks/interfaces/IInputReceiver.h"

#include <algorithm>

using namespace std::chrono_literals;

namespace
{
constexpr auto RUMBLE_TEST_DURATION_MS = 1000ms; // Per motor
constexpr auto RUMBLE_NOTIFICATION_DURATION_MS = 300ms;

// From game.controller.default profile
#define WEAK_MOTOR_NAME "rightmotor"
} // namespace

using namespace KODI;
using namespace JOYSTICK;

CRumbleGenerator::CRumbleGenerator()
  : CThread("RumbleGenerator"), m_motors(GetMotors(ControllerID()))
{
}

std::string CRumbleGenerator::ControllerID() const
{
  return DEFAULT_CONTROLLER_ID;
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
  return false;
}

void CRumbleGenerator::Process(void)
{
  switch (m_type)
  {
    case RUMBLE_NOTIFICATION:
    {
      std::vector<std::string> motors;

      if (std::find(m_motors.begin(), m_motors.end(), WEAK_MOTOR_NAME) != m_motors.end())
        motors.emplace_back(WEAK_MOTOR_NAME);
      else
        motors = m_motors; // Not using default profile? Just rumble all motors

      for (const std::string& motor : motors)
        m_receiver->SetRumbleState(motor, 1.0f);

      CThread::Sleep(RUMBLE_NOTIFICATION_DURATION_MS);

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

        CThread::Sleep(RUMBLE_TEST_DURATION_MS);

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

  CControllerManager& controllerManager = CServiceBroker::GetGameControllerManager();
  ControllerPtr controller = controllerManager.GetController(controllerId);
  if (controller)
    controller->GetFeatures(motors, FEATURE_TYPE::MOTOR);

  return motors;
}
