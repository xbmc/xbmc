/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeymapHandling.h"

#include "KeymapHandler.h"
#include "ServiceBroker.h"
#include "input/InputManager.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "input/keymaps/ButtonTranslator.h"
#include "input/keymaps/Keymap.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace KEYMAP;

CKeymapHandling::CKeymapHandling(JOYSTICK::IInputProvider* inputProvider,
                                 bool pPromiscuous,
                                 const IKeymapEnvironment* environment)
  : m_inputProvider(inputProvider), m_pPromiscuous(pPromiscuous), m_environment(environment)
{
  LoadKeymaps();
  CServiceBroker::GetInputManager().RegisterObserver(this);
}

CKeymapHandling::~CKeymapHandling()
{
  CServiceBroker::GetInputManager().UnregisterObserver(this);
  UnloadKeymaps();
}

JOYSTICK::IInputReceiver* CKeymapHandling::GetInputReceiver(const std::string& controllerId) const
{
  auto it =
      std::find_if(m_inputHandlers.begin(), m_inputHandlers.end(),
                   [&controllerId](const std::unique_ptr<JOYSTICK::IInputHandler>& inputHandler)
                   { return inputHandler->ControllerID() == controllerId; });

  if (it != m_inputHandlers.end())
    return (*it)->InputReceiver();

  return nullptr;
}

IKeymap* CKeymapHandling::GetKeymap(const std::string& controllerId) const
{
  auto it = std::find_if(m_keymaps.begin(), m_keymaps.end(),
                         [&controllerId](const std::unique_ptr<IKeymap>& keymap)
                         { return keymap->ControllerID() == controllerId; });

  if (it != m_keymaps.end())
    return it->get();

  return nullptr;
}

void CKeymapHandling::Notify(const Observable& obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageButtonMapsChanged)
    LoadKeymaps();
}

void CKeymapHandling::LoadKeymaps()
{
  UnloadKeymaps();

  auto& inputManager = CServiceBroker::GetInputManager();

  for (auto& windowKeymap : inputManager.GetJoystickKeymaps())
  {
    // Create keymap
    std::unique_ptr<IKeymap> keymap(new CKeymap(std::move(windowKeymap), m_environment));

    // Create keymap handler
    std::unique_ptr<JOYSTICK::IInputHandler> inputHandler(
        new CKeymapHandler(&inputManager, keymap.get()));

    // Register the handler with the input provider
    m_inputProvider->RegisterInputHandler(inputHandler.get(), m_pPromiscuous);

    // Save the keymap and handler
    m_keymaps.emplace_back(std::move(keymap));
    m_inputHandlers.emplace_back(std::move(inputHandler));
  }
}

void CKeymapHandling::UnloadKeymaps()
{
  if (m_inputProvider != nullptr)
  {
    for (auto it = m_inputHandlers.rbegin(); it != m_inputHandlers.rend(); ++it)
      m_inputProvider->UnregisterInputHandler(it->get());
  }
  m_inputHandlers.clear();
  m_keymaps.clear();
}
