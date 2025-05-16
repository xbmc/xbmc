/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "controllers/ControllerTypes.h"

#include <future>
#include <memory>
#include <string>

class CInputManager;
class CProfileManager;

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace RETRO
{
class CGUIGameRenderManager;
}

namespace GAME
{
class CAgentInput;
class CControllerManager;
class CGameSettings;

/*!
 * \ingroup games
 */
class CGameServices
{
public:
  CGameServices(CControllerManager& controllerManager,
                RETRO::CGUIGameRenderManager& renderManager,
                PERIPHERALS::CPeripherals& peripheralManager,
                const CProfileManager& profileManager,
                CInputManager& inputManager);
  ~CGameServices();

  ControllerPtr GetController(const std::string& controllerId) const;
  ControllerPtr GetDefaultController() const;
  ControllerPtr GetDefaultKeyboard() const;
  ControllerPtr GetDefaultMouse() const;
  ControllerVector GetControllers() const;

  /*!
   * \brief Translate a feature on a controller into its localized name
   *
   * \param controllerId The controller ID that the feature belongs to
   * \param featureName The feature name
   *
   * \return The localized feature name, or empty if the controller or feature
   *         doesn't exist
   */
  std::string TranslateFeature(const std::string& controllerId, const std::string& featureName) const;

  std::string GetSavestatesFolder() const;

  CGameSettings& GameSettings() const { return *m_gameSettings; }

  RETRO::CGUIGameRenderManager& GameRenderManager() const { return m_gameRenderManager; }

  CAgentInput& AgentInput() const { return *m_agentInput; }

  /*!
   * \brief Called when an add-on repo is installed
   *
   * If the repo contains game add-ons, it can introduce new file extensions
   * to the list of known game extensions.
   */
  void OnAddonRepoInstalled();

private:
  // Construction parameters
  CControllerManager& m_controllerManager;
  RETRO::CGUIGameRenderManager& m_gameRenderManager;
  const CProfileManager& m_profileManager;

  // Game services
  std::unique_ptr<CGameSettings> m_gameSettings;
  std::unique_ptr<CAgentInput> m_agentInput;

  // Game threads
  std::future<void> m_initializationTask;
};
} // namespace GAME
} // namespace KODI
