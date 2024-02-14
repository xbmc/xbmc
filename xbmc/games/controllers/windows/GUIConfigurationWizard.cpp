/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIConfigurationWizard.h"

#include "ServiceBroker.h"
#include "games/controllers/Controller.h"
#include "games/controllers/dialogs/GUIDialogAxisDetection.h"
#include "games/controllers/guicontrols/GUIFeatureButton.h"
#include "games/controllers/input/PhysicalFeature.h"
#include "input/InputManager.h"
#include "input/actions/ActionIDs.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/keyboard/KeyboardTranslator.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "input/keymaps/keyboard/KeyboardActionMap.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;
using namespace GAME;
using namespace std::chrono_literals;

namespace
{

#define ESC_KEY_CODE 27
#define SKIPPING_DETECTION_MS 200

// Duration to wait for axes to neutralize after mapping is finished
constexpr auto POST_MAPPING_WAIT_TIME_MS = 5000ms;

} // namespace

CGUIConfigurationWizard::CGUIConfigurationWizard()
  : CThread("GUIConfigurationWizard"), m_actionMap(new KEYMAP::CKeyboardActionMap)
{
  InitializeState();
}

CGUIConfigurationWizard::~CGUIConfigurationWizard(void) = default;

void CGUIConfigurationWizard::InitializeState(void)
{
  m_currentButton = nullptr;
  m_cardinalDirection = INPUT::CARDINAL_DIRECTION::NONE;
  m_wheelDirection = JOYSTICK::WHEEL_DIRECTION::NONE;
  m_throttleDirection = JOYSTICK::THROTTLE_DIRECTION::NONE;
  m_history.clear();
  m_lateAxisDetected = false;
  m_location.clear();
}

void CGUIConfigurationWizard::Run(const std::string& strControllerId,
                                  const std::vector<IFeatureButton*>& buttons)
{
  Abort();

  {
    std::unique_lock<CCriticalSection> lock(m_stateMutex);

    // Set Run() parameters
    m_strControllerId = strControllerId;
    m_buttons = buttons;

    // Reset synchronization variables
    m_inputEvent.Reset();
    m_motionlessEvent.Reset();
    m_bInMotion.clear();

    // Initialize state variables
    InitializeState();
  }

  Create();
}

void CGUIConfigurationWizard::OnUnfocus(IFeatureButton* button)
{
  std::unique_lock<CCriticalSection> lock(m_stateMutex);

  if (button == m_currentButton)
    Abort(false);
}

bool CGUIConfigurationWizard::Abort(bool bWait /* = true */)
{
  bool bWasRunning = !m_bStop;

  StopThread(false);

  m_inputEvent.Set();
  m_motionlessEvent.Set();

  if (bWait)
    StopThread(true);

  return bWasRunning;
}

void CGUIConfigurationWizard::RegisterKey(const CPhysicalFeature& key)
{
  if (key.Keycode() != XBMCK_UNKNOWN)
    m_keyMap[KEYBOARD::CKeyboardTranslator::TranslateKeycode(key.Keycode())] = key;
}

void CGUIConfigurationWizard::UnregisterKeys()
{
  m_keyMap.clear();
}

void CGUIConfigurationWizard::Process(void)
{
  CLog::Log(LOGDEBUG, "Starting configuration wizard");

  InstallHooks();

  bool bLateAxisDetected = false;

  {
    std::unique_lock<CCriticalSection> lock(m_stateMutex);
    for (IFeatureButton* button : m_buttons)
    {
      // Allow other threads to access the button we're using
      m_currentButton = button;

      while (!button->IsFinished())
      {
        // Allow other threads to access which direction the prompt is on
        m_cardinalDirection = button->GetCardinalDirection();
        m_wheelDirection = button->GetWheelDirection();
        m_throttleDirection = button->GetThrottleDirection();

        // Wait for input
        {
          using namespace JOYSTICK;

          CSingleExit exit(m_stateMutex);

          if (button->Feature().Type() == FEATURE_TYPE::UNKNOWN)
            CLog::Log(LOGDEBUG, "{}: Waiting for input", m_strControllerId);
          else
            CLog::Log(LOGDEBUG, "{}: Waiting for input for feature \"{}\"", m_strControllerId,
                      button->Feature().Name());

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

    bLateAxisDetected = m_lateAxisDetected;

    // Finished mapping
    InitializeState();
  }

  for (auto callback : ButtonMapCallbacks())
    callback.second->SaveButtonMap();

  if (bLateAxisDetected)
  {
    CGUIDialogAxisDetection dialog;
    dialog.Show();
  }
  else
  {
    // Wait for motion to stop to avoid sending analog actions for the button
    // that is pressed immediately after button mapping finishes.
    bool bInMotion;

    {
      std::unique_lock<CCriticalSection> lock(m_motionMutex);
      bInMotion = !m_bInMotion.empty();
    }

    if (bInMotion)
    {
      CLog::Log(LOGDEBUG, "Configuration wizard: waiting {}ms for axes to neutralize",
                POST_MAPPING_WAIT_TIME_MS.count());
      m_motionlessEvent.Wait(POST_MAPPING_WAIT_TIME_MS);
    }
  }

  RemoveHooks();

  CLog::Log(LOGDEBUG, "Configuration wizard ended");
}

bool CGUIConfigurationWizard::MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                                           KEYMAP::IKeymap* keymap,
                                           const JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace INPUT;
  using namespace JOYSTICK;

  bool bHandled = false;

  // Abort if another controller cancels the prompt
  if (IsMapping() && !IsMapping(buttonMap->Location()))
  {
    //! @todo This only succeeds for game.controller.default; no actions are
    //        currently defined for other controllers
    if (keymap)
    {
      std::string feature;
      if (buttonMap->GetFeature(primitive, feature))
      {
        const auto& actions = keymap->GetActions(CJoystickUtils::MakeKeyName(feature)).actions;
        if (!actions.empty())
        {
          //! @todo Handle multiple actions mapped to the same key
          OnAction(actions.begin()->actionId);
        }
      }
    }

    // Discard input
    bHandled = true;
  }
  else if (m_history.find(primitive) != m_history.end())
  {
    // Primitive has already been mapped this round, ignore it
    bHandled = true;
  }
  else if (buttonMap->IsIgnored(primitive))
  {
    bHandled = true;
  }
  else
  {
    // Get the current state of the thread
    IFeatureButton* currentButton;
    CARDINAL_DIRECTION cardinalDirection;
    WHEEL_DIRECTION wheelDirection;
    THROTTLE_DIRECTION throttleDirection;
    {
      std::unique_lock<CCriticalSection> lock(m_stateMutex);
      currentButton = m_currentButton;
      cardinalDirection = m_cardinalDirection;
      wheelDirection = m_wheelDirection;
      throttleDirection = m_throttleDirection;
    }

    if (currentButton)
    {
      // Check if we were expecting a keyboard key
      if (currentButton->NeedsKey())
      {
        if (primitive.Type() == PRIMITIVE_TYPE::KEY)
        {
          auto it =
              m_keyMap.find(KEYBOARD::CKeyboardTranslator::TranslateKeycode(primitive.Keycode()));
          if (it != m_keyMap.end())
          {
            const CPhysicalFeature& key = it->second;
            currentButton->SetKey(key);
            m_inputEvent.Set();
          }
        }
        else
        {
          //! @todo Check if primitive is a cancel or motion action
        }
        bHandled = true;
      }
      else
      {
        const CPhysicalFeature& feature = currentButton->Feature();

        if (primitive.Type() == PRIMITIVE_TYPE::RELATIVE_POINTER &&
            feature.Type() != FEATURE_TYPE::RELPOINTER)
        {
          // Don't allow relative pointers to map to other features
        }
        else
        {
          CLog::Log(LOGDEBUG, "{}: mapping feature \"{}\" for device {}", m_strControllerId,
                    feature.Name(), buttonMap->Location());

          switch (feature.Type())
          {
            case FEATURE_TYPE::SCALAR:
            {
              buttonMap->AddScalar(feature.Name(), primitive);
              bHandled = true;
              break;
            }
            case FEATURE_TYPE::ANALOG_STICK:
            {
              buttonMap->AddAnalogStick(feature.Name(), cardinalDirection, primitive);
              bHandled = true;
              break;
            }
            case FEATURE_TYPE::RELPOINTER:
            {
              buttonMap->AddRelativePointer(feature.Name(), cardinalDirection, primitive);
              bHandled = true;
              break;
            }
            case FEATURE_TYPE::WHEEL:
            {
              buttonMap->AddWheel(feature.Name(), wheelDirection, primitive);
              bHandled = true;
              break;
            }
            case FEATURE_TYPE::THROTTLE:
            {
              buttonMap->AddThrottle(feature.Name(), throttleDirection, primitive);
              bHandled = true;
              break;
            }
            case FEATURE_TYPE::KEY:
            {
              buttonMap->AddKey(feature.Name(), primitive);
              bHandled = true;
              break;
            }
            default:
              break;
          }
        }

        if (bHandled)
        {
          m_history.insert(primitive);

          // Don't record motion for relative pointers
          if (primitive.Type() != PRIMITIVE_TYPE::RELATIVE_POINTER)
            OnMotion(buttonMap);

          m_inputEvent.Set();

          if (m_location.empty())
          {
            m_location = buttonMap->Location();
            m_bIsKeyboard = (primitive.Type() == PRIMITIVE_TYPE::KEY);
          }
        }
      }
    }
  }

  return bHandled;
}

void CGUIConfigurationWizard::OnEventFrame(const JOYSTICK::IButtonMap* buttonMap, bool bMotion)
{
  std::unique_lock<CCriticalSection> lock(m_motionMutex);

  if (m_bInMotion.find(buttonMap) != m_bInMotion.end() && !bMotion)
    OnMotionless(buttonMap);
}

void CGUIConfigurationWizard::OnLateAxis(const JOYSTICK::IButtonMap* buttonMap,
                                         unsigned int axisIndex)
{
  std::unique_lock<CCriticalSection> lock(m_stateMutex);

  m_lateAxisDetected = true;
  Abort(false);
}

void CGUIConfigurationWizard::OnMotion(const JOYSTICK::IButtonMap* buttonMap)
{
  std::unique_lock<CCriticalSection> lock(m_motionMutex);

  m_motionlessEvent.Reset();
  m_bInMotion.insert(buttonMap);
}

void CGUIConfigurationWizard::OnMotionless(const JOYSTICK::IButtonMap* buttonMap)
{
  m_bInMotion.erase(buttonMap);
  if (m_bInMotion.empty())
    m_motionlessEvent.Set();
}

bool CGUIConfigurationWizard::OnKeyPress(const CKey& key)
{
  bool bHandled = false;

  if (!m_bStop)
  {
    // Only allow key to abort the prompt if we know for sure that we're mapping
    // a controller
    const bool bIsMappingController = (IsMapping() && !m_bIsKeyboard);

    if (bIsMappingController)
    {
      bHandled = OnAction(m_actionMap->GetActionID(key));
    }
    else
    {
      // Allow key press to fall through to the button mapper
    }
  }

  return bHandled;
}

bool CGUIConfigurationWizard::OnAction(unsigned int actionId)
{
  bool bHandled = false;

  switch (actionId)
  {
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_UP:
    case ACTION_MOVE_DOWN:
    case ACTION_PAGE_UP:
    case ACTION_PAGE_DOWN:
      // Abort and allow motion
      Abort(false);
      bHandled = false;
      break;

    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_STOP:
    case ACTION_NAV_BACK:
      // Abort and prevent action
      Abort(false);
      bHandled = true;
      break;

    default:
      // Absorb keypress
      bHandled = true;
      break;
  }

  return bHandled;
}

bool CGUIConfigurationWizard::IsMapping() const
{
  return !m_location.empty();
}

bool CGUIConfigurationWizard::IsMapping(const std::string& location) const
{
  return m_location == location;
}

void CGUIConfigurationWizard::InstallHooks(void)
{
  // Install button mapper with lowest priority
  CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);

  // Install hook to reattach button mapper when peripherals change
  CServiceBroker::GetPeripherals().RegisterObserver(this);

  // Install hook to cancel the button mapper
  CServiceBroker::GetInputManager().RegisterKeyboardDriverHandler(this);
}

void CGUIConfigurationWizard::RemoveHooks(void)
{
  CServiceBroker::GetInputManager().UnregisterKeyboardDriverHandler(this);
  CServiceBroker::GetPeripherals().UnregisterObserver(this);
  CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
}

void CGUIConfigurationWizard::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    {
      CServiceBroker::GetPeripherals().UnregisterJoystickButtonMapper(this);
      CServiceBroker::GetPeripherals().RegisterJoystickButtonMapper(this);
      break;
    }
    default:
      break;
  }
}
