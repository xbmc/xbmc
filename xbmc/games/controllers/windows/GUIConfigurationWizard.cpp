/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "GUIConfigurationWizard.h"
#include "games/controllers/guicontrols/GUIFeatureButton.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "input/joysticks/IButtonMap.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace GAME;

#define ESC_KEY_CODE  27

CGUIConfigurationWizard::CGUIConfigurationWizard() :
  CThread("GUIConfigurationWizard")
{
  InitializeState();
}

void CGUIConfigurationWizard::InitializeState(void)
{
  m_currentButton = nullptr;
  m_currentDirection = JOYSTICK::CARDINAL_DIRECTION::UNKNOWN;
  m_history.clear();
}

void CGUIConfigurationWizard::Run(const std::string& strControllerId, const std::vector<IFeatureButton*>& buttons)
{
  Abort();

  {
    CSingleLock lock(m_stateMutex);

    // Set Run() parameters
    m_strControllerId = strControllerId;
    m_buttons = buttons;

    // Initialize state variables
    InitializeState();
  }

  Create();
}

void CGUIConfigurationWizard::OnUnfocus(IFeatureButton* button)
{
  CSingleLock lock(m_stateMutex);

  if (button == m_currentButton)
    Abort(false);
}

bool CGUIConfigurationWizard::Abort(bool bWait /* = true */)
{
  if (IsRunning())
  {
    StopThread(false);

    m_inputEvent.Set();

    if (bWait)
      StopThread(true);

    return true;
  }
  return false;
}

void CGUIConfigurationWizard::Process(void)
{
  CLog::Log(LOGDEBUG, "Starting configuration wizard");

  InstallHooks();

  {
    CSingleLock lock(m_stateMutex);
    for (IFeatureButton* button : m_buttons)
    {
      // Allow other threads to access the button we're using
      m_currentButton = button;

      while (!button->IsFinished())
      {
        // Allow other threads to access which direction the analog stick is on
        m_currentDirection = button->GetDirection();

        // Wait for input
        {
          CSingleExit exit(m_stateMutex);
          if (!button->PromptForInput(m_inputEvent))
            Abort(false);
        }

        if (m_bStop)
          break;
      }

      button->Reset();

      if (m_bStop)
        break;
    }

    m_currentButton = nullptr;
  }

  RemoveHooks();

  CLog::Log(LOGDEBUG, "Configuration wizard ended");
}

bool CGUIConfigurationWizard::MapPrimitive(JOYSTICK::IButtonMap* buttonMap, const JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace JOYSTICK;

  bool bHandled = false;

  // Handle esc key separately
  if (primitive.Type() == PRIMITIVE_TYPE::BUTTON &&
      primitive.Index() == ESC_KEY_CODE)
  {
    bHandled = Abort(false);
  }
  else if (m_history.find(primitive) != m_history.end())
  {
    // Primitive has already been mapped this round, ignore it
    bHandled = true;
  }
  else
  {
    // Get the current state of the thread
    IFeatureButton* currentButton;
    CARDINAL_DIRECTION currentDirection;
    {
      CSingleLock lock(m_stateMutex);
      currentButton = m_currentButton;
      currentDirection = m_currentDirection;
    }

    if (currentButton)
    {
      const CControllerFeature& feature = currentButton->Feature();
      switch (feature.Type())
      {
        case FEATURE_TYPE::SCALAR:
        {
          bHandled = buttonMap->AddScalar(feature.Name(), primitive);
          break;
        }
        case FEATURE_TYPE::ANALOG_STICK:
        {
          CDriverPrimitive up;
          CDriverPrimitive down;
          CDriverPrimitive right;
          CDriverPrimitive left;

          buttonMap->GetAnalogStick(feature.Name(), up, down, right, left);

          switch (currentDirection)
          {
            case CARDINAL_DIRECTION::UP:    up    = primitive; break;
            case CARDINAL_DIRECTION::DOWN:  down  = primitive; break;
            case CARDINAL_DIRECTION::RIGHT: right = primitive; break;
            case CARDINAL_DIRECTION::LEFT:  left  = primitive; break;
            default:
              break;
          }

          bHandled = buttonMap->AddAnalogStick(feature.Name(), up, down, right, left);

          break;
        }
        default:
          break;
      }

      if (bHandled)
      {
        m_history.insert(primitive);
        m_inputEvent.Set();
      }
    }
  }
  
  return bHandled;
}

bool CGUIConfigurationWizard::OnKeyPress(const CKey& key)
{
  return Abort(false);
}

void CGUIConfigurationWizard::InstallHooks(void)
{
  using namespace PERIPHERALS;

  g_peripherals.RegisterJoystickButtonMapper(this);
  g_peripherals.RegisterObserver(this);
  CInputManager::GetInstance().RegisterKeyboardHandler(this);
}

void CGUIConfigurationWizard::RemoveHooks(void)
{
  using namespace PERIPHERALS;

  CInputManager::GetInstance().UnregisterKeyboardHandler(this);
  g_peripherals.UnregisterObserver(this);
  g_peripherals.UnregisterJoystickButtonMapper(this);
}

void CGUIConfigurationWizard::Notify(const Observable& obs, const ObservableMessage msg)
{
  using namespace PERIPHERALS;

  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    {
      g_peripherals.UnregisterJoystickButtonMapper(this);
      g_peripherals.RegisterJoystickButtonMapper(this);
      break;
    }
    default:
      break;
  }
}
