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

#define RUMBLE_DURATION_MS     1000

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
    for (const std::string& motor : m_motors)
      m_receiver->SetRumbleState(motor, 1.0f);

    Sleep(1000);

    if (m_bStop)
      break;

    for (const std::string& motor : m_motors)
      m_receiver->SetRumbleState(motor, 0.0f);

    break;
  }
  case RUMBLE_TEST:
  {
    for (const std::string& motor : m_motors)
    {
      m_receiver->SetRumbleState(motor, 1.0f);

      Sleep(1000);

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
