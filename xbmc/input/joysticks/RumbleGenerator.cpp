/*
 *      Copyright (C) 2016 Team Kodi
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
#include "addons/AddonManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "input/joysticks/IInputReceiver.h"
#include "settings/Settings.h"

#include <algorithm>

// From game.controller.default profile
#define STRONG_MOTOR_NAME  "leftmotor"
#define WEAK_MOTOR_NAME    "rightmotor"

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
    // Test now uses notification effect
    NotifyUser(receiver);
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
    const double duration = CSettings::GetInstance().GetNumber(CSettings::SETTING_INPUT_RUMBLE_DURATION);
    const unsigned int durationMs = std::min(static_cast<unsigned int>(duration * 1000), 1000U);

    const unsigned int strongPercent = CSettings::GetInstance().GetInt(CSettings::SETTING_INPUT_RUMBLE_STRONG);
    const unsigned int weakPercent = CSettings::GetInstance().GetInt(CSettings::SETTING_INPUT_RUMBLE_WEAK);

    for (const std::string& motor : m_motors)
    {
      if (motor == STRONG_MOTOR_NAME)
        m_receiver->SetRumbleState(motor, static_cast<float>(strongPercent) / 100.0f);
      else if (motor == WEAK_MOTOR_NAME)
        m_receiver->SetRumbleState(motor, static_cast<float>(weakPercent) / 100.0f);
      else
        m_receiver->SetRumbleState(motor, 1.0f);
    }

    Sleep(durationMs);

    if (m_bStop)
      break;

    for (const std::string& motor : m_motors)
      m_receiver->SetRumbleState(motor, 0.0f);

    break;
  }
  default:
    break;
  }
}

std::vector<std::string> CRumbleGenerator::GetMotors(const std::string& controllerId)
{
  using namespace ADDON;
  using namespace GAME;

  std::vector<std::string> motors;

  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(controllerId, addon, ADDON_GAME_CONTROLLER))
  {
    ControllerPtr controller = std::static_pointer_cast<CController>(addon);
    if (controller->LoadLayout())
    {
      for (const CControllerFeature& feature : controller->Layout().Features())
      {
        if (feature.Type() == JOYSTICK::FEATURE_TYPE::MOTOR)
          motors.push_back(feature.Name());
      }
    }
  }

  return motors;
}
