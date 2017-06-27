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

#include "KeymapHandling.h"
#include "KeymapHandler.h"
#include "input/joysticks/IInputHandler.h"
#include "input/joysticks/IInputProvider.h"
#include "input/Keymap.h"
#include "input/ButtonTranslator.h"
#include "input/InputManager.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace JOYSTICK;

CKeymapHandling::CKeymapHandling(IInputProvider *inputProvider, bool pPromiscuous, const IKeymapEnvironment *environment) :
  m_inputProvider(inputProvider)
{
  for (auto windowKeymap : CServiceBroker::GetInputManager().GetJoystickKeymaps())
  {
    // Create keymap
    std::unique_ptr<IKeymap> keymap(new CKeymap(windowKeymap, environment));

    // Create keymap handler
    IActionListener *actionHandler = &CServiceBroker::GetInputManager();
    std::unique_ptr<IInputHandler> inputHandler(new CKeymapHandler(actionHandler, keymap.get()));

    // Register the handler with the input provider
    m_inputProvider->RegisterInputHandler(inputHandler.get(), pPromiscuous);

    // Save the keymap and handler
    m_keymaps.emplace_back(std::move(keymap));
    m_inputHandlers.emplace_back(std::move(inputHandler));
  }
}

CKeymapHandling::~CKeymapHandling()
{
  for (auto it = m_inputHandlers.rbegin(); it != m_inputHandlers.rend(); ++it)
    m_inputProvider->UnregisterInputHandler(it->get());
  m_inputHandlers.clear();
  m_keymaps.clear();
}

IInputReceiver *CKeymapHandling::GetInputReceiver(const std::string &controllerId) const
{
  auto it = std::find_if(m_inputHandlers.begin(), m_inputHandlers.end(),
    [&controllerId](const std::unique_ptr<IInputHandler> &inputHandler)
    {
      return inputHandler->ControllerID() == controllerId;
    });

  if (it != m_inputHandlers.end())
    return (*it)->InputReceiver();

  return nullptr;
}

IKeymap *CKeymapHandling::GetKeymap(const std::string &controllerId) const
{
  auto it = std::find_if(m_keymaps.begin(), m_keymaps.end(),
    [&controllerId](const std::unique_ptr<IKeymap> &keymap)
    {
      return keymap->ControllerID() == controllerId;
    });

  if (it != m_keymaps.end())
    return it->get();

  return nullptr;
}
